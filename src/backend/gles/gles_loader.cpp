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

// Resolve a GLES client-API entry point. In adopt mode we prefer the host EGL's
// eglGetProcAddress, which yields pointers into the driver mapping that actually
// backs the (externally-owned) context; otherwise we fall back to dlsym on our
// own dlopen'd libGLESv2 handle.
template <typename F>
bool resolveGl(GLESLib& lib, F& fn, const char* sym) {
    if (lib.resolveGLViaProcAddr && lib.eglGetProcAddress) {
        fn = reinterpret_cast<F>(lib.eglGetProcAddress(sym));
        if (fn) return true;
    }
    if (!lib.glesHandle) return false;
    fn = reinterpret_cast<F>(dlsym(lib.glesHandle, sym));
    return fn != nullptr;
}
} // namespace

bool GLESLib::load() {
    void* egl = openLib({"libEGL.so.1", "libEGL.so", "libEGL.so.2"});
    if (!egl) return false;
    // In adopt mode (resolveGLViaProcAddr already set by setAdopt), GL entry points
    // are resolved through the host's eglGetProcAddress; libGLESv2 need not be
    // dlopen'd and may be absent in the LD_PRELOAD/shim environment.
    void* gles = nullptr;
    if (!resolveGLViaProcAddr) {
        gles = openLib(
            {"libGLESv2.so.2", "libGLESv2.so", "libGLESv2.so.1", "libGLESv3.so"});
        if (!gles) {
            dlclose(egl);
            return false;
        }
    }
    glesHandle = gles;

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
    resolveGl(*this, glGetError, "glGetError");
    resolveGl(*this, glGetString, "glGetString");
    resolveGl(*this, glGetIntegerv, "glGetIntegerv");
    resolveGl(*this, glGetBooleanv, "glGetBooleanv");
    resolveGl(*this, glGetFloatv, "glGetFloatv");
    resolveGl(*this, glGetDoublev, "glGetDoublev");
    resolveGl(*this, glGetInteger64v, "glGetInteger64v");
    resolveGl(*this, glGetStringi, "glGetStringi");
    resolveGl(*this, glGenBuffers, "glGenBuffers");
    resolveGl(*this, glDeleteBuffers, "glDeleteBuffers");
    resolveGl(*this, glBindBuffer, "glBindBuffer");
    resolveGl(*this, glBufferData, "glBufferData");
    // Buffer sub-data / immutable storage / copy are ES 3.1+; resolved optionally
    // so load() still succeeds on a driver that lacks them (capability reports
    // unsupported / the frontend emulates via its CPU mirror).
    resolveGl(*this, glBufferSubData, "glBufferSubData");
    resolveGl(*this, glBufferStorage, "glBufferStorage");
    resolveGl(*this, glCopyBufferSubData, "glCopyBufferSubData");
    // DSA buffer allocation (ES 3.1+, SPEC §6.1/§6.2). Optional: the frontend's
    // target-based path binds then allocates when these are absent.
    resolveGl(*this, glNamedBufferData, "glNamedBufferData");
    resolveGl(*this, glNamedBufferSubData, "glNamedBufferSubData");
    resolveGl(*this, glNamedBufferStorage, "glNamedBufferStorage");
    resolveGl(*this, glMapBufferRange, "glMapBufferRange");
    resolveGl(*this, glUnmapBuffer, "glUnmapBuffer");
    // DSA buffer mapping (ES 3.1+, SPEC §6.1). Optional.
    resolveGl(*this, glMapNamedBufferRange, "glMapNamedBufferRange");
    resolveGl(*this, glUnmapNamedBuffer, "glUnmapNamedBuffer");
    resolveGl(*this, glFlushMappedNamedBufferRange, "glFlushMappedNamedBufferRange");
    resolveGl(*this, glFlushMappedBufferRange, "glFlushMappedBufferRange");
    resolveGl(*this, glInvalidateBufferData, "glInvalidateBufferData");
    resolveGl(*this, glInvalidateBufferSubData, "glInvalidateBufferSubData");
    resolveGl(*this, glGenTextures, "glGenTextures");
    resolveGl(*this, glDeleteTextures, "glDeleteTextures");
    resolveGl(*this, glBindTexture, "glBindTexture");
    resolveGl(*this, glActiveTexture, "glActiveTexture");
    resolveGl(*this, glGenRenderbuffers, "glGenRenderbuffers");
    resolveGl(*this, glDeleteRenderbuffers, "glDeleteRenderbuffers");
    resolveGl(*this, glBindRenderbuffer, "glBindRenderbuffer");
    resolveGl(*this, glRenderbufferStorage, "glRenderbufferStorage");
    resolveGl(*this, glRenderbufferStorageMultisample,
                  "glRenderbufferStorageMultisample");
    resolveGl(*this, glGenFramebuffers, "glGenFramebuffers");
    resolveGl(*this, glDeleteFramebuffers, "glDeleteFramebuffers");
    resolveGl(*this, glBindFramebuffer, "glBindFramebuffer");
    resolveGl(*this, glGenVertexArrays, "glGenVertexArrays");
    resolveGl(*this, glDeleteVertexArrays, "glDeleteVertexArrays");
    resolveGl(*this, glBindVertexArray, "glBindVertexArray");
    resolveGl(*this, glCreateShader, "glCreateShader");
    resolveGl(*this, glShaderSource, "glShaderSource");
    resolveGl(*this, glCompileShader, "glCompileShader");
    resolveGl(*this, glGetShaderiv, "glGetShaderiv");
    resolveGl(*this, glGetShaderInfoLog, "glGetShaderInfoLog");
    resolveGl(*this, glDeleteShader, "glDeleteShader");
    resolveGl(*this, glCreateProgram, "glCreateProgram");
    resolveGl(*this, glAttachShader, "glAttachShader");
    resolveGl(*this, glDetachShader, "glDetachShader");
    resolveGl(*this, glLinkProgram, "glLinkProgram");
    resolveGl(*this, glGetProgramiv, "glGetProgramiv");
    resolveGl(*this, glGetProgramInfoLog, "glGetProgramInfoLog");
    resolveGl(*this, glDeleteProgram, "glDeleteProgram");
    // Program-interface reflection (ES 3.0+); resolved optionally so load()
    // still succeeds on stricter/driver-limited EGL stacks.
    resolveGl(*this, glGetProgramInterfaceiv, "glGetProgramInterfaceiv");
    resolveGl(*this, glGetProgramResourceIndex, "glGetProgramResourceIndex");
    resolveGl(*this, glGetProgramResourceName, "glGetProgramResourceName");
    resolveGl(*this, glGetProgramResourceiv, "glGetProgramResourceiv");
    resolveGl(*this, glGetProgramResourceLocation, "glGetProgramResourceLocation");
    resolveGl(*this, glGetProgramResourceLocationIndex, "glGetProgramResourceLocationIndex");
    // Subroutines (ES 3.1+); resolved optionally so load() still succeeds on
    // stricter/driver-limited EGL stacks.
    resolveGl(*this, glGetSubroutineIndex, "glGetSubroutineIndex");
    resolveGl(*this, glGetSubroutineUniformLocation, "glGetSubroutineUniformLocation");
    resolveGl(*this, glGetActiveSubroutineUniformiv, "glGetActiveSubroutineUniformiv");
    resolveGl(*this, glGetActiveSubroutineUniformName, "glGetActiveSubroutineUniformName");
    resolveGl(*this, glGetActiveSubroutineName, "glGetActiveSubroutineName");
    resolveGl(*this, glUniformSubroutinesuiv, "glUniformSubroutinesuiv");
    resolveGl(*this, glGetUniformSubroutineuiv, "glGetUniformSubroutineuiv");
    resolveGl(*this, glGetProgramStageiv, "glGetProgramStageiv");
    resolveGl(*this, glEnable, "glEnable");
    resolveGl(*this, glDisable, "glDisable");
    resolveGl(*this, glEnablei, "glEnablei");
    resolveGl(*this, glDisablei, "glDisablei");
    resolveGl(*this, glUseProgram, "glUseProgram");
    resolveGl(*this, glBlendFunc, "glBlendFunc");
    resolveGl(*this, glBlendEquation, "glBlendEquation");
    // Optional (core in ES 2.0/3.x but resolved defensively): separate blend and
    // constant blend color. The backend falls back to glBlendFunc/glBlendEquation
    // when these are unavailable.
    resolveGl(*this, glBlendFuncSeparate, "glBlendFuncSeparate");
    resolveGl(*this, glBlendEquationSeparate, "glBlendEquationSeparate");
    resolveGl(*this, glBlendColor, "glBlendColor");
    // Indexed blending (SPEC §15.3 / §17.3.4). Optional: ES 3.2+ / drivers with
    // ARB_draw_buffers_blend; resolved defensively so older drivers stay usable.
    resolveGl(*this, glBlendFunci, "glBlendFunci");
    resolveGl(*this, glBlendFuncSeparatei, "glBlendFuncSeparatei");
    resolveGl(*this, glBlendEquationi, "glBlendEquationi");
    resolveGl(*this, glBlendEquationSeparatei, "glBlendEquationSeparatei");
    resolveGl(*this, glDepthFunc, "glDepthFunc");
    resolveGl(*this, glDepthMask, "glDepthMask");
    resolveGl(*this, glDepthRangef, "glDepthRangef");
    resolveGl(*this, glDepthRangefIndexed, "glDepthRangefIndexed");
    resolveGl(*this, glStencilFunc, "glStencilFunc");
    resolveGl(*this, glStencilOp, "glStencilOp");
    resolveGl(*this, glStencilMask, "glStencilMask");
    resolveGl(*this, glStencilFuncSeparate, "glStencilFuncSeparate");
    resolveGl(*this, glStencilOpSeparate, "glStencilOpSeparate");
    resolveGl(*this, glStencilMaskSeparate, "glStencilMaskSeparate");
    resolveGl(*this, glColorMask, "glColorMask");
    resolveGl(*this, glColorMaski, "glColorMaski");
    resolveGl(*this, glDispatchCompute, "glDispatchCompute");
    resolveGl(*this, glDispatchComputeIndirect, "glDispatchComputeIndirect");
    resolveGl(*this, glSampleCoverage, "glSampleCoverage");
    // glPrimitiveRestartIndex is core in GLES 3.0 but some implementations expose
    // it conditionally; resolve it optionally so load() still succeeds without it.
    resolveGl(*this, glPrimitiveRestartIndex, "glPrimitiveRestartIndex");
    // glPatchParameteri is core in GLES 3.2; resolve it optionally so load()
    // still succeeds on ES 3.0/3.1 drivers that omit it.
    resolveGl(*this, glPatchParameteri, "glPatchParameteri");
    resolveGl(*this, glCullFace, "glCullFace");
    resolveGl(*this, glFrontFace, "glFrontFace");
    // glPointSize was removed from the OpenGL ES 3.0 API (point size is set via
    // the gl_PointSize vertex-shader builtin). Resolve it optionally so load()
    // still succeeds on ES 3.0+ drivers that omit it; the frontend keeps tracking
    // the state (SPEC §10) and GLESBackend::pointSize no-ops when absent.
    resolveGl(*this, glPointSize, "glPointSize");
    // glHint is core in GLES but a no-op on most drivers; resolved optionally so
    // load() still succeeds where the symbol is absent.
    resolveGl(*this, glHint, "glHint");
    resolveGl(*this, glLineWidth, "glLineWidth");
    resolveGl(*this, glPolygonOffset, "glPolygonOffset");
    resolveGl(*this, glPolygonOffsetClamp, "glPolygonOffsetClamp");
    resolveGl(*this, glPixelStorei, "glPixelStorei");
    resolveGl(*this, glViewport, "glViewport");
    resolveGl(*this, glScissor, "glScissor");
    resolveGl(*this, glViewportIndexedf, "glViewportIndexedf");
    resolveGl(*this, glScissorIndexed, "glScissorIndexed");
    resolveGl(*this, glClearColor, "glClearColor");
    resolveGl(*this, glClearDepthf, "glClearDepthf");
    resolveGl(*this, glClearStencil, "glClearStencil");
    resolveGl(*this, glClear, "glClear");
    resolveGl(*this, glClearTexImage, "glClearTexImage");
    resolveGl(*this, glClearTexSubImage, "glClearTexSubImage");
    resolveGl(*this, glCopyImageSubData, "glCopyImageSubData");
    resolveGl(*this, glFlush, "glFlush");
    resolveGl(*this, glFinish, "glFinish");
    resolveGl(*this, glReadPixels, "glReadPixels");
    // glReadnPixels is core in GL 4.5 (ARB_robustness) but not a standard ES entry
    // point; resolve it optionally so load() still succeeds on ES drivers that
    // omit it, and GLESBackend::readnPixels falls back to glReadPixels.
    resolveGl(*this, glReadnPixels, "glReadnPixels");
    // glTextureBarrierNV (GL_NV_texture_barrier) is the ES spelling of
    // glTextureBarrier; resolve optionally.
    resolveGl(*this, glTextureBarrierNV, "glTextureBarrierNV");
    resolveGl(*this, glBindBufferBase, "glBindBufferBase");
    resolveGl(*this, glBindBufferRange, "glBindBufferRange");
    resolveGl(*this, glUniformBlockBinding, "glUniformBlockBinding");
    resolveGl(*this, glShaderStorageBlockBinding, "glShaderStorageBlockBinding");
    // Transform-feedback varying capture setup (SPEC §13.3.1). ES 3.0+; resolved
    // optionally so load() still succeeds without it.
    resolveGl(*this, glTransformFeedbackVaryings, "glTransformFeedbackVaryings");
    resolveGl(*this, glTexImage2D, "glTexImage2D");
    resolveGl(*this, glTexImage3D, "glTexImage3D");
    resolveGl(*this, glTexParameteri, "glTexParameteri");
    // Parameter setters (float / vector) are core in GLES but resolved defensively.
    resolveGl(*this, glTexParameterf, "glTexParameterf");
    resolveGl(*this, glTexParameterfv, "glTexParameterfv");
    resolveGl(*this, glTexParameteriv, "glTexParameteriv");
    // Sub-image / copy-from-framebuffer are core in GLES but resolved defensively.
    resolveGl(*this, glTexSubImage1D, "glTexSubImage1D");
    resolveGl(*this, glTexSubImage2D, "glTexSubImage2D");
    resolveGl(*this, glTexSubImage3D, "glTexSubImage3D");
    resolveGl(*this, glCopyTexImage1D, "glCopyTexImage1D");
    resolveGl(*this, glCopyTexImage2D, "glCopyTexImage2D");
    resolveGl(*this, glCopyTexSubImage2D, "glCopyTexSubImage2D");
    resolveGl(*this, glCopyTexSubImage3D, "glCopyTexSubImage3D");
    // Compressed texture upload (SPEC §8.6, GLES 3.0 core).
    resolveGl(*this, glCompressedTexImage1D, "glCompressedTexImage1D");
    resolveGl(*this, glCompressedTexImage2D, "glCompressedTexImage2D");
    resolveGl(*this, glCompressedTexImage3D, "glCompressedTexImage3D");
    resolveGl(*this, glCompressedTexSubImage1D, "glCompressedTexSubImage1D");
    resolveGl(*this, glCompressedTexSubImage2D, "glCompressedTexSubImage2D");
    resolveGl(*this, glCompressedTexSubImage3D, "glCompressedTexSubImage3D");
    // Immutable texture storage + DSA helpers (GL 4.2/4.5). Resolved optionally so
    // load() still succeeds on drivers that lack them.
    resolveGl(*this, glTexStorage1D, "glTexStorage1D");
    resolveGl(*this, glTexStorage2D, "glTexStorage2D");
    resolveGl(*this, glTexStorage3D, "glTexStorage3D");
    resolveGl(*this, glTexStorage2DMultisample, "glTexStorage2DMultisample");
    resolveGl(*this, glTexStorage3DMultisample, "glTexStorage3DMultisample");
    resolveGl(*this, glTexImage2DMultisample, "glTexImage2DMultisample");
    resolveGl(*this, glTexImage3DMultisample, "glTexImage3DMultisample");
    resolveGl(*this, glGenerateMipmap, "glGenerateMipmap");
    resolveGl(*this, glGetTexImage, "glGetTexImage");
    // Robustness read-back (ARB_robustness / GL 4.5). Resolved optionally: a
    // driver without these entries still loads; GLESBackend falls back to the
    // non-robust read at runtime.
    resolveGl(*this, glGetnTexImage, "glGetnTexImage");
    resolveGl(*this, glGetnCompressedTexImage, "glGetnCompressedTexImage");
    resolveGl(*this, glGetCompressedTexImage, "glGetCompressedTexImage");
    resolveGl(*this, glGetTexLevelParameteriv, "glGetTexLevelParameteriv");
    resolveGl(*this, glGetTexLevelParameterfv, "glGetTexLevelParameterfv");
    resolveGl(*this, glGetInternalformativ, "glGetInternalformativ");
    // Multisample sample-position query (SPEC §14.3.1 glGetMultisamplefv). GLES
    // 3.1+; resolved optionally so load() still succeeds on a driver that lacks
    // it (the frontend reports the entry point unsupported-dependently via error).
    resolveGl(*this, glGetMultisamplefv, "glGetMultisamplefv");
    resolveGl(*this, glTexBuffer, "glTexBuffer");
    resolveGl(*this, glTexBufferRange, "glTexBufferRange");
    // Texture views (SPEC §8.19 glTextureView) are ES 3.1+; resolved optionally so
    // load() still succeeds on drivers that lack them (capability reports views
    // unsupported, the frontend rejects the call with GL_INVALID_OPERATION).
    resolveGl(*this, glTextureView, "glTextureView");
    // Integer texture parameters + texture invalidation (SPEC §8.1, ES 3.0+).
    resolveGl(*this, glTexParameterIiv, "glTexParameterIiv");
    resolveGl(*this, glTexParameterIuiv, "glTexParameterIuiv");
    resolveGl(*this, glInvalidateTexImage, "glInvalidateTexImage");
    resolveGl(*this, glInvalidateTexSubImage, "glInvalidateTexSubImage");
    resolveGl(*this, glFramebufferTexture2D, "glFramebufferTexture2D");
    resolveGl(*this, glFramebufferRenderbuffer, "glFramebufferRenderbuffer");
    resolveGl(*this, glFramebufferTextureLayer, "glFramebufferTextureLayer");
    resolveGl(*this, glFramebufferParameteri, "glFramebufferParameteri");
    // Framebuffer parameter read-back (SPEC §9.2.3). GLES 3.0+; resolved
    // optionally. Used to read the bound framebuffer's SAMPLES for
    // glGetMultisamplefv index validation.
    resolveGl(*this, glGetFramebufferParameteriv, "glGetFramebufferParameteriv");
    resolveGl(*this, glCheckFramebufferStatus, "glCheckFramebufferStatus");
    resolveGl(*this, glGetFramebufferAttachmentParameteriv,
              "glGetFramebufferAttachmentParameteriv");
    // Whole-framebuffer buffer selection (core in GLES but resolved defensively).
    resolveGl(*this, glDrawBuffers, "glDrawBuffers");
    resolveGl(*this, glReadBuffer, "glReadBuffer");
    resolveGl(*this, glDrawArrays, "glDrawArrays");
    resolveGl(*this, glDrawElements, "glDrawElements");
    // Instanced draws are ES 3.0+; resolve optionally so load() still succeeds
    // on a driver that lacks them (the capability system reports them
    // unsupported instead of failing the whole backend init).
    resolveGl(*this, glDrawArraysInstanced, "glDrawArraysInstanced");
    resolveGl(*this, glDrawElementsInstanced, "glDrawElementsInstanced");
    // Draw expansion (SPEC §10): solved optionally (ES 3.0+ / ES 3.2).
    resolveGl(*this, glMultiDrawArrays, "glMultiDrawArrays");
    resolveGl(*this, glMultiDrawElements, "glMultiDrawElements");
    resolveGl(*this, glMultiDrawArraysBaseInstance, "glMultiDrawArraysBaseInstance");
    resolveGl(*this, glMultiDrawElementsBaseInstance, "glMultiDrawElementsBaseInstance");
    resolveGl(*this, glDrawRangeElements, "glDrawRangeElements");
    resolveGl(*this, glDrawElementsBaseVertex, "glDrawElementsBaseVertex");
    resolveGl(*this, glDrawElementsInstancedBaseVertex, "glDrawElementsInstancedBaseVertex");
    resolveGl(*this, glDrawRangeElementsBaseVertex, "glDrawRangeElementsBaseVertex");
    resolveGl(*this, glMultiDrawElementsBaseVertex, "glMultiDrawElementsBaseVertex");
    // Base-instance draws (SPEC §10, ES 3.2+); resolved optionally.
    resolveGl(*this, glDrawArraysInstancedBaseInstance, "glDrawArraysInstancedBaseInstance");
    resolveGl(*this, glDrawElementsInstancedBaseInstance, "glDrawElementsInstancedBaseInstance");
    resolveGl(*this, glDrawElementsInstancedBaseVertexBaseInstance,
            "glDrawElementsInstancedBaseVertexBaseInstance");
    // Indirect draw (SPEC §10, ES 3.1+); resolved optionally.
    resolveGl(*this, glDrawArraysIndirect, "glDrawArraysIndirect");
    resolveGl(*this, glDrawElementsIndirect, "glDrawElementsIndirect");
    // Multi-draw indirect (SPEC §10, ES 3.1+); resolved optionally.
    resolveGl(*this, glMultiDrawArraysIndirect, "glMultiDrawArraysIndirect");
    resolveGl(*this, glMultiDrawElementsIndirect, "glMultiDrawElementsIndirect");
    // Multi-draw indirect count (SPEC §10.4, GL 4.6); resolved optionally (never
    // present in GLES). Not folded into `ok` so it cannot fail backend load().
    resolveGl(*this, glMultiDrawArraysIndirectCount, "glMultiDrawArraysIndirectCount");
    resolveGl(*this, glMultiDrawElementsIndirectCount,
            "glMultiDrawElementsIndirectCount");
    // Transform-feedback draws (SPEC §13.3.3, ES 3.2+); resolved optionally.
    resolveGl(*this, glDrawTransformFeedback, "glDrawTransformFeedback");
    resolveGl(*this, glDrawTransformFeedbackInstanced, "glDrawTransformFeedbackInstanced");
    resolveGl(*this, glDrawTransformFeedbackStream, "glDrawTransformFeedbackStream");
    resolveGl(*this, glDrawTransformFeedbackStreamInstanced, "glDrawTransformFeedbackStreamInstanced");

    // Transform feedback (ES 3.0+); resolved optionally.
    resolveGl(*this, glGenTransformFeedbacks, "glGenTransformFeedbacks");
    resolveGl(*this, glDeleteTransformFeedbacks, "glDeleteTransformFeedbacks");
    resolveGl(*this, glBindTransformFeedback, "glBindTransformFeedback");
    resolveGl(*this, glBeginTransformFeedback, "glBeginTransformFeedback");
    resolveGl(*this, glEndTransformFeedback, "glEndTransformFeedback");
    resolveGl(*this, glPauseTransformFeedback, "glPauseTransformFeedback");
    resolveGl(*this, glResumeTransformFeedback, "glResumeTransformFeedback");

    // Sampler objects (ES 3.0+); resolved optionally.
    resolveGl(*this, glGenSamplers, "glGenSamplers");
    resolveGl(*this, glDeleteSamplers, "glDeleteSamplers");
    resolveGl(*this, glBindSampler, "glBindSampler");
    // glBindImageTexture is core in GLES 3.1; resolve it optionally so load()
    // still succeeds on ES 3.0 drivers that omit it.
    resolveGl(*this, glBindImageTexture, "glBindImageTexture");
    resolveGl(*this, glSamplerParameteri, "glSamplerParameteri");
    resolveGl(*this, glSamplerParameterf, "glSamplerParameterf");
    resolveGl(*this, glSamplerParameterfv, "glSamplerParameterfv");
    resolveGl(*this, glSamplerParameteriv, "glSamplerParameteriv");
    resolveGl(*this, glSamplerParameterIiv, "glSamplerParameterIiv");
    resolveGl(*this, glSamplerParameterIuiv, "glSamplerParameterIuiv");
    resolveGl(*this, glIsSampler, "glIsSampler");

    // Query objects (ES 3.0+); resolved optionally.
    resolveGl(*this, glGenQueries, "glGenQueries");
    resolveGl(*this, glDeleteQueries, "glDeleteQueries");
    resolveGl(*this, glIsQuery, "glIsQuery");
    resolveGl(*this, glBeginQuery, "glBeginQuery");
    resolveGl(*this, glEndQuery, "glEndQuery");
    resolveGl(*this, glQueryCounter, "glQueryCounter");
    resolveGl(*this, glGetQueryiv, "glGetQueryiv");
    resolveGl(*this, glGetQueryObjectiv, "glGetQueryObjectiv");
    resolveGl(*this, glGetQueryObjectuiv, "glGetQueryObjectuiv");
    resolveGl(*this, glGetQueryObjectui64v, "glGetQueryObjectui64v");

    // Shader precision query (SPEC §7.1, core in GLES 2.0+); resolved optionally.
    resolveGl(*this, glGetShaderPrecisionFormat, "glGetShaderPrecisionFormat");

    // Conditional rendering (SPEC §10.11). NV_conditional_render on GLES;
    // resolved optionally so load() still succeeds without it.
    resolveGl(*this, glBeginConditionalRenderNV, "glBeginConditionalRenderNV");
    resolveGl(*this, glEndConditionalRenderNV, "glEndConditionalRenderNV");

    // Color logic op + framebuffer copy/invalidate (ES 3.0+); resolved optionally.
    resolveGl(*this, glLogicOp, "glLogicOp");
    resolveGl(*this, glBlitFramebuffer, "glBlitFramebuffer");
    resolveGl(*this, glInvalidateFramebuffer, "glInvalidateFramebuffer");
    resolveGl(*this, glInvalidateSubFramebuffer, "glInvalidateSubFramebuffer");

    // Vertex attributes are ES 2.0+; resolve optionally so load() still succeeds
    // if a driver somehow lacks them.
    resolveGl(*this, glGetAttribLocation, "glGetAttribLocation");
    resolveGl(*this, glBindAttribLocation, "glBindAttribLocation");
    resolveGl(*this, glEnableVertexAttribArray, "glEnableVertexAttribArray");
    resolveGl(*this, glDisableVertexAttribArray, "glDisableVertexAttribArray");
    resolveGl(*this, glVertexAttribPointer, "glVertexAttribPointer");
    resolveGl(*this, glVertexAttribDivisor, "glVertexAttribDivisor");

    // Uniforms are ES 2.0+; resolve optionally so load() still succeeds if a
    // driver somehow lacks them (capability system reports unsupported instead).
    resolveGl(*this, glGetUniformLocation, "glGetUniformLocation");
    resolveGl(*this, glUniform1f, "glUniform1f");
    resolveGl(*this, glUniform2f, "glUniform2f");
    resolveGl(*this, glUniform3f, "glUniform3f");
    resolveGl(*this, glUniform4f, "glUniform4f");
    resolveGl(*this, glUniform1i, "glUniform1i");
    resolveGl(*this, glUniform2i, "glUniform2i");
    resolveGl(*this, glUniform3i, "glUniform3i");
    resolveGl(*this, glUniform4i, "glUniform4i");
    resolveGl(*this, glUniform1fv, "glUniform1fv");
    resolveGl(*this, glUniform2fv, "glUniform2fv");
    resolveGl(*this, glUniform3fv, "glUniform3fv");
    resolveGl(*this, glUniform4fv, "glUniform4fv");
    resolveGl(*this, glUniform1iv, "glUniform1iv");
    resolveGl(*this, glUniform2iv, "glUniform2iv");
    resolveGl(*this, glUniform3iv, "glUniform3iv");
    resolveGl(*this, glUniform4iv, "glUniform4iv");
    resolveGl(*this, glUniform1ui, "glUniform1ui");
    resolveGl(*this, glUniform2ui, "glUniform2ui");
    resolveGl(*this, glUniform3ui, "glUniform3ui");
    resolveGl(*this, glUniform4ui, "glUniform4ui");
    resolveGl(*this, glUniform1uiv, "glUniform1uiv");
    resolveGl(*this, glUniform2uiv, "glUniform2uiv");
    resolveGl(*this, glUniform3uiv, "glUniform3uiv");
    resolveGl(*this, glUniform4uiv, "glUniform4uiv");
    resolveGl(*this, glUniformMatrix2fv, "glUniformMatrix2fv");
    resolveGl(*this, glUniformMatrix3fv, "glUniformMatrix3fv");
    resolveGl(*this, glUniformMatrix4fv, "glUniformMatrix4fv");
    resolveGl(*this, glUniformMatrix2x3fv, "glUniformMatrix2x3fv");
    resolveGl(*this, glUniformMatrix2x4fv, "glUniformMatrix2x4fv");
    resolveGl(*this, glUniformMatrix3x2fv, "glUniformMatrix3x2fv");
    resolveGl(*this, glUniformMatrix3x4fv, "glUniformMatrix3x4fv");
    resolveGl(*this, glUniformMatrix4x2fv, "glUniformMatrix4x2fv");
    resolveGl(*this, glUniformMatrix4x3fv, "glUniformMatrix4x3fv");
    resolveGl(*this, glGetUniformfv, "glGetUniformfv");
    resolveGl(*this, glGetUniformiv, "glGetUniformiv");
    resolveGl(*this, glGetUniformuiv, "glGetUniformuiv");

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
