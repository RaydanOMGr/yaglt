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
    // eglGetPlatformDisplayEXT is the correct entry for platform-specific displays
    // such as surfaceless (EGL_PLATFORM_SURFACELESS_MESA). The core
    // eglGetPlatformDisplay routes the same platform enum through _eglFindDisplay,
    // which reads past a single-element attrib list and corrupts the stack; the
    // EXT entry handles the surfaceless platform cleanly. Resolved optionally so
    // load() still succeeds where only the core entry exists.
    EGLDisplay (*eglGetPlatformDisplayEXT)(EGLenum, void*, const EGLint*) = nullptr;
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
    void (*glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const void*) = nullptr;
    void (*glBufferStorage)(GLenum, GLsizeiptr, const void*, GLenum) = nullptr;
    void (*glNamedBufferData)(GLuint, GLsizeiptr, const void*, GLenum) = nullptr;
    void (*glNamedBufferSubData)(GLuint, GLintptr, GLsizeiptr, const void*) =
        nullptr;
    void (*glNamedBufferStorage)(GLuint, GLsizeiptr, const void*, GLbitfield) =
        nullptr;
    void (*glCopyBufferSubData)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr) = nullptr;
    void* (*glMapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield) = nullptr;
    GLboolean (*glUnmapBuffer)(GLenum) = nullptr;
    // DSA buffer mapping (SPEC §6.1, ES 3.1+). Optional: the frontend serves its
    // own CPU mirror, so a missing native symbol falls back to the target-based
    // path automatically.
    void* (*glMapNamedBufferRange)(GLuint, GLintptr, GLsizeiptr, GLbitfield) =
        nullptr;
    GLboolean (*glUnmapNamedBuffer)(GLuint) = nullptr;
    void (*glFlushMappedNamedBufferRange)(GLuint, GLintptr, GLsizeiptr) = nullptr;
    void (*glFlushMappedBufferRange)(GLenum, GLintptr, GLsizeiptr) = nullptr;
    // Buffer discard hints (SPEC §6, GLES 3.0+). Resolved optionally so load()
    // still succeeds on a driver that lacks them (the frontend still tracks its
    // CPU mirror regardless).
    void (*glInvalidateBufferData)(GLuint) = nullptr;
    void (*glInvalidateBufferSubData)(GLuint, GLintptr, GLsizeiptr) = nullptr;

    void (*glGenTextures)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteTextures)(GLsizei, const GLuint*) = nullptr;
    void (*glBindTexture)(GLenum, GLuint) = nullptr;
    void (*glActiveTexture)(GLenum) = nullptr;

    void (*glGenRenderbuffers)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteRenderbuffers)(GLsizei, const GLuint*) = nullptr;
    void (*glBindRenderbuffer)(GLenum, GLuint) = nullptr;
    void (*glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei) = nullptr;
    void (*glRenderbufferStorageMultisample)(GLenum, GLsizei, GLenum, GLsizei,
                                            GLsizei) = nullptr;

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
    void (*glDetachShader)(GLuint, GLuint) = nullptr;
    void (*glLinkProgram)(GLuint) = nullptr;
    void (*glGetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glDeleteProgram)(GLuint) = nullptr;
    // Program-interface reflection (SPEC §7.3.11, ES 3.0+). Resolved optionally.
    void (*glGetProgramInterfaceiv)(GLuint, GLenum, GLenum, GLint*) = nullptr;
    // Uniform-block binding (SPEC §7.6.2). ES 3.0+; resolved optionally.
    void (*glUniformBlockBinding)(GLuint, GLuint, GLuint) = nullptr;
    void (*glShaderStorageBlockBinding)(GLuint, GLuint, GLuint) = nullptr;
    // Transform-feedback varying capture setup (SPEC §13.3.1). ES 3.0+; resolved
    // optionally so load() still succeeds on drivers without it.
    void (*glTransformFeedbackVaryings)(GLuint, GLsizei, const GLchar* const*, GLenum) = nullptr;
    GLuint (*glGetProgramResourceIndex)(GLuint, GLenum, const GLchar*) = nullptr;
    void (*glGetProgramResourceName)(GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glGetProgramResourceiv)(GLuint, GLenum, GLuint, GLsizei, const GLenum*, GLsizei, GLsizei*, GLint*) = nullptr;
    GLint (*glGetProgramResourceLocation)(GLuint, GLenum, const GLchar*) = nullptr;
    GLint (*glGetProgramResourceLocationIndex)(GLuint, GLenum, const GLchar*) = nullptr;
    // Subroutines (SPEC §7.9, ES 3.1+). Resolved optionally.
    GLuint (*glGetSubroutineIndex)(GLuint, GLenum, const GLchar*) = nullptr;
    GLint (*glGetSubroutineUniformLocation)(GLuint, GLenum, const GLchar*) = nullptr;
    void (*glGetActiveSubroutineUniformiv)(GLuint, GLenum, GLuint, GLenum, GLint*) = nullptr;
    void (*glGetActiveSubroutineUniformName)(GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glGetActiveSubroutineName)(GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
    void (*glUniformSubroutinesuiv)(GLenum, GLsizei, const GLuint*) = nullptr;
    void (*glGetUniformSubroutineuiv)(GLenum, GLint, GLuint*) = nullptr;
    void (*glGetProgramStageiv)(GLuint, GLenum, GLenum, GLint*) = nullptr;

    // Pipeline state (SPEC §10): pushed by GLStateSink when the frontend
    // flushes tracked state to the driver.
    void (*glEnable)(GLenum) = nullptr;
    void (*glDisable)(GLenum) = nullptr;
    void (*glEnablei)(GLenum, GLuint) = nullptr;
    void (*glDisablei)(GLenum, GLuint) = nullptr;
    void (*glUseProgram)(GLuint) = nullptr;
    void (*glBlendFunc)(GLenum, GLenum) = nullptr;
    void (*glBlendEquation)(GLenum) = nullptr;
    void (*glBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum) = nullptr;
    void (*glBlendEquationSeparate)(GLenum, GLenum) = nullptr;
    void (*glBlendColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    // Indexed blending (SPEC §15.3 / §17.3.4, ARB_draw_buffers_blend, ES 3.2+).
    void (*glBlendFunci)(GLuint, GLenum, GLenum) = nullptr;
    void (*glBlendFuncSeparatei)(GLuint, GLenum, GLenum, GLenum, GLenum) = nullptr;
    void (*glBlendEquationi)(GLuint, GLenum) = nullptr;
    void (*glBlendEquationSeparatei)(GLuint, GLenum, GLenum) = nullptr;
    void (*glDepthFunc)(GLenum) = nullptr;
    void (*glDepthMask)(GLboolean) = nullptr;
    void (*glDepthRangef)(GLfloat, GLfloat) = nullptr;
    void (*glDepthRangefIndexed)(GLuint, GLfloat, GLfloat) = nullptr;
    void (*glStencilFunc)(GLenum, GLint, GLuint) = nullptr;
    void (*glStencilOp)(GLenum, GLenum, GLenum) = nullptr;
    void (*glStencilMask)(GLuint) = nullptr;
    void (*glStencilFuncSeparate)(GLenum, GLenum, GLint, GLuint) = nullptr;
    void (*glStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum) = nullptr;
    void (*glStencilMaskSeparate)(GLenum, GLuint) = nullptr;
    void (*glColorMask)(GLboolean, GLboolean, GLboolean, GLboolean) = nullptr;
    void (*glColorMaski)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean) =
        nullptr;
    void (*glDispatchCompute)(GLuint, GLuint, GLuint) = nullptr;
    void (*glDispatchComputeIndirect)(GLintptr) = nullptr;
    void (*glSampleCoverage)(GLfloat, GLboolean) = nullptr;
    void (*glPrimitiveRestartIndex)(GLuint) = nullptr;
    void (*glPatchParameteri)(GLenum, GLint) = nullptr;
    void (*glCullFace)(GLenum) = nullptr;
    void (*glFrontFace)(GLenum) = nullptr;
    void (*glPointSize)(GLfloat) = nullptr;
    void (*glHint)(GLenum, GLenum) = nullptr;
    void (*glLineWidth)(GLfloat) = nullptr;
    void (*glPolygonOffset)(GLfloat, GLfloat) = nullptr;
    void (*glPolygonOffsetClamp)(GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glPixelStorei)(GLenum, GLint) = nullptr;
    void (*glViewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;
    void (*glScissor)(GLint, GLint, GLsizei, GLsizei) = nullptr;
    void (*glViewportIndexedf)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glScissorIndexed)(GLuint, GLint, GLint, GLsizei, GLsizei) = nullptr;
    void (*glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*glClearDepthf)(GLfloat) = nullptr;
    void (*glClearStencil)(GLint) = nullptr;
    void (*glClear)(GLbitfield) = nullptr;
    // Texture clearing (SPEC §8.10). DSA; ES 3.0+. Resolved optionally so load()
    // still succeeds on a driver that lacks them (capability reports unsupported).
    void (*glClearTexImage)(GLuint, GLint, GLenum, GLenum, const void*) = nullptr;
    void (*glClearTexSubImage)(GLuint, GLint, GLint, GLint, GLint, GLsizei,
                               GLsizei, GLsizei, GLenum, GLenum, const void*) = nullptr;
    // Image-to-image copy (SPEC §8.21). Core in GLES 3.2; also exposed via
    // GL_EXT_copy_image / GL_OES_copy_image. Resolved optionally.
    void (*glCopyImageSubData)(GLuint, GLenum, GLint, GLint, GLint, GLint, GLuint,
                               GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei,
                               GLsizei) = nullptr;
    void (*glFlush)(void) = nullptr;
    void (*glFinish)(void) = nullptr;
    void (*glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,
                         void*) = nullptr;
    void (*glReadnPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei,
                           void*) = nullptr;
    // Texture barrier (SPEC §10.9.2). Core in GL 4.5; not a standard ES entry
    // point. GL_NV_texture_barrier exposes it as glTextureBarrierNV, resolved
    // optionally so load() still succeeds on drivers without the extension.
    void (*glTextureBarrierNV)(void) = nullptr;
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

    // Query objects (SPEC §4 / §19). Core in GLES 3.0+; resolved optionally so
    // load() still succeeds on a driver that lacks them (capability reports
    // unsupported).
    void (*glGenQueries)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteQueries)(GLsizei, const GLuint*) = nullptr;
    GLboolean (*glIsQuery)(GLuint) = nullptr;
    void (*glBeginQuery)(GLenum, GLuint) = nullptr;
    void (*glEndQuery)(GLenum) = nullptr;
    void (*glQueryCounter)(GLuint, GLenum) = nullptr;
    void (*glGetQueryiv)(GLenum, GLenum, GLint*) = nullptr;
    void (*glGetQueryObjectiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*glGetQueryObjectuiv)(GLuint, GLenum, GLuint*) = nullptr;
    void (*glGetQueryObjectui64v)(GLuint, GLenum, GLuint64*) = nullptr;

    // Shader precision query (SPEC §7.1). Core in GLES 2.0+; resolved optionally
    // so load() still succeeds on a driver that lacks it.
    void (*glGetShaderPrecisionFormat)(GLenum, GLenum, GLint*, GLint*) = nullptr;

    // Conditional rendering (SPEC §10.11). On GLES only available via
    // GL_NV_conditional_render; resolved optionally so load() still succeeds on
    // drivers that lack it (the capability system reports it Unsupported).
    void (*glBeginConditionalRenderNV)(GLuint, GLenum) = nullptr;
    void (*glEndConditionalRenderNV)(void) = nullptr;

    // Sampler objects (SPEC §8.2, GLES 3.0+). Resolved optionally so load()
    // still succeeds on a driver that lacks them (capability reports unsupported).
    void (*glGenSamplers)(GLsizei, GLuint*) = nullptr;
    void (*glDeleteSamplers)(GLsizei, const GLuint*) = nullptr;
    void (*glBindSampler)(GLuint, GLuint) = nullptr;
    void (*glBindImageTexture)(GLuint, GLuint, GLint, GLboolean, GLint, GLenum,
                               GLenum) = nullptr;
    void (*glSamplerParameteri)(GLuint, GLenum, GLint) = nullptr;
    void (*glSamplerParameterf)(GLuint, GLenum, GLfloat) = nullptr;
    void (*glSamplerParameterfv)(GLuint, GLenum, const GLfloat*) = nullptr;
    void (*glSamplerParameteriv)(GLuint, GLenum, const GLint*) = nullptr;
    void (*glSamplerParameterIiv)(GLuint, GLenum, const GLint*) = nullptr;
    void (*glSamplerParameterIuiv)(GLuint, GLenum, const GLuint*) = nullptr;
    GLboolean (*glIsSampler)(GLuint) = nullptr;

    void (*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,
                         GLenum, const void*) = nullptr;
    void (*glTexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint,
                         GLenum, GLenum, const void*) = nullptr;
    void (*glTexParameteri)(GLenum, GLenum, GLint) = nullptr;
    void (*glTexParameterf)(GLenum, GLenum, GLfloat) = nullptr;
    void (*glTexParameterfv)(GLenum, GLenum, const GLfloat*, GLsizei) = nullptr;
    void (*glTexParameteriv)(GLenum, GLenum, const GLint*, GLsizei) = nullptr;
    void (*glTexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum,
                            const void*) = nullptr;
    void (*glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum,
                            GLenum, const void*) = nullptr;
    void (*glTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei,
                            GLsizei, GLenum, GLenum, const void*) = nullptr;
    void (*glCopyTexImage1D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei,
                              GLint) = nullptr;
    void (*glCopyTexImage2D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei,
                              GLint) = nullptr;
    void (*glCopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei,
                                GLsizei) = nullptr;
    void (*glCopyTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint,
                                GLsizei, GLsizei) = nullptr;

    // Compressed texture upload (SPEC §8.6, GLES 3.0 core). Resolved optionally
    // so load() still succeeds on drivers that lack them; the frontend validates
    // and the backend forwards the call when present.
    void (*glCompressedTexImage1D)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei,
                                   const void*) = nullptr;
    void (*glCompressedTexImage2D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint,
                                   GLsizei, const void*) = nullptr;
    void (*glCompressedTexImage3D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei,
                                   GLint, GLsizei, const void*) = nullptr;
    void (*glCompressedTexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei,
                                      const void*) = nullptr;
    void (*glCompressedTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei,
                                      GLenum, GLsizei, const void*) = nullptr;
    void (*glCompressedTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei,
                                      GLsizei, GLsizei, GLenum, GLsizei,
                                      const void*) = nullptr;

    // Immutable texture storage + DSA helpers (SPEC §8.1, GL 4.2/4.5). Resolved
    // optionally so load() still succeeds on drivers that lack them (the
    // capability system reports immutable storage unsupported when absent).
    void (*glTexStorage1D)(GLenum, GLsizei, GLenum, GLsizei) = nullptr;
    void (*glTexStorage2D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei) = nullptr;
    void (*glTexStorage3D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei) = nullptr;
    void (*glTexStorage2DMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei,
                                      GLboolean) = nullptr;
    void (*glTexStorage3DMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei,
                                      GLsizei, GLboolean) = nullptr;
    void (*glTexImage2DMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei,
                                    GLboolean) = nullptr;
    void (*glTexImage3DMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei,
                                    GLsizei, GLboolean) = nullptr;
    void (*glGenerateMipmap)(GLenum) = nullptr;
    void (*glGetTexImage)(GLenum, GLint, GLenum, GLenum, void*) = nullptr;
    void (*glGetnTexImage)(GLenum, GLint, GLenum, GLenum, GLsizei, void*) = nullptr;
    void (*glGetnCompressedTexImage)(GLenum, GLint, GLsizei, void*) = nullptr;
    // Non-robust compressed read-back fallback (not in GLES core; resolved
    // optionally so load() still succeeds when the driver lacks it).
    void (*glGetCompressedTexImage)(GLenum, GLint, void*) = nullptr;
    void (*glGetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint*) = nullptr;
    void (*glGetTexLevelParameterfv)(GLenum, GLint, GLenum, GLfloat*) = nullptr;
    // Internal format queries (SPEC §22.3). GLES 3.0 core; resolved optionally so
    // load() still succeeds on a driver that lacks it.
    void (*glGetInternalformativ)(GLenum, GLenum, GLenum, GLsizei, GLint*) = nullptr;
    // Multisample sample-position query (SPEC §14.3.1 glGetMultisamplefv). GLES
    // 3.1+; resolved optionally.
    void (*glGetMultisamplefv)(GLenum, GLuint, GLfloat*) = nullptr;
    void (*glTexBuffer)(GLenum, GLenum, GLuint) = nullptr;
    void (*glTexBufferRange)(GLenum, GLenum, GLuint, GLintptr, GLsizeiptr) = nullptr;
    void (*glTextureView)(GLuint, GLenum, GLuint, GLenum, GLuint, GLuint, GLuint,
                          GLuint) = nullptr;

    // Integer texture parameters + texture invalidation (SPEC §8.1). ES 3.0+;
    // resolved optionally so load() still succeeds on drivers that lack them.
    void (*glTexParameterIiv)(GLenum, GLenum, const GLint*, GLsizei) = nullptr;
    void (*glTexParameterIuiv)(GLenum, GLenum, const GLuint*, GLsizei) = nullptr;
    void (*glInvalidateTexImage)(GLenum, GLint) = nullptr;
    void (*glInvalidateTexSubImage)(GLenum, GLint, GLint, GLint, GLint, GLsizei,
                                   GLsizei, GLsizei) = nullptr;

    void (*glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
    void (*glFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint) = nullptr;
    void (*glFramebufferTextureLayer)(GLenum, GLenum, GLuint, GLint,
                                     GLint) = nullptr;
    void (*glFramebufferParameteri)(GLenum, GLenum, GLint) = nullptr;
    // Framebuffer parameter read-back (SPEC §9.2.3). GLES 3.0+; resolved
    // optionally. Used to report the bound framebuffer's SAMPLES for
    // glGetMultisamplefv index validation.
    void (*glGetFramebufferParameteriv)(GLenum, GLenum, GLint*) = nullptr;
    GLenum (*glCheckFramebufferStatus)(GLenum) = nullptr;

    // Color logic op + framebuffer copy/invalidate (SPEC §15 / §16 / §17.3.4).
    // Core in GLES 3.0+; resolved optionally so load() still succeeds on a driver
    // that lacks them (capability reports unsupported).
    void (*glLogicOp)(GLenum) = nullptr;
    void (*glBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                              GLint, GLbitfield, GLenum) = nullptr;
    void (*glInvalidateFramebuffer)(GLenum, GLsizei, const GLenum*) = nullptr;
    void (*glInvalidateSubFramebuffer)(GLenum, GLsizei, const GLenum*, GLint,
                                       GLint, GLsizei, GLsizei) = nullptr;

    // Whole-framebuffer buffer selection (SPEC §15 / §16). Core in GLES 2.0+, but
    // resolved defensively so load() still succeeds when absent.
    void (*glDrawBuffers)(GLsizei, const GLenum*) = nullptr;
    void (*glReadBuffer)(GLenum) = nullptr;

    // Draw commands.
    void (*glDrawArrays)(GLenum, GLint, GLsizei) = nullptr;
    void (*glDrawElements)(GLenum, GLsizei, GLenum, const void*) = nullptr;
    void (*glDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei) = nullptr;
    void (*glDrawElementsInstanced)(GLenum, GLsizei, GLenum, const void*,
                                     GLsizei) = nullptr;
    // Base-instance draws (SPEC §10, ARB_base_instance, ES 3.2+). Resolved
    // optionally so load() still succeeds on drivers that lack them.
    void (*glDrawArraysInstancedBaseInstance)(GLenum, GLint, GLsizei, GLsizei,
                                              GLuint) = nullptr;
    void (*glDrawElementsInstancedBaseInstance)(GLenum, GLsizei, GLenum, const void*,
                                                GLsizei, GLuint) = nullptr;
    void (*glDrawElementsInstancedBaseVertexBaseInstance)(GLenum, GLsizei, GLenum,
                                                          const void*, GLsizei, GLint,
                                                          GLuint) = nullptr;
    // Draw expansion (SPEC §10). ES 3.0+ (multi-draw / range-elements) and
    // ES 3.2 (base-vertex); resolved optionally so load() still succeeds when a
    // driver lacks them (the capability system reports them unsupported).
    void (*glMultiDrawArrays)(GLenum, const GLint*, const GLsizei*, GLsizei) = nullptr;
    void (*glMultiDrawElements)(GLenum, const GLsizei*, GLenum, const void* const*,
                                GLsizei) = nullptr;
    void (*glMultiDrawArraysBaseInstance)(GLenum, const GLint*, const GLsizei*,
                                          const GLsizei*, const GLuint*,
                                          GLsizei) = nullptr;
    void (*glMultiDrawElementsBaseInstance)(GLenum, const GLsizei*, GLenum,
                                           const void* const*, GLsizei,
                                           const GLuint*) = nullptr;
    void (*glDrawRangeElements)(GLenum, GLuint, GLuint, GLsizei, GLenum,
                                const void*) = nullptr;
    void (*glDrawElementsBaseVertex)(GLenum, GLsizei, GLenum, const void*,
                                     GLint) = nullptr;
    // Base-vertex draw variants (SPEC §10, GL 3.2 / ES 3.2). Resolved optionally so
    // load() still succeeds on a driver that lacks them (the capability system
    // reports DrawElementsBaseVertex unsupported instead of failing backend init).
    void (*glDrawElementsInstancedBaseVertex)(GLenum, GLsizei, GLenum, const void*,
                                              GLsizei, GLint) = nullptr;
    void (*glDrawRangeElementsBaseVertex)(GLenum, GLuint, GLuint, GLsizei, GLenum,
                                          const void*, GLint) = nullptr;
    void (*glMultiDrawElementsBaseVertex)(GLenum, const GLsizei*, GLenum,
                                          const void* const*, GLsizei,
                                          GLint) = nullptr;
    // Indirect draw (SPEC §10, ES 3.1+). Resolved optionally so load() still
    // succeeds on a driver that lacks them (the capability system reports
    // IndirectDrawing unsupported instead of failing the whole backend init).
    void (*glDrawArraysIndirect)(GLenum, const void*) = nullptr;
    void (*glDrawElementsIndirect)(GLenum, GLenum, const void*) = nullptr;
    // Multi-draw indirect (SPEC §10, ARB_multi_draw_indirect, ES 3.1+). Resolved
    // optionally so load() still succeeds on a driver that lacks them.
    void (*glMultiDrawArraysIndirect)(GLenum, const void*, GLsizei, GLsizei) = nullptr;
    void (*glMultiDrawElementsIndirect)(GLenum, GLenum, const void*, GLsizei,
                                       GLsizei) = nullptr;
    // Multi-draw indirect, count from a buffer (SPEC §10.4, GL 4.6, desktop only).
    // No native GLES equivalent exists, so these are resolved optionally and remain
    // null on every GLES driver; the backend drops the call when absent.
    void (*glMultiDrawArraysIndirectCount)(GLenum, const void*, GLintptr, GLsizei,
                                          GLsizei) = nullptr;
    void (*glMultiDrawElementsIndirectCount)(GLenum, GLenum, const void*, GLintptr,
                                           GLsizei, GLsizei) = nullptr;
    // Transform-feedback draws (SPEC §13.3.3, ES 3.2+). Resolved optionally so
    // load() still succeeds on a driver that lacks them.
    void (*glDrawTransformFeedback)(GLenum, GLuint) = nullptr;
    void (*glDrawTransformFeedbackInstanced)(GLenum, GLuint, GLsizei) = nullptr;
    void (*glDrawTransformFeedbackStream)(GLenum, GLuint, GLuint) = nullptr;
    void (*glDrawTransformFeedbackStreamInstanced)(GLenum, GLuint, GLuint,
                                                  GLsizei) = nullptr;

    // Vertex attributes (SPEC §2.1).
    GLint (*glGetAttribLocation)(GLuint, const GLchar*) = nullptr;
    void (*glBindAttribLocation)(GLuint, GLuint, const GLchar*) = nullptr;
    void (*glEnableVertexAttribArray)(GLuint) = nullptr;
    void (*glDisableVertexAttribArray)(GLuint) = nullptr;
    void (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                  const void*) = nullptr;
    void (*glVertexAttribDivisor)(GLuint, GLuint) = nullptr;

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
    void (*glUniform2fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform3fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform4fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*glUniform1iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform2iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform3iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform4iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*glUniform1ui)(GLint, GLuint) = nullptr;
    void (*glUniform2ui)(GLint, GLuint, GLuint) = nullptr;
    void (*glUniform3ui)(GLint, GLuint, GLuint, GLuint) = nullptr;
    void (*glUniform4ui)(GLint, GLuint, GLuint, GLuint, GLuint) = nullptr;
    void (*glUniform1uiv)(GLint, GLsizei, const GLuint*) = nullptr;
    void (*glUniform2uiv)(GLint, GLsizei, const GLuint*) = nullptr;
    void (*glUniform3uiv)(GLint, GLsizei, const GLuint*) = nullptr;
    void (*glUniform4uiv)(GLint, GLsizei, const GLuint*) = nullptr;
    void (*glUniformMatrix2fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix2x3fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix2x4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix3x2fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix3x4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix4x2fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glUniformMatrix4x3fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*glGetUniformfv)(GLuint, GLint, GLfloat*) = nullptr;
    void (*glGetUniformiv)(GLuint, GLint, GLint*) = nullptr;
    void (*glGetUniformuiv)(GLuint, GLint, GLuint*) = nullptr;

    // Tracks the driver program bound by this loader so uniform calls can avoid
    // redundant glUseProgram (SPEC §10: skip unchanged native state).
    GLuint currentProgram = 0;

    // True only when every required symbol resolved.
    bool loaded = false;

    // True between GLES context creation and backend shutdown. Resource
    // destructors consult this so they never issue driver calls on a context
    // that has already been torn down (which is undefined and can corrupt
    // driver heap state, e.g. glDelete* after eglTerminate). The frontend owns
    // object lifetime and normally deletes resources before the backend shuts
    // down, but a resource that outlives shutdown must not call the driver.
    bool contextAlive = false;

    // Safe to issue driver calls: symbols resolved AND a live context exists.
    bool driverLive() const { return loaded && contextAlive; }

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
