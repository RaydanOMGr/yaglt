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
    resolve(egl, eglGetPlatformDisplayEXT, "eglGetPlatformDisplayEXT");
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
    // Buffer sub-data / immutable storage / copy are ES 3.1+; resolved optionally
    // so load() still succeeds on a driver that lacks them (capability reports
    // unsupported / the frontend emulates via its CPU mirror).
    resolve(gles, glBufferSubData, "glBufferSubData");
    resolve(gles, glBufferStorage, "glBufferStorage");
    resolve(gles, glCopyBufferSubData, "glCopyBufferSubData");
    // DSA buffer allocation (ES 3.1+, SPEC §6.1/§6.2). Optional: the frontend's
    // target-based path binds then allocates when these are absent.
    resolve(gles, glNamedBufferData, "glNamedBufferData");
    resolve(gles, glNamedBufferSubData, "glNamedBufferSubData");
    resolve(gles, glNamedBufferStorage, "glNamedBufferStorage");
    resolve(gles, glMapBufferRange, "glMapBufferRange");
    resolve(gles, glUnmapBuffer, "glUnmapBuffer");
    // DSA buffer mapping (ES 3.1+, SPEC §6.1). Optional.
    resolve(gles, glMapNamedBufferRange, "glMapNamedBufferRange");
    resolve(gles, glUnmapNamedBuffer, "glUnmapNamedBuffer");
    resolve(gles, glFlushMappedNamedBufferRange, "glFlushMappedNamedBufferRange");
    resolve(gles, glFlushMappedBufferRange, "glFlushMappedBufferRange");
    resolve(gles, glInvalidateBufferData, "glInvalidateBufferData");
    resolve(gles, glInvalidateBufferSubData, "glInvalidateBufferSubData");
    ok &= resolve(gles, glGenTextures, "glGenTextures");
    ok &= resolve(gles, glDeleteTextures, "glDeleteTextures");
    ok &= resolve(gles, glBindTexture, "glBindTexture");
    resolve(gles, glActiveTexture, "glActiveTexture");
    ok &= resolve(gles, glGenRenderbuffers, "glGenRenderbuffers");
    ok &= resolve(gles, glDeleteRenderbuffers, "glDeleteRenderbuffers");
    ok &= resolve(gles, glBindRenderbuffer, "glBindRenderbuffer");
    ok &= resolve(gles, glRenderbufferStorage, "glRenderbufferStorage");
    ok &= resolve(gles, glRenderbufferStorageMultisample,
                  "glRenderbufferStorageMultisample");
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
    ok &= resolve(gles, glDetachShader, "glDetachShader");
    ok &= resolve(gles, glLinkProgram, "glLinkProgram");
    ok &= resolve(gles, glGetProgramiv, "glGetProgramiv");
    ok &= resolve(gles, glGetProgramInfoLog, "glGetProgramInfoLog");
    ok &= resolve(gles, glDeleteProgram, "glDeleteProgram");
    // Program-interface reflection (ES 3.0+); resolved optionally so load()
    // still succeeds on stricter/driver-limited EGL stacks.
    resolve(gles, glGetProgramInterfaceiv, "glGetProgramInterfaceiv");
    resolve(gles, glGetProgramResourceIndex, "glGetProgramResourceIndex");
    resolve(gles, glGetProgramResourceName, "glGetProgramResourceName");
    resolve(gles, glGetProgramResourceiv, "glGetProgramResourceiv");
    resolve(gles, glGetProgramResourceLocation, "glGetProgramResourceLocation");
    resolve(gles, glGetProgramResourceLocationIndex, "glGetProgramResourceLocationIndex");
    // Subroutines (ES 3.1+); resolved optionally so load() still succeeds on
    // stricter/driver-limited EGL stacks.
    resolve(gles, glGetSubroutineIndex, "glGetSubroutineIndex");
    resolve(gles, glGetSubroutineUniformLocation, "glGetSubroutineUniformLocation");
    resolve(gles, glGetActiveSubroutineUniformiv, "glGetActiveSubroutineUniformiv");
    resolve(gles, glGetActiveSubroutineUniformName, "glGetActiveSubroutineUniformName");
    resolve(gles, glGetActiveSubroutineName, "glGetActiveSubroutineName");
    resolve(gles, glUniformSubroutinesuiv, "glUniformSubroutinesuiv");
    resolve(gles, glGetUniformSubroutineuiv, "glGetUniformSubroutineuiv");
    resolve(gles, glGetProgramStageiv, "glGetProgramStageiv");
    ok &= resolve(gles, glEnable, "glEnable");
    ok &= resolve(gles, glDisable, "glDisable");
    resolve(gles, glEnablei, "glEnablei");
    resolve(gles, glDisablei, "glDisablei");
    ok &= resolve(gles, glUseProgram, "glUseProgram");
    ok &= resolve(gles, glBlendFunc, "glBlendFunc");
    ok &= resolve(gles, glBlendEquation, "glBlendEquation");
    // Optional (core in ES 2.0/3.x but resolved defensively): separate blend and
    // constant blend color. The backend falls back to glBlendFunc/glBlendEquation
    // when these are unavailable.
    resolve(gles, glBlendFuncSeparate, "glBlendFuncSeparate");
    resolve(gles, glBlendEquationSeparate, "glBlendEquationSeparate");
    resolve(gles, glBlendColor, "glBlendColor");
    // Indexed blending (SPEC §15.3 / §17.3.4). Optional: ES 3.2+ / drivers with
    // ARB_draw_buffers_blend; resolved defensively so older drivers stay usable.
    resolve(gles, glBlendFunci, "glBlendFunci");
    resolve(gles, glBlendFuncSeparatei, "glBlendFuncSeparatei");
    resolve(gles, glBlendEquationi, "glBlendEquationi");
    resolve(gles, glBlendEquationSeparatei, "glBlendEquationSeparatei");
    ok &= resolve(gles, glDepthFunc, "glDepthFunc");
    ok &= resolve(gles, glDepthMask, "glDepthMask");
    ok &= resolve(gles, glDepthRangef, "glDepthRangef");
    ok &= resolve(gles, glDepthRangefIndexed, "glDepthRangefIndexed");
    ok &= resolve(gles, glStencilFunc, "glStencilFunc");
    ok &= resolve(gles, glStencilOp, "glStencilOp");
    ok &= resolve(gles, glStencilMask, "glStencilMask");
    ok &= resolve(gles, glStencilFuncSeparate, "glStencilFuncSeparate");
    ok &= resolve(gles, glStencilOpSeparate, "glStencilOpSeparate");
    ok &= resolve(gles, glStencilMaskSeparate, "glStencilMaskSeparate");
    ok &= resolve(gles, glColorMask, "glColorMask");
    ok &= resolve(gles, glColorMaski, "glColorMaski");
    resolve(gles, glDispatchCompute, "glDispatchCompute");
    resolve(gles, glDispatchComputeIndirect, "glDispatchComputeIndirect");
    ok &= resolve(gles, glSampleCoverage, "glSampleCoverage");
    // glPrimitiveRestartIndex is core in GLES 3.0 but some implementations expose
    // it conditionally; resolve it optionally so load() still succeeds without it.
    resolve(gles, glPrimitiveRestartIndex, "glPrimitiveRestartIndex");
    // glPatchParameteri is core in GLES 3.2; resolve it optionally so load()
    // still succeeds on ES 3.0/3.1 drivers that omit it.
    resolve(gles, glPatchParameteri, "glPatchParameteri");
    ok &= resolve(gles, glCullFace, "glCullFace");
    ok &= resolve(gles, glFrontFace, "glFrontFace");
    // glPointSize was removed from the OpenGL ES 3.0 API (point size is set via
    // the gl_PointSize vertex-shader builtin). Resolve it optionally so load()
    // still succeeds on ES 3.0+ drivers that omit it; the frontend keeps tracking
    // the state (SPEC §10) and GLESBackend::pointSize no-ops when absent.
    resolve(gles, glPointSize, "glPointSize");
    // glHint is core in GLES but a no-op on most drivers; resolved optionally so
    // load() still succeeds where the symbol is absent.
    resolve(gles, glHint, "glHint");
    ok &= resolve(gles, glLineWidth, "glLineWidth");
    ok &= resolve(gles, glPolygonOffset, "glPolygonOffset");
    ok &= resolve(gles, glPolygonOffsetClamp, "glPolygonOffsetClamp");
    ok &= resolve(gles, glPixelStorei, "glPixelStorei");
    ok &= resolve(gles, glViewport, "glViewport");
    ok &= resolve(gles, glScissor, "glScissor");
    ok &= resolve(gles, glViewportIndexedf, "glViewportIndexedf");
    ok &= resolve(gles, glScissorIndexed, "glScissorIndexed");
    ok &= resolve(gles, glClearColor, "glClearColor");
    ok &= resolve(gles, glClearDepthf, "glClearDepthf");
    ok &= resolve(gles, glClearStencil, "glClearStencil");
    ok &= resolve(gles, glClear, "glClear");
    resolve(gles, glClearTexImage, "glClearTexImage");
    resolve(gles, glClearTexSubImage, "glClearTexSubImage");
    resolve(gles, glCopyImageSubData, "glCopyImageSubData");
    ok &= resolve(gles, glFlush, "glFlush");
    ok &= resolve(gles, glFinish, "glFinish");
    ok &= resolve(gles, glReadPixels, "glReadPixels");
    // glReadnPixels is core in GL 4.5 (ARB_robustness) but not a standard ES entry
    // point; resolve it optionally so load() still succeeds on ES drivers that
    // omit it, and GLESBackend::readnPixels falls back to glReadPixels.
    resolve(gles, glReadnPixels, "glReadnPixels");
    // glTextureBarrierNV (GL_NV_texture_barrier) is the ES spelling of
    // glTextureBarrier; resolve optionally.
    resolve(gles, glTextureBarrierNV, "glTextureBarrierNV");
    ok &= resolve(gles, glBindBufferBase, "glBindBufferBase");
    ok &= resolve(gles, glBindBufferRange, "glBindBufferRange");
    ok &= resolve(gles, glUniformBlockBinding, "glUniformBlockBinding");
    resolve(gles, glShaderStorageBlockBinding, "glShaderStorageBlockBinding");
    // Transform-feedback varying capture setup (SPEC §13.3.1). ES 3.0+; resolved
    // optionally so load() still succeeds without it.
    resolve(gles, glTransformFeedbackVaryings, "glTransformFeedbackVaryings");
    ok &= resolve(gles, glTexImage2D, "glTexImage2D");
    ok &= resolve(gles, glTexImage3D, "glTexImage3D");
    ok &= resolve(gles, glTexParameteri, "glTexParameteri");
    // Parameter setters (float / vector) are core in GLES but resolved defensively.
    resolve(gles, glTexParameterf, "glTexParameterf");
    resolve(gles, glTexParameterfv, "glTexParameterfv");
    resolve(gles, glTexParameteriv, "glTexParameteriv");
    // Sub-image / copy-from-framebuffer are core in GLES but resolved defensively.
    resolve(gles, glTexSubImage1D, "glTexSubImage1D");
    resolve(gles, glTexSubImage2D, "glTexSubImage2D");
    resolve(gles, glTexSubImage3D, "glTexSubImage3D");
    resolve(gles, glCopyTexImage1D, "glCopyTexImage1D");
    resolve(gles, glCopyTexImage2D, "glCopyTexImage2D");
    resolve(gles, glCopyTexSubImage2D, "glCopyTexSubImage2D");
    resolve(gles, glCopyTexSubImage3D, "glCopyTexSubImage3D");
    // Compressed texture upload (SPEC §8.6, GLES 3.0 core).
    resolve(gles, glCompressedTexImage1D, "glCompressedTexImage1D");
    resolve(gles, glCompressedTexImage2D, "glCompressedTexImage2D");
    resolve(gles, glCompressedTexImage3D, "glCompressedTexImage3D");
    resolve(gles, glCompressedTexSubImage1D, "glCompressedTexSubImage1D");
    resolve(gles, glCompressedTexSubImage2D, "glCompressedTexSubImage2D");
    resolve(gles, glCompressedTexSubImage3D, "glCompressedTexSubImage3D");
    // Immutable texture storage + DSA helpers (GL 4.2/4.5). Resolved optionally so
    // load() still succeeds on drivers that lack them.
    resolve(gles, glTexStorage1D, "glTexStorage1D");
    resolve(gles, glTexStorage2D, "glTexStorage2D");
    resolve(gles, glTexStorage3D, "glTexStorage3D");
    resolve(gles, glTexStorage2DMultisample, "glTexStorage2DMultisample");
    resolve(gles, glTexStorage3DMultisample, "glTexStorage3DMultisample");
    resolve(gles, glTexImage2DMultisample, "glTexImage2DMultisample");
    resolve(gles, glTexImage3DMultisample, "glTexImage3DMultisample");
    resolve(gles, glGenerateMipmap, "glGenerateMipmap");
    resolve(gles, glGetTexImage, "glGetTexImage");
    // Robustness read-back (ARB_robustness / GL 4.5). Resolved optionally: a
    // driver without these entries still loads; GLESBackend falls back to the
    // non-robust read at runtime.
    resolve(gles, glGetnTexImage, "glGetnTexImage");
    resolve(gles, glGetnCompressedTexImage, "glGetnCompressedTexImage");
    resolve(gles, glGetCompressedTexImage, "glGetCompressedTexImage");
    resolve(gles, glGetTexLevelParameteriv, "glGetTexLevelParameteriv");
    resolve(gles, glGetTexLevelParameterfv, "glGetTexLevelParameterfv");
    resolve(gles, glGetInternalformativ, "glGetInternalformativ");
    // Multisample sample-position query (SPEC §14.3.1 glGetMultisamplefv). GLES
    // 3.1+; resolved optionally so load() still succeeds on a driver that lacks
    // it (the frontend reports the entry point unsupported-dependently via error).
    resolve(gles, glGetMultisamplefv, "glGetMultisamplefv");
    resolve(gles, glTexBuffer, "glTexBuffer");
    resolve(gles, glTexBufferRange, "glTexBufferRange");
    // Texture views (SPEC §8.19 glTextureView) are ES 3.1+; resolved optionally so
    // load() still succeeds on drivers that lack them (capability reports views
    // unsupported, the frontend rejects the call with GL_INVALID_OPERATION).
    resolve(gles, glTextureView, "glTextureView");
    // Integer texture parameters + texture invalidation (SPEC §8.1, ES 3.0+).
    resolve(gles, glTexParameterIiv, "glTexParameterIiv");
    resolve(gles, glTexParameterIuiv, "glTexParameterIuiv");
    resolve(gles, glInvalidateTexImage, "glInvalidateTexImage");
    resolve(gles, glInvalidateTexSubImage, "glInvalidateTexSubImage");
    ok &= resolve(gles, glFramebufferTexture2D, "glFramebufferTexture2D");
    ok &= resolve(gles, glFramebufferRenderbuffer, "glFramebufferRenderbuffer");
    resolve(gles, glFramebufferTextureLayer, "glFramebufferTextureLayer");
    resolve(gles, glFramebufferParameteri, "glFramebufferParameteri");
    // Framebuffer parameter read-back (SPEC §9.2.3). GLES 3.0+; resolved
    // optionally. Used to read the bound framebuffer's SAMPLES for
    // glGetMultisamplefv index validation.
    resolve(gles, glGetFramebufferParameteriv, "glGetFramebufferParameteriv");
    ok &= resolve(gles, glCheckFramebufferStatus, "glCheckFramebufferStatus");
    // Whole-framebuffer buffer selection (core in GLES but resolved defensively).
    resolve(gles, glDrawBuffers, "glDrawBuffers");
    resolve(gles, glReadBuffer, "glReadBuffer");
    ok &= resolve(gles, glDrawArrays, "glDrawArrays");
    ok &= resolve(gles, glDrawElements, "glDrawElements");
    // Instanced draws are ES 3.0+; resolve optionally so load() still succeeds
    // on a driver that lacks them (the capability system reports them
    // unsupported instead of failing the whole backend init).
    resolve(gles, glDrawArraysInstanced, "glDrawArraysInstanced");
    resolve(gles, glDrawElementsInstanced, "glDrawElementsInstanced");
    // Draw expansion (SPEC §10): solved optionally (ES 3.0+ / ES 3.2).
    resolve(gles, glMultiDrawArrays, "glMultiDrawArrays");
    resolve(gles, glMultiDrawElements, "glMultiDrawElements");
    resolve(gles, glMultiDrawArraysBaseInstance, "glMultiDrawArraysBaseInstance");
    resolve(gles, glMultiDrawElementsBaseInstance, "glMultiDrawElementsBaseInstance");
    resolve(gles, glDrawRangeElements, "glDrawRangeElements");
    resolve(gles, glDrawElementsBaseVertex, "glDrawElementsBaseVertex");
    resolve(gles, glDrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertex");
    resolve(gles, glDrawRangeElementsBaseVertex, "glDrawRangeElementsBaseVertex");
    resolve(gles, glMultiDrawElementsBaseVertex, "glMultiDrawElementsBaseVertex");
    // Base-instance draws (SPEC §10, ES 3.2+); resolved optionally.
    resolve(gles, glDrawArraysInstancedBaseInstance, "glDrawArraysInstancedBaseInstance");
    resolve(gles, glDrawElementsInstancedBaseInstance, "glDrawElementsInstancedBaseInstance");
    resolve(gles, glDrawElementsInstancedBaseVertexBaseInstance,
            "glDrawElementsInstancedBaseVertexBaseInstance");
    // Indirect draw (SPEC §10, ES 3.1+); resolved optionally.
    resolve(gles, glDrawArraysIndirect, "glDrawArraysIndirect");
    resolve(gles, glDrawElementsIndirect, "glDrawElementsIndirect");
    // Multi-draw indirect (SPEC §10, ES 3.1+); resolved optionally.
    resolve(gles, glMultiDrawArraysIndirect, "glMultiDrawArraysIndirect");
    resolve(gles, glMultiDrawElementsIndirect, "glMultiDrawElementsIndirect");
    // Transform-feedback draws (SPEC §13.3.3, ES 3.2+); resolved optionally.
    resolve(gles, glDrawTransformFeedback, "glDrawTransformFeedback");
    resolve(gles, glDrawTransformFeedbackInstanced, "glDrawTransformFeedbackInstanced");
    resolve(gles, glDrawTransformFeedbackStream, "glDrawTransformFeedbackStream");
    resolve(gles, glDrawTransformFeedbackStreamInstanced, "glDrawTransformFeedbackStreamInstanced");

    // Transform feedback (ES 3.0+); resolved optionally.
    resolve(gles, glGenTransformFeedbacks, "glGenTransformFeedbacks");
    resolve(gles, glDeleteTransformFeedbacks, "glDeleteTransformFeedbacks");
    resolve(gles, glBindTransformFeedback, "glBindTransformFeedback");
    resolve(gles, glBeginTransformFeedback, "glBeginTransformFeedback");
    resolve(gles, glEndTransformFeedback, "glEndTransformFeedback");
    resolve(gles, glPauseTransformFeedback, "glPauseTransformFeedback");
    resolve(gles, glResumeTransformFeedback, "glResumeTransformFeedback");

    // Sampler objects (ES 3.0+); resolved optionally.
    resolve(gles, glGenSamplers, "glGenSamplers");
    resolve(gles, glDeleteSamplers, "glDeleteSamplers");
    resolve(gles, glBindSampler, "glBindSampler");
    // glBindImageTexture is core in GLES 3.1; resolve it optionally so load()
    // still succeeds on ES 3.0 drivers that omit it.
    resolve(gles, glBindImageTexture, "glBindImageTexture");
    resolve(gles, glSamplerParameteri, "glSamplerParameteri");
    resolve(gles, glSamplerParameterf, "glSamplerParameterf");
    resolve(gles, glSamplerParameterfv, "glSamplerParameterfv");
    resolve(gles, glSamplerParameteriv, "glSamplerParameteriv");
    resolve(gles, glSamplerParameterIiv, "glSamplerParameterIiv");
    resolve(gles, glSamplerParameterIuiv, "glSamplerParameterIuiv");
    resolve(gles, glIsSampler, "glIsSampler");

    // Query objects (ES 3.0+); resolved optionally.
    resolve(gles, glGenQueries, "glGenQueries");
    resolve(gles, glDeleteQueries, "glDeleteQueries");
    resolve(gles, glIsQuery, "glIsQuery");
    resolve(gles, glBeginQuery, "glBeginQuery");
    resolve(gles, glEndQuery, "glEndQuery");
    resolve(gles, glQueryCounter, "glQueryCounter");
    resolve(gles, glGetQueryiv, "glGetQueryiv");
    resolve(gles, glGetQueryObjectiv, "glGetQueryObjectiv");
    resolve(gles, glGetQueryObjectuiv, "glGetQueryObjectuiv");
    resolve(gles, glGetQueryObjectui64v, "glGetQueryObjectui64v");

    // Shader precision query (SPEC §7.1, core in GLES 2.0+); resolved optionally.
    resolve(gles, glGetShaderPrecisionFormat, "glGetShaderPrecisionFormat");

    // Conditional rendering (SPEC §10.11). NV_conditional_render on GLES;
    // resolved optionally so load() still succeeds without it.
    resolve(gles, glBeginConditionalRenderNV, "glBeginConditionalRenderNV");
    resolve(gles, glEndConditionalRenderNV, "glEndConditionalRenderNV");

    // Color logic op + framebuffer copy/invalidate (ES 3.0+); resolved optionally.
    resolve(gles, glLogicOp, "glLogicOp");
    resolve(gles, glBlitFramebuffer, "glBlitFramebuffer");
    resolve(gles, glInvalidateFramebuffer, "glInvalidateFramebuffer");
    resolve(gles, glInvalidateSubFramebuffer, "glInvalidateSubFramebuffer");

    // Vertex attributes are ES 2.0+; resolve optionally so load() still succeeds
    // if a driver somehow lacks them.
    resolve(gles, glGetAttribLocation, "glGetAttribLocation");
    resolve(gles, glBindAttribLocation, "glBindAttribLocation");
    resolve(gles, glEnableVertexAttribArray, "glEnableVertexAttribArray");
    resolve(gles, glDisableVertexAttribArray, "glDisableVertexAttribArray");
    resolve(gles, glVertexAttribPointer, "glVertexAttribPointer");
    resolve(gles, glVertexAttribDivisor, "glVertexAttribDivisor");

    // Uniforms are ES 2.0+; resolve optionally so load() still succeeds if a
    // driver somehow lacks them (capability system reports unsupported instead).
    resolve(gles, glGetUniformLocation, "glGetUniformLocation");
    resolve(gles, glUniform1f, "glUniform1f");
    resolve(gles, glUniform2f, "glUniform2f");
    resolve(gles, glUniform3f, "glUniform3f");
    resolve(gles, glUniform4f, "glUniform4f");
    resolve(gles, glUniform1i, "glUniform1i");
    resolve(gles, glUniform2i, "glUniform2i");
    resolve(gles, glUniform3i, "glUniform3i");
    resolve(gles, glUniform4i, "glUniform4i");
    resolve(gles, glUniform1fv, "glUniform1fv");
    resolve(gles, glUniform2fv, "glUniform2fv");
    resolve(gles, glUniform3fv, "glUniform3fv");
    resolve(gles, glUniform4fv, "glUniform4fv");
    resolve(gles, glUniform1iv, "glUniform1iv");
    resolve(gles, glUniform2iv, "glUniform2iv");
    resolve(gles, glUniform3iv, "glUniform3iv");
    resolve(gles, glUniform4iv, "glUniform4iv");
    resolve(gles, glUniform1ui, "glUniform1ui");
    resolve(gles, glUniform2ui, "glUniform2ui");
    resolve(gles, glUniform3ui, "glUniform3ui");
    resolve(gles, glUniform4ui, "glUniform4ui");
    resolve(gles, glUniform1uiv, "glUniform1uiv");
    resolve(gles, glUniform2uiv, "glUniform2uiv");
    resolve(gles, glUniform3uiv, "glUniform3uiv");
    resolve(gles, glUniform4uiv, "glUniform4uiv");
    resolve(gles, glUniformMatrix2fv, "glUniformMatrix2fv");
    resolve(gles, glUniformMatrix3fv, "glUniformMatrix3fv");
    resolve(gles, glUniformMatrix4fv, "glUniformMatrix4fv");
    resolve(gles, glUniformMatrix2x3fv, "glUniformMatrix2x3fv");
    resolve(gles, glUniformMatrix2x4fv, "glUniformMatrix2x4fv");
    resolve(gles, glUniformMatrix3x2fv, "glUniformMatrix3x2fv");
    resolve(gles, glUniformMatrix3x4fv, "glUniformMatrix3x4fv");
    resolve(gles, glUniformMatrix4x2fv, "glUniformMatrix4x2fv");
    resolve(gles, glUniformMatrix4x3fv, "glUniformMatrix4x3fv");
    resolve(gles, glGetUniformfv, "glGetUniformfv");
    resolve(gles, glGetUniformiv, "glGetUniformiv");
    resolve(gles, glGetUniformuiv, "glGetUniformuiv");

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
