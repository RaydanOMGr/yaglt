#include "glcompat/backend/gles/gles_backend.hpp"

#include <EGL/eglext.h>

#include <cstdlib>
#include <cstring>
#include <string>

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif
#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif
#ifndef EGL_CONTEXT_MAJOR_VERSION
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#endif
#ifndef EGL_CONTEXT_MINOR_VERSION
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#endif

namespace glcompat {

GLESBackend::GLESBackend()
    : lib_(std::make_shared<GLESLib>()),
      factory_(lib_),
      compiler_(lib_) {}

GLESBackend::~GLESBackend() {
    if (initialized_) shutdown();
}

bool GLESBackend::createContext() {
    if (!lib_->eglGetPlatformDisplay && !lib_->eglGetDisplay) return false;

    // Prefer a surfaceless display (no window system needed).
    if (lib_->eglGetPlatformDisplay) {
        EGLint attrs[] = {EGL_NONE};
        display_ = lib_->eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                               EGL_DEFAULT_DISPLAY, attrs);
    }
    if (display_ == EGL_NO_DISPLAY && lib_->eglGetDisplay) {
        display_ = lib_->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (display_ == EGL_NO_DISPLAY) return false;

    EGLint major = 0, minor = 0;
    if (!lib_->eglInitialize(display_, &major, &minor)) return false;

    // Choose a config suitable for offscreen rendering.
    static const EGLint cfgAttrs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16,
        EGL_NONE};
    EGLConfig config = nullptr;
    EGLint num = 0;
    if (!lib_->eglChooseConfig(display_, cfgAttrs, &config, 1, &num) || num == 0) {
        return false;
    }

    static const EGLint ctxAttrs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE};
    context_ = lib_->eglCreateContext(display_, config, EGL_NO_CONTEXT, ctxAttrs);
    if (context_ == EGL_NO_CONTEXT) return false;

    // Surfaceless: bind with no draw/read surfaces.
    if (!lib_->eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
        return false;
    }
    return true;
}

void GLESBackend::queryVersion() {
    const GLubyte* v = lib_->glGetString(GL_VERSION);
    lib_->versionString = v ? reinterpret_cast<const char*>(v) : "";
    const GLubyte* r = lib_->glGetString(GL_RENDERER);
    lib_->rendererString = r ? reinterpret_cast<const char*>(r) : "";

    lib_->glesMajor = 0;
    lib_->glesMinor = 0;
    // Typical: "OpenGL ES 3.1 v1.2.3" or "OpenGL ES 2.0".
    std::string s = lib_->versionString;
    auto pos = s.find("OpenGL ES ");
    if (pos != std::string::npos) {
        s = s.substr(pos + 10);
        int mj = std::atoi(s.c_str());
        size_t dot = s.find('.');
        int mn = 0;
        if (dot != std::string::npos) mn = std::atoi(s.c_str() + dot + 1);
        lib_->glesMajor = mj;
        lib_->glesMinor = mn;
    }

    // Extensions: GLES3 exposes glGetStringi; fall back to glGetString.
    lib_->extensionsString.clear();
    if (lib_->glGetStringi && lib_->glGetIntegerv) {
        GLint n = 0;
        lib_->glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (GLint i = 0; i < n; ++i) {
            const GLubyte* e = nullptr;
            lib_->glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i), &e);
            if (e) {
                if (!lib_->extensionsString.empty()) lib_->extensionsString += " ";
                lib_->extensionsString += reinterpret_cast<const char*>(e);
            }
        }
    } else if (lib_->glGetString) {
        const GLubyte* e = lib_->glGetString(GL_EXTENSIONS);
        if (e) lib_->extensionsString = reinterpret_cast<const char*>(e);
    }
}

bool GLESBackend::initialize() {
    if (initialized_) return true;
    if (!lib_->load()) return false;
    if (!createContext()) {
        lib_->unload();
        return false;
    }
    queryVersion();
    populateGLESCapabilities(caps_, *lib_);
    initialized_ = true;
    return true;
}

void GLESBackend::shutdown() {
    if (display_ != EGL_NO_DISPLAY) {
        if (context_ != EGL_NO_CONTEXT && lib_->eglDestroyContext)
            lib_->eglDestroyContext(display_, context_);
        if (lib_->eglTerminate) lib_->eglTerminate(display_);
    }
    context_ = EGL_NO_CONTEXT;
    display_ = EGL_NO_DISPLAY;
    initialized_ = false;
}

std::string GLESBackend::describe() const {
    return "GLESBackend(ES " + std::to_string(lib_->glesMajor) + "." +
           std::to_string(lib_->glesMinor) + ", renderer=" + lib_->rendererString + ")";
}

} // namespace glcompat
