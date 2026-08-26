#include "glcompat/backend/gles/gles_loader.hpp"

#include <dlfcn.h>

#include <vector>

namespace glcompat {

namespace {
void* openLib(const std::vector<const char*>& names) {
    for (const char* n : names) {
        void* h = dlopen(n, RTLD_NOW | RTLD_LOCAL);
        if (h) return h;
    }
    return nullptr;
}

template <typename F>
bool resolve(void* lib, F& fn, const char* sym) {
    fn = reinterpret_cast<F>(dlsym(lib, sym));
    return fn != nullptr;
}
} // namespace

bool GLESLib::load() {
    void* egl = openLib({"libEGL.so.1", "libEGL.so", "libEGL.so.2"});
    void* gles = openLib(
        {"libGLESv2.so.2", "libGLESv2.so", "libGLESv2.so.1", "libGLESv3.so"});
    if (!egl || !gles) {
        if (egl) dlclose(egl);
        if (gles) dlclose(gles);
        return false;
    }

    bool ok = true;
    // EGL
    ok &= resolve(egl, eglInitialize, "eglInitialize");
    ok &= resolve(egl, eglGetError, "eglGetError");
    ok &= resolve(egl, eglGetPlatformDisplay, "eglGetPlatformDisplay");
    ok &= resolve(egl, eglGetDisplay, "eglGetDisplay");
    ok &= resolve(egl, eglChooseConfig, "eglChooseConfig");
    ok &= resolve(egl, eglCreateContext, "eglCreateContext");
    ok &= resolve(egl, eglMakeCurrent, "eglMakeCurrent");
    ok &= resolve(egl, eglDestroyContext, "eglDestroyContext");
    ok &= resolve(egl, eglTerminate, "eglTerminate");
    ok &= resolve(egl, eglQueryString, "eglQueryString");
    ok &= resolve(egl, eglGetProcAddress, "eglGetProcAddress");

    // GLES core
    ok &= resolve(gles, glGetError, "glGetError");
    ok &= resolve(gles, glGetString, "glGetString");
    ok &= resolve(gles, glGetIntegerv, "glGetIntegerv");
    ok &= resolve(gles, glGetStringi, "glGetStringi");
    ok &= resolve(gles, glGenBuffers, "glGenBuffers");
    ok &= resolve(gles, glDeleteBuffers, "glDeleteBuffers");
    ok &= resolve(gles, glBindBuffer, "glBindBuffer");
    ok &= resolve(gles, glBufferData, "glBufferData");
    ok &= resolve(gles, glGenTextures, "glGenTextures");
    ok &= resolve(gles, glDeleteTextures, "glDeleteTextures");
    ok &= resolve(gles, glBindTexture, "glBindTexture");
    ok &= resolve(gles, glGenRenderbuffers, "glGenRenderbuffers");
    ok &= resolve(gles, glDeleteRenderbuffers, "glDeleteRenderbuffers");
    ok &= resolve(gles, glBindRenderbuffer, "glBindRenderbuffer");
    ok &= resolve(gles, glGenFramebuffers, "glGenFramebuffers");
    ok &= resolve(gles, glDeleteFramebuffers, "glDeleteFramebuffers");
    ok &= resolve(gles, glBindFramebuffer, "glBindFramebuffer");
    ok &= resolve(gles, glGenVertexArrays, "glGenVertexArrays");
    ok &= resolve(gles, glDeleteVertexArrays, "glDeleteVertexArrays");
    ok &= resolve(gles, glBindVertexArray, "glBindVertexArray");
    ok &= resolve(gles, glCreateShader, "glCreateShader");
    ok &= resolve(gles, glShaderSource, "glShaderSource");
    ok &= resolve(gles, glCompileShader, "glCompileShader");
    ok &= resolve(gles, glGetShaderiv, "glGetShaderiv");
    ok &= resolve(gles, glGetShaderInfoLog, "glGetShaderInfoLog");
    ok &= resolve(gles, glDeleteShader, "glDeleteShader");
    ok &= resolve(gles, glCreateProgram, "glCreateProgram");
    ok &= resolve(gles, glAttachShader, "glAttachShader");
    ok &= resolve(gles, glLinkProgram, "glLinkProgram");
    ok &= resolve(gles, glGetProgramiv, "glGetProgramiv");
    ok &= resolve(gles, glGetProgramInfoLog, "glGetProgramInfoLog");
    ok &= resolve(gles, glDeleteProgram, "glDeleteProgram");
    ok &= resolve(gles, glEnable, "glEnable");
    ok &= resolve(gles, glDisable, "glDisable");
    ok &= resolve(gles, glUseProgram, "glUseProgram");
    ok &= resolve(gles, glBlendFunc, "glBlendFunc");
    ok &= resolve(gles, glBlendEquation, "glBlendEquation");
    ok &= resolve(gles, glDepthFunc, "glDepthFunc");
    ok &= resolve(gles, glDepthMask, "glDepthMask");
    ok &= resolve(gles, glStencilFunc, "glStencilFunc");
    ok &= resolve(gles, glStencilOp, "glStencilOp");
    ok &= resolve(gles, glStencilMask, "glStencilMask");
    ok &= resolve(gles, glCullFace, "glCullFace");
    ok &= resolve(gles, glFrontFace, "glFrontFace");
    ok &= resolve(gles, glPixelStorei, "glPixelStorei");
    ok &= resolve(gles, glBindBufferBase, "glBindBufferBase");
    ok &= resolve(gles, glBindBufferRange, "glBindBufferRange");

    if (!ok) {
        dlclose(egl);
        dlclose(gles);
        loaded = false;
        return false;
    }
    loaded = true;
    return true;
}

void GLESLib::unload() {
    // The library handles are intentionally leaked across the process, but we
    // drop our resolved pointers so a re-init is clean. dlclose of EGL/GLES is
    // unsafe once contexts exist, so we keep the handles open.
    loaded = false;
}

} // namespace glcompat
