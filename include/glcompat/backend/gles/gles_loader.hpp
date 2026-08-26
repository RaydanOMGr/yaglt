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
    void (*glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei) = nullptr;

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
    void (*glDepthRangef)(GLfloat, GLfloat) = nullptr;
    void (*glStencilFunc)(GLenum, GLint, GLuint) = nullptr;
    void (*glStencilOp)(GLenum, GLenum, GLenum) = nullptr;
    void (*glStencilMask)(GLuint) = nullptr;
    void (*glCullFace)(GLenum) = nullptr;
    void (*glFrontFace)(GLenum) = nullptr;
    void (*glPixelStorei)(GLenum, GLint) = nullptr;
    void (*glViewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;
    void (*glScissor)(GLint, GLint, GLsizei, GLsizei) = nullptr;
    void (*glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glClearDepthf)(GLfloat) = nullptr;
    void (*glClear)(GLbitfield) = nullptr;
    void (*glFlush)(void) = nullptr;
    void (*glFinish)(void) = nullptr;
    void (*glBindBufferBase)(GLenum, GLuint, GLuint) = nullptr;
    void (*glBindBufferRange)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr) = nullptr;

    // Transform feedback (SPEC §13.3). ES 3.0+; resolved optionally so load()
    // still succeeds on a driver that lacks them (capability reports unsupported).
    void (*glGenTransformFeedbacks)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteTransformFeedbacks)(GLsizei, const GLuint*) = nullptr;
    void (*glBindTransformFeedback)(GLenum, GLuint) = nullptr;
    void (*glBeginTransformFeedback)(GLenum) = nullptr;
    void (*glEndTransformFeedback)(void) = nullptr;
    void (*glPauseTransformFeedback)(void) = nullptr;
    void (*glResumeTransformFeedback)(void) = nullptr;

    void (*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,
                        GLenum, const void*) = nullptr;
    void (*glTexParameteri)(GLenum, GLenum, GLint) = nullptr;
    void (*glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
    void (*glFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint) = nullptr;
    GLenum (*glCheckFramebufferStatus)(GLenum) = nullptr;

    // Draw commands.
    void (*glDrawArrays)(GLenum, GLint, GLsizei) = nullptr;
    void (*glDrawElements)(GLenum, GLsizei, GLenum, const void*) = nullptr;
    void (*glDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei) = nullptr;
    void (*glDrawElementsInstanced)(GLenum, GLsizei, GLenum, const void*,
                                    GLsizei) = nullptr;

    // Vertex attributes (SPEC §2.1).
    GLint (*glGetAttribLocation)(GLuint, const GLchar*) = nullptr;
    void (*glBindAttribLocation)(GLuint, GLuint, const GLchar*) = nullptr;
    void (*glEnableVertexAttribArray)(GLuint) = nullptr;
    void (*glDisableVertexAttribArray)(GLuint) = nullptr;
    void (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                  const void*) = nullptr;

    // Uniforms (SPEC §8). Resolved for ES 2.0+ drivers; absent on a driver that
    // lacks them they stay null and the backend reports unsupported honestly.
    GLint (*glGetUniformLocation)(GLuint, const GLchar*) = nullptr;
    void (*glUniform1f)(GLint, GLfloat) = nullptr;
    void (*glUniform2f)(GLint, GLfloat, GLfloat) = nullptr;
    void (*glUniform3f)(GLint, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glUniform1i)(GLint, GLint) = nullptr;
    void (*glUniform2i)(GLint, GLint, GLint) = nullptr;
    void (*glUniform3i)(GLint, GLint, GLint, GLint) = nullptr;
    void (*glUniform4i)(GLint, GLint, GLint, GLint, GLint) = nullptr;
    void (*glUniform1fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform1iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;

    // Tracks the driver program bound by this loader so uniform calls can avoid
    // redundant glUseProgram (SPEC §10: skip unchanged native state).
    GLuint currentProgram = 0;

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
