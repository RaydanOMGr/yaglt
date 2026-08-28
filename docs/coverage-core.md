# OpenGL 4.6 Core Profile — Implementation Coverage

Status assessment for **YAGLT** as of 2026-08-28 (regenerated). Companion to
`docs/feature-matrix.md` and `docs/agent-progress.md`. The goal of this
document is to answer one question quantitatively: **how much of the OpenGL
4.6 spec is implemented, specifically the core profile?**

## Method

1. The OpenGL 4.6 (Compatibility Profile) specification (`OpenGL-4.6-
   Compatibility.md`, 23 780 lines) was scanned for command prototypes of the
   form `void <Name>(` — both the literal `void Foo(` declarations **and** the
   braced template families (e.g. `void TexParameter{if}(`, `void
   VertexAttrib{1234}{...}(`), each of which collapses to a single command
   family. This yields the **command universe**.
2. The implemented frontend surface (`include/glcompat/frontend/gl_api.hpp`)
   was extracted as `gl*` entry-point names. Each was normalized to a spec
   command family (stripping the `gl` prefix and trailing vector/type/digit
   markers such as `fv`/`iv`/`uiv`/`Matrix4fv` → family) and matched against the
   command universe.
3. The **core profile** was isolated by removing the compatibility-only
   (removed-in-core) commands enumerated in spec **Appendix E.2.2** (immediate
   mode, fixed-function matrix/lighting/texgen, display lists, selection/
   feedback, evaluators, attribute stacks, pixel drawing, accum, etc.).

### Caveat on the number

The spec text defines many type/vector variants as a single family prototype
(e.g. `void Uniform1f(` is present, but `Uniform1fv`/`UniformMatrix4fv` and the
`TexParameter*`/`Get*` variants are not all emitted as separate `void X(`
prototypes). So the extracted universe still undercounts the real GL command
set (the real API has ~700+ entry points). Consequently the percentages below
are an **optimistic proxy**: they measure how many of the spec's *declared
command prototypes / families* have a frontend entry point, not the true entry-
point count. The qualitative chapter breakdown (below) is the more reliable
signal. A reproducible regen script counts 571 declared families, 315 `gl_api`
   entry points, and 311 matched families.

## Headline numbers

| Universe | Prototypes | With frontend entry point | Coverage |
|----------|-----------:|--------------------------:|---------:|
| Full spec (compat + core) | 571 | 311 | **54.5%** |
| Core profile only (~571 − ~55 removed commands) | ~516 | 311 | **~60.3%** |

> Note: this document was regenerated on 2026-08-28 from `gl_api.hpp` vs the
> spec universe. The per-area table below and `docs/agent-progress.md` are the
> live sources of truth; the headline proxy is a coarse signal only.

 All 311 matched families are real `gl_api` entry points with frontend semantics
and tests (mock path, most also against Mesa GLES). The 315 `gl_api` entry
points include 4 that do not map to a spec *family* in the universe:
`glFlushState` (internal helper, not a GL command), `glDeleteQuery` (singular of
the `DeleteQueries` family), and `glInvalidateNamedBufferData`/
`glInvalidateNamedBufferSubData` (spec spelling differs). None of the
compatibility-only removed commands are implemented (correct — they are out of
scope per `docs/feature-matrix.md`).

**Estimated true core coverage** (adjusting for the undercount above and for
core commands that exist as an entry point but are capability-gated to
*Unsupported*, e.g. the geometry/tessellation shader stages which have no GLES
equivalent): roughly
   **two-fifths of the real ~700-entry GL core command set** (312 of
~700 ≈ 45%).

## Core coverage by spec area

Status: ✅ Implemented · 🟡 Partial · ❌ Not implemented · 🚫 Honestly Unsupported

