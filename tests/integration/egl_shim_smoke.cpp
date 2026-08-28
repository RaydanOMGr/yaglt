// Integration smoke test for the libEGL drop-in shim.
//
// Builds as a normal executable that links the shim (libEGL.so). At runtime the
// shim loads the real system libEGL (YAGLT_HOST_EGL) and routes gl* through
// YAGLT. We verify:
//   1. EGL calls are forwarded to the real driver (eglGetDisplay / Initialize).
//   2. eglGetProcAddress("gl...") returns YAGLT's GL implementation, so the
//      YAGLT version string is reported for GL_VERSION.
//
// Run with:
//   YAGLT_HOST_EGL=<real libEGL.so.1> \
//   LD_LIBRARY_PATH=<shim dir>:<mesa lib dir> \
//   ./egl_shim_smoke

#include <EGL/egl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

typedef const char* (*PFNGLGETSTRINGPROC)(unsigned int);
typedef void (*PFNGLGENBUFFERSPROC)(int, unsigned int*);
typedef void (*PFNGLBINBUFFERPROC)(unsigned int, unsigned int);
typedef unsigned int (*PFNGLGETERRORPROC)(void);

static int failures = 0;

#define CHECK(cond, msg)                                          \
    do {                                                          \
        if (!(cond)) {                                            \
            std::fprintf(stderr, "FAIL: %s\n", msg);              \
            ++failures;                                           \
        } else {                                                  \
            std::fprintf(stderr, "ok:   %s\n", msg);              \
        }                                                         \
    } while (0)

int main() {
    // 1. EGL forwarding to the real driver.
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    CHECK(dpy != EGL_NO_DISPLAY, "eglGetDisplay forwarded to real driver");
    if (dpy == EGL_NO_DISPLAY) {
        std::fprintf(stderr, "shim could not reach a real EGL driver "
                             "(set YAGLT_HOST_EGL)\n");
        return 2;
    }

    EGLint major = 0, minor = 0;
    CHECK(eglInitialize(dpy, &major, &minor) == EGL_TRUE,
          "eglInitialize forwarded to real driver");

    static const EGLint cfgAttrs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16,
        EGL_NONE};
    EGLConfig config = nullptr;
    EGLint num = 0;
    CHECK(eglChooseConfig(dpy, cfgAttrs, &config, 1, &num) == EGL_TRUE && num > 0,
          "eglChooseConfig forwarded to real driver");

    static const EGLint ctxAttrs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};
    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctxAttrs);
    CHECK(ctx != EGL_NO_CONTEXT, "eglCreateContext forwarded + YAGLT ctx created");

    CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) == EGL_TRUE,
          "eglMakeCurrent forwarded + YAGLT ctx made current");

    // 2. GL routing through YAGLT via eglGetProcAddress.
    PFNGLGETSTRINGPROC pfnGetString =
        reinterpret_cast<PFNGLGETSTRINGPROC>(eglGetProcAddress("glGetString"));
    CHECK(pfnGetString != nullptr, "eglGetProcAddress(\"glGetString\") -> YAGLT");

    const char* version = pfnGetString(0x1F02 /* GL_VERSION */);
    std::string verStr(version ? version : "");
    CHECK(verStr.find("YAGLT") != std::string::npos,
          "glGetString(GL_VERSION) reports YAGLT (routed through shim)");
    if (version) std::fprintf(stderr, "   GL_VERSION = %s\n", version);

    // 3. A real GL draw-path call exercises YAGLT's frontend + GLES backend.
    PFNGLGENBUFFERSPROC pfnGen =
        reinterpret_cast<PFNGLGENBUFFERSPROC>(eglGetProcAddress("glGenBuffers"));
    PFNGLBINBUFFERPROC pfnBind =
        reinterpret_cast<PFNGLBINBUFFERPROC>(eglGetProcAddress("glBindBuffer"));
    PFNGLGETERRORPROC pfnErr =
        reinterpret_cast<PFNGLGETERRORPROC>(eglGetProcAddress("glGetError"));
    if (pfnGen && pfnBind && pfnErr) {
        unsigned int buf = 0;
        pfnGen(1, &buf);
        pfnBind(0x8892 /* GL_ARRAY_BUFFER */, buf);
        CHECK(pfnErr() == 0, "glGenBuffers/glBindBuffer via shim: GL_NO_ERROR");
    } else {
        CHECK(false, "glGenBuffers/glBindBuffer resolvable via shim");
    }

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);

    if (failures != 0) {
        std::fprintf(stderr, "\n%d check(s) failed\n", failures);
        return 1;
    }
    std::fprintf(stderr, "\nall checks passed\n");
    return 0;
}
