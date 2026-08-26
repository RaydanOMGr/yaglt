#pragma once

// Dynamic loader for EGL + OpenGL ES. We resolve symbols at runtime (dlopen)
// rather than linking libEGL/libGLESv2, so YAGLT builds on hosts that lack the
// dev libraries (and on Android where the drivers are present at runtime). If
// the libraries or required symbols are absent, load() returns false and the
// GLES backend reports itself as unavailable honestly.

#include <EGL/egl.h>
#include <GLES3/gl32.h>

#include <memory>
#include <string>

namespace glcompat {

// One entry point set per GLES backend instance.
struct GLESLib {
    // EGL
    EGLBoolean (*eglInitialize)(EGLDisplay, EGLint*, EGLint*) = nullptr;
    EGLint (*eglGetError)(void) = nullptr;
    EGLDisplay (*eglGetPlatformDisplay)(EGLenum, void*, const EGLint*) = nullptr;
    EGLDisplay (*eglGetDisplay)(EGLNativeDisplayType) = nullptr;
    EGLBoolean (*eglChooseConfig)(EGLDisplay, const EGLint*, EGLConfig*,
                                  EGLint, EGLint*) = nullptr;
    EGLContext (*eglCreateContext)(EGLDisplay, EGLConfig, EGLContext,
                                   const EGLint*) = nullptr;
    EGLBoolean (*eglMakeCurrent)(EGLDisplay, EGLSurface, EGLSurface,
                                 EGLContext) = nullptr;
    EGLBoolean (*eglDestroyContext)(EGLDisplay, EGLContext) = nullptr;
    EGLBoolean (*eglTerminate)(EGLDisplay) = nullptr;
    const char* (*eglQueryString)(EGLDisplay, EGLint) = nullptr;
    void* (*eglGetProcAddress)(const char*) = nullptr;

    // GLES (core subset used by the foundation)
    GLenum (*glGetError)(void) = nullptr;
    const GLubyte* (*glGetString)(GLenum) = nullptr;
    void (*glGetIntegerv)(GLenum, GLint*) = nullptr;
    void (*glGetStringi)(GLenum, GLuint, const GLubyte**) = nullptr;

    void (*glGenBuffers)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteBuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glBindBuffer)(GLenum, GLuint) = nullptr;
    void (*glBufferData)(GLenum, GLsizeiptr, const void*, GLenum) = nullptr;

    void (*glGenTextures)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteTextures)(GLsizei, const GLuint*) = nullptr;
    void (*glBindTexture)(GLenum, GLuint) = nullptr;

    void (*glGenRenderbuffers)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteRenderbuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glBindRenderbuffer)(GLenum, GLuint) = nullptr;

    void (*glGenFramebuffers)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteFramebuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glBindFramebuffer)(GLenum, GLuint) = nullptr;

    void (*glGenVertexArrays)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteVertexArrays)(GLsizei, const GLuint*) = nullptr;
    void (*glBindVertexArray)(GLuint) = nullptr;

    GLuint (*glCreateShader)(GLenum) = nullptr;
    void (*glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
    void (*glCompileShader)(GLuint) = nullptr;
    void (*glGetShaderiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glDeleteShader)(GLuint) = nullptr;

    GLuint (*glCreateProgram)(void) = nullptr;
    void (*glAttachShader)(GLuint, GLuint) = nullptr;
    void (*glLinkProgram)(GLuint) = nullptr;
    void (*glGetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glDeleteProgram)(GLuint) = nullptr;

    // Pipeline state (SPEC §10): pushed by GLStateSink when the frontend
    // flushes tracked state to the driver.
    void (*glEnable)(GLenum) = nullptr;
    void (*glDisable)(GLenum) = nullptr;
    void (*glUseProgram)(GLuint) = nullptr;
    void (*glBlendFunc)(GLenum, GLenum) = nullptr;
    void (*glBlendEquation)(GLenum) = nullptr;
    void (*glDepthFunc)(GLenum) = nullptr;
    void (*glDepthMask)(GLboolean) = nullptr;
    void (*glStencilFunc)(GLenum, GLint, GLuint) = nullptr;
    void (*glStencilOp)(GLenum, GLenum, GLenum) = nullptr;
    void (*glStencilMask)(GLuint) = nullptr;
    void (*glCullFace)(GLenum) = nullptr;
    void (*glFrontFace)(GLenum) = nullptr;
    void (*glPixelStorei)(GLenum, GLint) = nullptr;
    void (*glBindBufferBase)(GLenum, GLuint, GLuint) = nullptr;
    void (*glBindBufferRange)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr) = nullptr;

    // Draw commands.
    void (*glDrawArrays)(GLenum, GLint, GLsizei) = nullptr;
    void (*glDrawElements)(GLenum, GLsizei, GLenum, const void*) = nullptr;
    void (*glDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei) = nullptr;
    void (*glDrawElementsInstanced)(GLenum, GLsizei, GLenum, const void*,
                                    GLsizei) = nullptr;

    // True only when every required symbol resolved.
    bool loaded = false;

    // Opens libEGL / libGLESv2 (trying a few common sonames) and resolves all
    // of the above. Returns false (and leaves loaded=false) if unavailable.
    bool load();
    void unload();

    // GLES major version parsed from GL_VERSION (0 if unknown).
    int glesMajor = 0;
    int glesMinor = 0;
    std::string versionString;
    std::string rendererString;
    std::string extensionsString; // space-joined (GLES2 style)
};

using GLESLibPtr = std::shared_ptr<GLESLib>;

} // namespace glcompat