| Spec area (chapter) | Status | Notes |
|---------------------|--------|-------|
| §2 Fundamentals / errors / strings / flush-finish | ✅ | `glGetError`, `glGetString`, `glGetStringi` (SPEC §22.2, indexed; only `GL_EXTENSIONS` indexable, 0 extensions exposed → `GL_INVALID_VALUE` for any index), `glGetGraphicsResetStatus` (always `GL_NO_ERROR`), `glFlush`, `glFinish`, `glEnable/Disable` (tracked caps), `glGetBooleanv/Integerv/Floatv/Doublev`, `glGetInteger64v` (SPEC §22.1, widens tracked integer state to `GLint64`), `glGetBooleani_v`/`glGetIntegeri_v` (SPEC §22.1, indexed caps `GL_BLEND`/`GL_SCISSOR_TEST` per slot), `glIsEnabled`, `glEnablei/glDisablei/glIsEnabledi` (indexed caps, SPEC §10.3.1; only `GL_BLEND`/`GL_SCISSOR_TEST` indexable, invalid cap → `GL_INVALID_ENUM`, index ≥ 16 → `GL_INVALID_VALUE`, tracked per-slot, push-only-on-change) |
| §6 Buffer objects | 🟢 | gen/bind/delete, `glBufferData`, `glBindBufferBase/Range`, `glBufferSubData`, `glBufferStorage` (immutable, capability-gated), `glMapBuffer`/`glMapBufferRange`/`glUnmapBuffer`, `glCopyBufferSubData`, `glGetBufferParameteriv`, `glGetBufferParameteri64v`/`glGetNamedBufferParameteri64v` (64-bit size/usage queries, SPEC §6.1.1), `glGetNamedBufferParameteriv` (32-bit DSA counterpart, SPEC §6.1.1), `glGetBufferSubData`/`glGetNamedBufferSubData` (read the frontend CPU mirror), `glClearBufferData`/`glClearNamedBufferData`/`glClearBufferSubData`/`glClearNamedBufferSubData` (fill the mirror in-memory; the practical subset of table 8.24 sized internal formats is handled with full component/type conversion), `glInvalidateBufferData`/`glInvalidateBufferSubData`/`glInvalidateNamedBuffer*` (driver discard hint). Bounds/format/mapping validation matches SPEC §6. |
| §7 Shaders / programs | 🟡 | create/source/compile/attach/link, `glGetShader*`, `glGetProgram*`, info logs, `glUseProgram`, `glGetAttribLocation`, `glGetUniformLocation`, full `glUniform*` (f/i/vec/mat4), GLSL version gate, **program pipelines** (§7.4): `glGen/Delete/IsProgramPipeline`, `glBindProgramPipeline`, `glCreateShaderProgramv`, `glUseProgramStages`, `glActiveShaderProgram`, `glGetProgramPipelineiv`, `glValidateProgramPipeline`, `glGetProgramPipelineInfoLog` (capability-gated by `ProgramPipelines`; GLES consumes the bound pipeline via `GLStateSink` only where separable programs exist), `glBindAttribLocation` (SPEC §7.3.7: recorded frontend-side, applied to the backend program at the next link; unknown program → `GL_INVALID_OPERATION`), and **compute dispatch** (§7.4): `glDispatchCompute`/`glDispatchComputeIndirect` (capability-gated by `ComputeShaders`; requires an active program; indirect requires a buffer bound to `GL_DISPATCH_INDIRECT_BUFFER`; forwarded to the backend via `GLStateSink::dispatchCompute`/`dispatchComputeIndirect`). **Compute shader objects/stages** are now created/translated when the backend reports `ComputeShaders` (native in GLES 3.1+; the mock mirrors that baseline), so a compute program can be built, linked, and dispatched (see §7.4 dispatch). **Shader binaries** (§7.2/§19.1): `glShaderBinary` /
`glProgramBinary` / `glGetProgramBinary` load and retrieve a precompiled binary
blob (the frontend keeps the authoritative mirror, matching the buffer-mirror
pattern); loading a binary marks the program linked / the shader compiled. |
| §8 Textures / samplers | 🟡 | gen/bind/delete, `glActiveTexture`, `glBindTexture` (per-unit), `glTexImage1D/2D/3D` (1D emulated as 2D height=1 on GLES), `glTexSubImage1D/2D/3D`, `glCopyTexImage1D/2D`, `glTexParameteri`/`f`/`fv`/`iv` (scalar + vector pnames), sampler objects, DSA texture bind (`glBindTextureUnit`/`glBindTextures`), texture-parameter queries (`glGetTexParameteriv`/`fv`), DSA storage (`CreateTextures`/`TextureStorage1D/2D/3D`), DSA sub-image (`TextureSubImage1D/2D/3D`), DSA level queries (`GetTextureLevelParameteriv`/`fv`) and classic target-based level queries (`GetTexLevelParameteriv`/`fv`), `GenerateTextureMipmap`, `GetTextureImage`, `TextureBuffer`/`TextureBufferRange`, integer texture params (`glTexParameterIiv`/`Iuiv` + `glTextureParameterIiv`/`Iuiv`), integer param queries (`glGetTexParameterIiv`/`Iuiv` + `glGetTextureParameterIiv`/`Iuiv`), classic `glGenerateMipmap`, texture invalidation (`glInvalidateTexImage`/`glInvalidateTexSubImage`), and **texture views** (`glTextureView`, SPEC §8.19: validates both objects exist and differ, source has immutable storage, target is valid, and the level range fits; records the view's derived base dimensions/levels and forwards to the backend; capability-gated by `TextureViews`, native on GLES 3.1, unsupported otherwise). Non-DSA `glTexStorage1D/2D/3D` + `glTexBuffer`/`glTexBufferRange` (§8.5/§8.9) and the multisample surface — `glTexStorage2DMultisample`/`glTexStorage3DMultisample`/`glTexImage2DMultisample`/`glTexImage3DMultisample` + DSA `glTextureStorage2DMultisample`/`glTextureStorage3DMultisample` — are now implemented with target/sample/dimension validation. Missing: cube/array/rect TexImage targets, `GetTexImage` multisample. Compressed image readback (`glGetCompressedTexImage` / `glGetCompressedTextureImage`, SPEC §8.11) is now implemented (honest no-op where the backend has no native read). Texture-sub-image readback (`glGetTextureSubImage` / `glGetCompressedTextureSubImage`, SPEC §8.11.4/§8.11.5) is now implemented (DSA; honest no-op where the backend has no native read). |
| §9 (program/pipeline — folded into §7.4) | ✅ | program pipeline objects implemented (see §7 row); the pipeline stage→program mapping, active program, validation, and queries are frontend-owned and forwarded to the backend via `GLStateSink::bindProgramPipeline` |
| §10 Vertex spec / draw | 🟡 | VAO gen/bind/delete, `glVertexAttribPointer`, enable/disable attrib, `glDrawArrays`/`glDrawElements` (+ instanced), **primitive restart** (`glPrimitiveRestartIndex` + `GL_PRIMITIVE_RESTART`, SPEC §10.4), **vertex attrib divisor** (`glVertexAttribDivisor`, capability-gated), **multi-draw** (`glMultiDrawArrays`/`glMultiDrawElements`), **`glDrawRangeElements`**, **`glDrawElementsBaseVertex`** (capability-gated, ES 3.2). **DSA vertex arrays** (`glCreateVertexArrays`, `glVertexArrayElementBuffer`, `glEnable/DisableVertexArrayAttrib`, `glVertexArrayVertexBuffer(s)`, `glVertexArrayAttribFormat/IFormat/LFormat`, `glVertexArrayAttribBinding`, `glVertexArrayBindingDivisor`, SPEC §10.3.1, replayed via the unified flush path) and DSA queries `glGetVertexArrayiv` / `glGetVertexArrayIndexediv` / `glGetVertexArrayIndexed64v` (SPEC §10.3.1). **Generic vertex attribute values** (`glVertexAttrib1f..4f`/`*fv`, `glVertexAttribI4i`/`I4ui`/`I4iv`/`I4uiv`, `glGetVertexAttrib{fv,iv,dv,Iiv,Iuiv,Pointerv}` covering the full §10.4 pname set: CURRENT_VERTEX_ATTRIB plus the array state pnames ENABLED/SIZE/STRIDE/TYPE/NORMALIZED/INTEGER/DIVISOR/BUFFER_BINDING/POINTER) now implemented (SPEC §10.2/§10.4). Indirect draw implemented (SPEC §10). Missing: client array legacy (removed-in-core semantics) |
| §11 (rasterization — points/lines/polygons) | ✅ | `glPointSize` / `glLineWidth` / `glPolygonOffset` implemented (tracked scalar state, pushed only on change, GLES3-backed). `glPolygonMode` implemented (front/back mode tracked; `GL_FILL` only on GLES — honest no-op backend override), `glSampleMaski` (per-word `GL_SAMPLE_MASK` state, push-only-changed-words), `glMinSampleShading` (multisample raster state, `GL_MIN_SAMPLE_SHADING` query), and `glProvokingVertex` (SPEC §11: `GL_FIRST_VERTEX_CONVENTION` / `GL_LAST_VERTEX_CONVENTION` tracked, pushed on change, `GL_PROVOKING_VERTEX` query; invalid mode → `GL_INVALID_ENUM`; GLES records without a native call). |
| §12 (fixed-function vertex / matrix / lighting / texgen) | 🚫 | entirely removed-in-core; not implemented (correct) |
| §13 Transform feedback | 🟡 | object lifecycle + begin/end/pause/resume + capability gate; forwards to backend. Missing: actual varying capture wiring to buffers, counter queries |
| §14 (rasterization per-fragment — depth/stencil/blend/scissor/viewport) | ✅ | `glDepthFunc/Mask/Range`, `glStencilFunc/Op/Mask`, `glBlendFunc(/Separate)`, `glBlendEquation(/Separate)`, `glBlendColor`, `glViewport`, `glScissor` (box), scissor test, `glSampleCoverage`, `glMinSampleShading`, `glPolygonOffset` (all tracked, push-only-on-change) |
| §15/§16 (per-fragment ops / whole framebuffer) | 🟡 | `glClear`(+values), `glReadPixels`, color/depth clear, `glDrawBuffers`/`glReadBuffer` (tracked state, pushed on flush), `glBlitFramebuffer`+`glBlitNamedFramebuffer` (mask validated → `GL_INVALID_VALUE`, forwards after state flush/bind), `glInvalidateFramebuffer`/`glInvalidateSubFramebuffer`+`glInvalidateNamedFramebuffer*` (null-attachments / negative-dim `GL_INVALID_VALUE`, sub-rectangle form routed to backend), `glClearNamedFramebufferiv/uiv/fv/fi` (explicit clear values, DSA: no binding side effect). `glClampColor` implemented (SPEC §15.2.3). `GL_FRAMEBUFFER_SRGB` (SPEC §15.1.1) and `GL_SAMPLE_ALPHA_TO_COVERAGE` (SPEC §15.3.1) implemented as tracked capabilities (off by default, push-only-on-change). |
| §17 (fragment op details — alpha test, dither, logical op) | 🟡 | `glLogicOp` implemented (SPEC §17.3.4, capability-gated, push-only-on-change); `glColorMask` implemented (SPEC §17.3.6, tracked, push-only-on-change, `GL_COLOR_WRITEMASK` query); `glSampleCoverage` implemented (SPEC §17.3.6 multisample, tracked value+invert, push-only-on-change, `GL_SAMPLE_COVERAGE_VALUE`/`GL_SAMPLE_COVERAGE_INVERT` queries); `glEnable/glDisable(GL_DITHER)` implemented (SPEC §17.3.7, tracked capability, enabled by default, push-only-on-change, `GL_DITHER` query). `glStencilFuncSeparate`/`glStencilOpSeparate`/`glStencilMaskSeparate` (SPEC §17.3.3) track per-face state and push per-face only when a face differs, else a single combined push (SPEC §10); invalid face → `GL_INVALID_ENUM`. Alpha test removed-in-core |
| §18 (pixels: ReadPixels done; Copy/DrawPixels removed-compat) | 🟡 | `glReadPixels` implemented; `glPixelStorei` implemented |
| §4 / §19 Sync objects & fences | ✅ | `glFenceSync`, `glClientWaitSync`, `glWaitSync`, `glDeleteSync`, `glIsSync`, `glGetSynciv` implemented (frontend-owned `SyncObject`, SPEC §3/§20) |
| §4 / §20 Query objects (occlusion, timer, pipeline, primitive) | ✅ | `glGenQueries`, `glBeginQuery`/`BeginQueryIndexed`, `glEndQuery`, `glGetQueryiv`, `glGetQueryObjectiv`/`uiv`/`i64v`/`ui64v` implemented (SPEC §4/§19) |
| §21 (evaluators / selection / feedback / display lists / hints) | 🟡 | `glHint` implemented (SPEC §21.1.1; target/mode validated, pushed on flush via `GLStateSink::hint`). Evaluators/selection/feedback/display lists remain removed-in-core (correct). |
| §22 State queries (non-generic) | 🟡 | generic `glGet*` done (incl. `glGetInteger64v`, `glGetBooleani_v`/`glGetIntegeri_v`, `glGetStringi`, `glGetGraphicsResetStatus`); internal format queries `glGetInternalformativ` / `glGetInternalformati64v` (SPEC §22.3) read the backend's format support (mock returns a conservative documented default, GLES forwards to the driver); `glGetMultisamplefv` (SPEC §14.3.1) returns the indexed sample position, validating pname == SAMPLE_POSITION, null params, and index against the backend's sample count, then forwarding to the GLES driver (mock returns a fixed grid); program/shader reflection `glGetAttachedShaders` (SPEC §7.3.4) and `glGetShaderSource` (SPEC §7.3.7) return frontend-owned attachment/source data; program-interface summary `glGetProgramInterfaceiv` (SPEC §7.3.1) reports ACTIVE_RESOURCES from the backend's `programResourceCount` and the MAX_* sizing pnames (honest 0 without introspection; GLES forwards to the driver); `glGetBufferPointerv` / `glGetNamedBufferPointerv` (SPEC §6.1.1) return the mapped buffer pointer (DSA variant capability-gated by `DirectStateAccess`); object labels `glObjectLabel` / `glObjectPtrLabel` / `glGetObjectLabel` / `glGetObjectPtrLabel` (SPEC §22.2) are now implemented as a frontend-owned label store keyed by namespace+name and by sync pointer; remaining specific `glGet*` (e.g. `glGetPointerv`) not yet exposed |
| Shader stages | 🟡 | **Geometry, Tessellation** honestly **Unsupported** (no GLES equivalent; capability-gated, rejected at creation). **Compute** dispatch commands *and* compute shader objects/stages are implemented (native in GLES 3.1+; the mock mirrors that baseline): a compute program can be created, compiled, linked, and dispatched (see §7 row). Vertex + fragment + compute stages translate (desktop→GLSL ES via glslang + SPIRV-Cross for ES compute). |

## The implemented frontend surface (gl_api entry points)

  315 `gl*` entry points; 311 map to a spec command family (see Method). Listed
 alphabetically:

glActiveShaderProgram, glActiveTexture, glAttachShader, glBeginQuery, glBeginQueryIndexed,
glBeginTransformFeedback, glBindAttribLocation, glBindBuffer, glBindBufferBase, glBindBufferRange,
glBindFramebuffer, glBindProgramPipeline, glBindRenderbuffer, glBindSampler, glBindTexture, glBindTextureUnit,
glBindTextures, glBindTransformFeedback, glBindVertexArray, glBlendColor, glBlendEquation,
glBlendEquationSeparate, glBlendFunc, glBlendFuncSeparate, glBlitFramebuffer, glBlitNamedFramebuffer,
glBufferData, glBufferStorage, glBufferSubData, glClampColor, glClear, glClearBufferData, glClearBufferSubData,
glClearColor, glClearDepth, glClearDepthf, glClearNamedBufferData, glClearNamedBufferSubData,
glClearNamedFramebufferfi, glClearNamedFramebufferfv, glClearNamedFramebufferiv, glClearNamedFramebufferuiv,
glColorMask, glCompileShader, glCopyBufferSubData, glCopyTexImage1D, glCopyTexImage2D, glCreateFramebuffers,
glCreateRenderbuffers, glCreateTextures, glCreateVertexArrays, glCullFace, glDeleteBuffers,
glDeleteFramebuffers, glDeleteProgram, glDeleteProgramPipelines, glDeleteQueries, glDeleteRenderbuffers,
glDeleteSampler, glDeleteSamplers, glDeleteShader, glDeleteSync, glDeleteTextures, glDeleteTransformFeedback,
glDeleteTransformFeedbacks, glDeleteVertexArrays, glDepthFunc, glDepthMask, glDepthRange, glDepthRangef,
glDisable, glDisableVertexArrayAttrib, glDisableVertexAttribArray, glDisablei, glDispatchCompute,
glDispatchComputeIndirect, glDrawArrays, glDrawArraysIndirect, glDrawArraysInstanced, glDrawBuffers,
glDrawElements, glDrawElementsBaseVertex, glDrawElementsIndirect, glDrawElementsInstanced, glDrawRangeElements,
glEnable, glEnableVertexArrayAttrib, glEnableVertexAttribArray, glEnablei, glEndQuery, glEndQueryIndexed,
glEndTransformFeedback, glFinish, glFlush, glFramebufferRenderbuffer, glFramebufferTexture2D, glFrontFace,
glGenBuffers, glGenFramebuffers, glGenProgramPipelines, glGenQueries, glGenRenderbuffers, glGenSamplers,
glGenTextures, glGenTransformFeedbacks, glGenVertexArrays, glGenerateMipmap, glGenerateTextureMipmap,
glGetActiveAttrib, glGetActiveSubroutineName, glGetActiveSubroutineUniformName, glGetActiveSubroutineUniformiv,
glGetActiveUniform, glGetActiveUniformBlockName, glGetActiveUniformBlockiv, glGetAttachedShaders, glGetBooleanv, glGetBooleani_v,
glGetBufferParameteriv, glGetBufferParameteri64v, glGetNamedBufferParameteri64v, glGetNamedBufferParameteriv, glGetBufferPointerv, glGetNamedBufferPointerv, glGetBufferSubData, glGetCompressedTexImage, glGetCompressedTextureImage, glGetCompressedTextureSubImage, glGetTextureSubImage, glGetDoublev, glGetFloatv, glGetIntegerv, glGetInteger64v, glGetIntegeri_v, glGetInternalformativ, glGetInternalformati64v, glGetMultisamplefv, glGetNamedBufferSubData,
glGetNamedFramebufferAttachmentParameteriv, glGetFramebufferAttachmentParameteriv, glGetNamedFramebufferParameteriv, glGetFramebufferParameteriv, glGetGraphicsResetStatus,
glGetNamedRenderbufferParameteriv, glGetFragDataIndex, glGetFragDataLocation, glGetObjectLabel, glGetObjectPtrLabel, glGetRenderbufferParameteriv, glGetProgramBinary, glGetProgramInfoLog, glGetProgramInterfaceiv, glGetProgramPipelineInfoLog, glGetProgramPipelineiv,
glGetProgramResourceName, glGetProgramResourceiv, glGetQueryObjecti64v, glGetQueryObjectiv,
glProgramBinary, glProgramParameteri,
glGetQueryObjectui64v, glGetQueryObjectuiv, glGetQueryiv, glGetSamplerParameteriv, glGetShaderInfoLog, glGetShaderSource, glGetString, glGetStringi,
glGetSynciv, glGetTexImage, glGetTexParameterIiv, glGetTexParameterIuiv, glGetTexParameterfv,
glGetTexParameteriv, glGetTextureImage, glGetTextureLevelParameterfv, glGetTextureLevelParameteriv,
glGetTexLevelParameterfv, glGetTexLevelParameteriv,
glGetTextureParameterIiv, glGetTextureParameterIuiv, glGetTextureParameterfv, glGetTextureParameteriv, glGetTransformFeedbackVarying,
glGetUniformSubroutineuiv, glGetVertexAttribfv, glGetVertexAttribiv, glGetVertexAttribdv, glGetVertexAttribIiv, glGetVertexAttribIuiv, glGetVertexAttribPointerv, glGetVertexArrayiv, glGetVertexArrayIndexediv, glGetVertexArrayIndexed64v, glHint, glInvalidateBufferData,
glInvalidateBufferSubData, glInvalidateFramebuffer, glInvalidateNamedFramebufferData,
glInvalidateNamedFramebufferSubData, glInvalidateSubFramebuffer, glInvalidateTexImage, glInvalidateTexSubImage,
glLineWidth, glLinkProgram, glLogicOp, glMemoryBarrier, glMemoryBarrierByRegion, glMinSampleShading, glMultiDrawArrays, glMultiDrawElements,
glNamedFramebufferParameteri, glNamedFramebufferRenderbuffer, glNamedFramebufferTexture,
glNamedFramebufferTextureLayer, glNamedRenderbufferStorage, glNamedRenderbufferStorageMultisample,
glObjectLabel, glObjectPtrLabel, glPauseTransformFeedback, glPixelStorei, glPointSize, glPolygonMode, glPolygonOffset, glPrimitiveRestartIndex,
glProvokingVertex, glReadBuffer, glReadPixels, glRenderbufferStorage, glResumeTransformFeedback,
glSampleCoverage, glSampleMaski, glShaderBinary, glShaderSource, glStencilFunc,
glStencilFuncSeparate, glStencilMask, glStencilMaskSeparate, glStencilOp, glStencilOpSeparate, glTexBuffer,
glTexBufferRange, glTexImage1D, glTexImage2D, glTexImage2DMultisample, glTexImage3D, glTexImage3DMultisample,
glTexParameterIiv, glTexParameterIuiv, glTexParameterf, glTexParameterfv, glTexParameteri, glTexParameteriv,
glTexStorage1D, glTexStorage2D, glTexStorage2DMultisample, glTexStorage3D, glTexStorage3DMultisample,
glTexSubImage1D, glTexSubImage2D, glTexSubImage3D, glTextureBuffer, glTextureBufferRange,
glTextureParameterIiv, glTextureParameterIuiv, glTextureParameterf, glTextureParameterfv, glTextureParameteri,
glTextureParameteriv, glTextureStorage1D, glTextureStorage2D, glTextureStorage2DMultisample,
glTextureStorage3D, glTextureStorage3DMultisample, glTextureSubImage1D, glTextureSubImage2D,
glTextureSubImage3D, glUniform1f, glUniform1fv, glUniform1i, glUniform1iv, glUniform2f, glUniform2i,
glUniform3f, glUniform3i, glUniform4f, glUniform4i, glUniformMatrix4fv, glUniformSubroutinesuiv, glUseProgram,
glUseProgramStages, glValidateProgramPipeline, glVertexArrayAttribBinding, glVertexArrayAttribFormat,
glVertexArrayAttribIFormat, glVertexArrayAttribLFormat, glVertexArrayBindingDivisor,
glVertexArrayElementBuffer, glVertexArrayVertexBuffer, glVertexArrayVertexBuffers, glVertexAttrib1f,
glVertexAttrib1fv, glVertexAttrib2f, glVertexAttrib2fv, glVertexAttrib3f, glVertexAttrib3fv, glVertexAttrib4f,
glVertexAttrib4fv, glVertexAttribDivisor, glVertexAttribI4i, glVertexAttribI4iv, glVertexAttribI4ui,
glVertexAttribI4uiv, glVertexAttribPointer, glViewport, glWaitSync

(Plus the type/vector variants already present in `gl_api`: `glUniform1fv`,
`glUniform1iv`, `glUniformMatrix4fv`, `glGetString`, `glGetError`,
`glGenTransformFeedback`, `glBindTransformFeedback`, `glDeleteTransformFeedback`,
`glGenSampler`, `glDeleteSampler`, `glIsSampler`, `glGenTextures` family, etc.)

## Major unimplemented core areas (priority order for next steps)

This list reflects only what is **not yet done** as of 2026-08-28. Items marked
Done in earlier drafts (buffer/texture completeness, full DSA surface, queries &
sync, whole-framebuffer ops, draw expansion, program pipelines & subroutines,
rasterization controls) are now implemented and omitted here.

 1. **Geometry / tessellation shader stages** — honestly **Unsupported** (no GLES
    equivalent; `createShader` for these stages is rejected with
    `GL_INVALID_OPERATION`). Compute shaders are now implemented (native in GLES
    3.1+; the mock mirrors that baseline) — a compute program can be created,
    compiled, linked, and dispatched. Geometry/tessellation remain the only
    unsupported shader stages. (§7/§13)
2. ~~**Shader binaries**~~ — **Implemented** (SPEC §7.2/§19.1): `glShaderBinary` /
   `glProgramBinary` / `glGetProgramBinary` load and retrieve a precompiled binary
   blob; the frontend keeps the authoritative mirror (buffer-mirror pattern) and
   marks the program linked / shader compiled. (Removed from the gap list.)
3. **Texture completeness** — `GL_TEXTURE_RECTANGLE` (no GLES equivalent —
   honest capability gap) and `GetTexImage` multisample remain. Texture views
   (`glTextureView`) are now implemented (SPEC §8.19). Cube-map face and array
   targets are functional. (§8)
4. **Specific `glGet*` coverage** — many named-object / buffer / texture /
   internalformat parameter queries and program-interface reflection
   (`glGetActiveUniform`, `glGetActiveAttrib`, `glGetUniformBlockIndex`, …) are
   not yet exposed as dedicated entry points (generic `glGet*` is done). (§22)
5. **Client array legacy** — removed-in-core; intentionally out of scope.

## Verdict

YAGLT implements a **substantial and growing slice of the OpenGL 4.6 core
profile**: object lifecycle, a working vertex+fragment shader pipeline with
desktop→ES translation, uniforms, program pipelines, subroutines, queries &
sync, the core per-fragment/blend/depth/stencil/viewport/scissor state
(including separate stencil faces and indexed capabilities), transform feedback
scaffolding, sampler objects, the full DSA surface (texture/renderbuffer/
framebuffer/vertex-array), buffer completeness with a CPU mirror, whole-
framebuffer ops (blit/invalidate/clear), rasterization controls, and a broad set
of draws (instanced, multi-draw, primitive restart, indirect, base-vertex) — all
with dispatch, validation, and tests.

By the regenerated proxy (2026-08-28): **54.1% of the spec's declared command
prototypes** (311/571) and **~60.3% of the core profile** have a frontend entry
point; true entry-point coverage against the real ~700-entry GL core API is
roughly **42%**. This is materially more than the 2026-08-26 snapshot (then
~241/490 ≈ 49% declared, low-teens percent true), but YAGLT is **still not a
complete 4.6 core implementation**. The largest remaining gaps are the two
unsupported shader stages (geometry/tessellation), a few texture
targets, and broader specific `glGet*` coverage.

Per project policy (`docs/feature-matrix.md`), the **compatibility profile**
(deprecated fixed-function API) remains intentionally unimplemented and is gated
behind a majority of core being done.
