# Feature Matrix

Tracks OpenGL feature → support status → implementation location → tests.
Status vocabulary: `Native`, `Extension`, `Emulated`, `Unsupported`,
`Not implemented`.

> Honesty rule: a feature is only marked supported when an implementation and
> tests exist. Phase 1 establishes infrastructure only; the frontend API is not
> yet exposed, so most features are `Not implemented` at the OpenGL level even
> where the mock backend could theoretically provide them.

## Backend capability profile (Mock backend, headless Linux)

This is the capability table the **mock backend** populates for tests. It is a
beliefable GLES 3.1-like baseline used to exercise the abstraction.

| Feature | Mock status | Implementation |
|---------|-------------|----------------|
| BufferObjects | Native | `MockResourceFactory::createBuffer` |
| ImmutableBufferStorage | Native | `Context::bufferStorage` (SPEC §6, write-once immutable store) |
| TextureObjects | Native | `MockResourceFactory::createTexture` |
| ImmutableTextureStorage | Native | (planned) |
| TextureMultisample | Native | (planned) |
| ShaderObjects | Native | `MockResourceFactory::createShader` |
| ProgramObjects | Native | `MockResourceFactory::createProgram` |
| GeometryShaders | Unsupported | — |
| TessellationShaders | Unsupported | — |
| ComputeShaders | Unsupported | — |
| VertexArrayObjects | Native | `MockResourceFactory::createVertexArray`; DSA vertex-array surface (`glCreateVertexArrays`, `glVertexArrayElementBuffer`, `glVertexArrayVertexBuffer(s)`, `glVertexArrayAttribFormat/IFormat/LFormat`, `glVertexArrayAttribBinding`, `glVertexArrayBindingDivisor`, `glEnable/DisableVertexArrayAttrib`) implemented under `DirectStateAccess` (Emulated), replayed via the unified flush path |
| InstancedRendering | Native | (planned) |
| FramebufferObjects | Native | `MockResourceFactory::createFramebuffer` |
| RenderbufferObjects | Native | `MockResourceFactory::createRenderbuffer` |
| UniformBufferObjects | Native | (planned) |
| ShaderStorageBufferObjects | Native | (planned) |
| TransformFeedback | Native | (planned) |
| ImageLoadStore | Unsupported | — |
| IndirectDrawing | Unsupported | — |
| ProgramPipelines | Emulated | Frontend pipeline objects + `glGen/Delete/IsProgramPipeline`, `glBindProgramPipeline`, `glCreateShaderProgramv`, `glUseProgramStages`, `glActiveShaderProgram`, `glGetProgramPipelineiv`, `glValidateProgramPipeline`, `glGetProgramPipelineInfoLog`. The bound pipeline is forwarded to the backend via `GLStateSink::bindProgramPipeline`; GLES consumes it only where separable programs exist (ES 3.1-class / `GL_EXT_separate_shader_objects`), else `Unsupported` honestly. Per-stage draw consumption requires backend separable-program wiring (not yet modeled). |
| DirectStateAccess | Emulated | `Context::bindTextureUnit`/`bindTextures` (SPEC §2.1) |
| SamplerObjects | Native | `MockResourceFactory::createSampler` |
| Queries | Native | `MockResourceFactory::createQuery` |
| SyncObjects | Native | `Context::fenceSync` (frontend-owned `SyncObject`) |

## Honest-Unsupported shader stages (this session)

`createShader` now rejects stages the backend cannot provide, per SPEC §8/§19.
Mapping: `GL_VERTEX_SHADER`/`GL_FRAGMENT_SHADER` → `ShaderObjects`;
`GL_GEOMETRY_SHADER` → `GeometryShaders`;
`GL_TESS_CONTROL_SHADER`/`GL_TESS_EVALUATION_SHADER` → `TessellationShaders`;
`GL_COMPUTE_SHADER` → `ComputeShaders`. Unknown type → `GL_INVALID_ENUM`.
Emulation path planned (see Emulation roadmap) but not yet implemented, so these
remain honestly reported as `Unsupported` rather than faked.


## OpenGL-facing support (frontend)

| Subsystem | Status | Notes |
|-----------|--------|-------|
| OpenGL 4.6 API entry points | Partial | `glcompat` dispatch for buffers/textures/RBO/FBO/VAO gen-bind-delete, glBufferData, glGetError, draw calls (tests pass via mock) |
| Indexed buffer bindings | Implemented | `glBindBufferBase`/`glBindBufferRange` routed through `Context`; capability-guarded (UBO/SSBO/transform-feedback). Native on GLES backend, recorded on mock. Verification: `ubo_ssbo_test`, `capabilities_test` |
| Shader translation | Implemented (desktop→ES) | glslang + SPIRV-Cross; uniform/storage blocks get auto-assigned `binding=` (420pack) so desktop GLSL 330 translates to GLSL ES 3.10. Verified by `shader_translate_test` + Mesa e2e compile. |
| Frontend Context (name gen / bind / delete) | Partial | `Context` in `include/glcompat/frontend/context.hpp`; tested via mock |
| Object model (Buffer/Texture/RBO/FBO/VAO) | Partial | `src/frontend` objects hold `unique_ptr<BackendX>`; gen/bind/delete done |
| Error handling (GLError) | Partial | `getError`/`setError`; InvalidOperation on bad bind |
| State tracking | Implemented | `GLStateTracker` + `GLStateSink`; Mock & GLES backends flush via `Context::flushState()`/`glFlushState()`. Viewport (`glViewport`), scissor box (`glScissor`), and depth range (`glDepthRange`/`glDepthRangef`) tracked and pushed only on change; the scissor *test* is `GL_SCISSOR_TEST` capability (enable/disable). Blend tracking extended to separate RGB/alpha factors and equations (`glBlendFuncSeparate`/`glBlendEquationSeparate`) plus a constant `glBlendColor`, pushed independently of the func/eq category so only changed state reaches the driver (SPEC §10/§17.3). Stencil state (`glStencilFunc`/`glStencilOp`/`glStencilMask`) is tracked and pushed as a single stencil category. Verified by `viewport_scissor_test` + `blend_test` + `stencil_test` |
| State queries (`glGet*`) | Implemented | `glGetBooleanv`/`glGetIntegerv`/`glGetFloatv`/`glGetDoublev`/`glIsEnabled` read the frontend-owned tracked state from `GLStateTracker` (SPEC §22). Covers tracked caps (BLEND/CULL_FACE/DEPTH_TEST/STENCIL_TEST/SCISSOR_TEST), VIEWPORT, SCISSOR_BOX, BLEND_SRC/DST (RGB/alpha), BLEND_EQUATION (RGB/alpha), BLEND_COLOR, DEPTH_FUNC, DEPTH_WRITEMASK, DEPTH_RANGE, COLOR/DEPTH_CLEAR_VALUE, CULL_FACE_MODE, FRONT_FACE, CURRENT_PROGRAM. Unknown pname -> GL_INVALID_ENUM; null buffer -> GL_INVALID_VALUE; untracked cap in glIsEnabled -> GL_INVALID_ENUM. No backend round-trip (SPEC §10). Verified by `getstate_test` |
| Draw calls | Implemented | `glDrawArrays`/`glDrawElements` + instanced variants on `IGraphicsBackend`; `Context` flushes tracked state + bound VAO/attribs + program then issues the draw; no active program → `GL_INVALID_OPERATION`; instanced gated by `InstancedRendering` capability. Verified by `draw_test`/`shader_program_test` |
| Buffer object completeness (SPEC §6) | Implemented | `glBufferSubData`, `glBufferStorage` (immutable, capability-gated by `ImmutableBufferStorage`), `glCopyBufferSubData`, `glGetBufferParameteriv` (size/usage/access/immutable/mapped), `glMapBuffer`/`glMapBufferRange`/`glUnmapBuffer`. The frontend `BufferObject` owns a CPU data store that is the authoritative mirror, so subdata/copy/map/query semantics are fully defined on the mock; real backends additionally receive the data via `BackendBuffer` (GLES forwards `glBufferSubData`/`glBufferStorage`/`glCopyBufferSubData`/`glMapBufferRange`). Bounds/validation per SPEC. Verified by `buffer_completeness_test` |
| Clear | Implemented | `glClearColor`/`glClearDepth`/`glClearDepthf` record the per-context clear values in `GLStateTracker` (pushed only on change, SPEC §10); `glClear` validates the mask (bits outside color/depth/stencil → `GL_INVALID_VALUE`) then flushes tracked state and issues the native clear via `IGraphicsBackend::clear`. `glFlush`/`glFinish` forward to the backend command stream (SPEC §2.1). Mock records every call; GLES drives `glClearColor`/`glClearDepthf`/`glClear`/`glFlush`/`glFinish` through `GLESLib`. Verified by `clear_test` |
| Framebuffer readback | Implemented | `glReadPixels` flushes tracked state then reads the bound framebuffer via `IGraphicsBackend::readPixels`; non-positive width/height → `GL_INVALID_VALUE`. Mock records the call; GLES drives `glReadPixels` through `GLESLib`. Verified by `readpixels_test` |
| Shaders | Implemented | `glCreateShader`/`glShaderSource`/`glCompileShader`/`glGetShaderiv`; desktop GLSL translated via `IShaderCompiler` before the backend compiles. Capability-gated: `createShader` maps each stage to its `Feature` (vertex/fragment→ShaderObjects, geometry→GeometryShaders, tessellation→TessellationShaders, compute→ComputeShaders). Unsupported stage → `GL_INVALID_OPERATION`; unknown stage → `GL_INVALID_ENUM`. `compileShader` rejects GLSL versions beyond the translatable ceiling (desktop > 4.60, ES > 3.20) before translation, reporting via COMPILE_STATUS + info log. Verified by `shader_program_test`/`shader_stage_test`/`glsl_version_check_test`/`gles_e2e_program_test` |
| Programs | Implemented | `glCreateProgram`/`glAttachShader`/`glLinkProgram`/`glGetProgramiv`/`glGetAttribLocation`; link status gated by ProgramObjects; name → native id mapping for bind-at-draw. `glGetProgramiv` covers LINK_STATUS, DELETE_STATUS, ATTACHED_SHADERS, INFO_LOG_LENGTH, ACTIVE_UNIFORMS/ATTRIBUTES/UNIFORM_BLOCKS (delegated to backend; mock 0). Verified by `shader_program_test`/`gles_e2e_program_test`/`shader_program_query_test` |
| Shader/Program queries | Implemented | `glGetShaderiv` covers SHADER_TYPE, COMPILE_STATUS, DELETE_STATUS, SHADER_SOURCE_LENGTH, INFO_LOG_LENGTH; `glGetProgramiv` covers LINK_STATUS, DELETE_STATUS, ATTACHED_SHADERS, INFO_LOG_LENGTH, ACTIVE_UNIFORMS/ATTRIBUTES/UNIFORM_BLOCKS. Unknown pname → `GL_INVALID_ENUM`, unknown object → `GL_INVALID_OPERATION` (SPEC §7.3/§7.14). `glGetShaderInfoLog`/`glGetProgramInfoLog` copy the log (nul-terminated, length excludes nul; bufSize 0 writes nothing). Verified by `shader_program_query_test`/`infolog_test` |
| Transform feedback (SPEC §13.3) | Implemented | `glGenTransformFeedback`/`glBindTransformFeedback`/`glDeleteTransformFeedback`/`glBeginTransformFeedback`/`glEndTransformFeedback`/`glPauseTransformFeedback`/`glResumeTransformFeedback`; capability-gated (TransformFeedback); begin/end/pause/resume validated (already-active / not-active / not-paused → `GL_INVALID_OPERATION`); forwarded to backend `BackendTransformFeedback`. Verified by `transform_feedback_test` |
| Sampler objects (SPEC §8.2) | Implemented | `glGenSampler`/`glGenSamplers`/`glBindSampler`/`glDeleteSampler`/`glDeleteSamplers`/`glIsSampler`/`glSamplerParameteri`/`glGetSamplerParameteriv`. Capability-gated (SamplerObjects; Native on GLES 3.0+). `bindSampler(unit, sampler)` binds a sampler to a texture unit, recorded in `GLStateTracker` and pushed via `GLStateSink::bindSampler` only when the binding changes (SPEC §10). Out-of-range unit → `GL_INVALID_VALUE`; ungenerated name → `GL_INVALID_OPERATION`. `samplerParameteri` accepts only the scalar sampler pnames (table 23.23); non-scalar/unknown → `GL_INVALID_ENUM`. `GL_SAMPLER_BINDING` query reflects the sampler on the active texture unit. Backend: `MockSampler` records params; `GLESBackendSampler` drives `glGenSamplers`/`glBindSampler`/`glSamplerParameteri` (resolved optionally). Verified by `sampler_test` |
| Query objects (SPEC §4 / §19) | Implemented | `glGenQuery`/`glGenQueries`/`glDeleteQuery`/`glDeleteQueries`/`glIsQuery`/`glBeginQuery`/`glEndQuery`/`glBeginQueryIndexed`/`glEndQueryIndexed`/`glGetQueryiv`/`glGetQueryObjectiv`/`glGetQueryObjectuiv`/`glGetQueryObjecti64v`/`glGetQueryObjectui64v`. Capability-gated (Queries; Native on GLES 3.0+). One active query per target; begin already-active / end with none active / begin ungenerated id → `GL_INVALID_OPERATION`; indexed variants require a counter target (PRIMITIVES_GENERATED / TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) else `GL_INVALID_ENUM`. `getQueryiv` CURRENT_QUERY reads the active id; results read the cached backend value (frontend-owned, SPEC §10). Backend: `MockQuery` records begin/end and a test-injected result; `GLESBackendQuery` drives `glGenQueries`/`glBeginQuery`/`glEndQuery`/`glGetQueryObjectuiv`(ui64v) resolved optionally. Verified by `query_sync_test` |
| Sync objects (SPEC §4 / §20) | Implemented | `glFenceSync`/`glClientWaitSync`/`glWaitSync`/`glDeleteSync`/`glIsSync`/`glGetSynciv`. Capability-gated (SyncObjects; Native on GLES 3.0+). `fenceSync` returns an opaque `GLsync` (frontend-owned `SyncObject`; the raw pointer never reaches the backend, SPEC §3) and flushes the backend so the fence will be signaled. Unknown condition → `GL_INVALID_ENUM`. `getSynciv` reports SYNC_STATUS / SYNC_CONDITION / SYNC_FLAGS; non-sync → `GL_INVALID_OPERATION`, `waitSync` non-sync → `GL_INVALID_VALUE`. `deleteSync` on a non-sync is a silent no-op. Verified by `query_sync_test` |
| Vertex attributes | Implemented | `glEnableVertexAttribArray`/`glDisableVertexAttribArray`/`glVertexAttribPointer` recorded on the bound VAO (requires a bound VAO) and pushed via `GLStateSink` at draw/flush. Verified by `shader_program_test`/`gles_e2e_program_test` |
| Texture units / active texture | Implemented | `glActiveTexture(GL_TEXTURE0+i)` selects the active unit (out-of-range → `GL_INVALID_ENUM`); `glBindTexture(target, tex)` binds to (active unit, target). The tracker pushes per-unit bindings via `GLStateSink` at flush so each unit's texture reaches the driver (SPEC §2.1/§10). `glGetIntegerv(GL_ACTIVE_TEXTURE)` and `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` query the tracked state. Also fixed a latent backend bug: the GLES `texImage2D`/`texParameteri` now bind the texture before the driver call, so multi-texture uploads target the correct texture. Verified by `texture_unit_test` + `gles_e2e_*` (Mesa). |
| DSA texture binding (SPEC §2.1) | Implemented | `glBindTextureUnit(unit, texture)` binds without touching the active-texture selector; `glBindTextures(first, count, target, textures)` binds an array to consecutive units for one target. Both capability-gated by `DirectStateAccess` (Emulated in the mock). Out-of-range unit → `GL_INVALID_VALUE`; ungenerated name → `GL_INVALID_OPERATION`; invalid target → `GL_INVALID_ENUM`. Stored in the per-unit binding table so the existing `GLStateSink` flush pushes them (switching the driver active unit only when needed, SPEC §10). `glBindTextureUnit(unit, 0)` clears the whole unit. Verified by `dsa_texture_test`. |
| DSA renderbuffer + framebuffer surface (SPEC §9.2) | Implemented | `glCreateRenderbuffers`/`glNamedRenderbufferStorage`/`glNamedRenderbufferStorageMultisample`/`glGetNamedRenderbufferParameteriv`; `glCreateFramebuffers`/`glNamedFramebufferRenderbuffer`/`glNamedFramebufferTexture`/`glNamedFramebufferTextureLayer`/`glCheckNamedFramebufferStatus`/`glNamedFramebufferParameteri`/`glGetNamedFramebufferParameteriv`/`glGetNamedFramebufferAttachmentParameteriv`; `glBlitNamedFramebuffer`/`glInvalidateNamedFramebufferData`/`glInvalidateNamedFramebufferSubData`/`glClearNamedFramebufferiv/uiv/fv/fi`. All capability-gated by `DirectStateAccess` (Emulated: YAGLT drives the named object's backend resource directly, no global bind needed); named blit/invalidate/clear bind the named framebuffer(s) to the driver and restore the tracked binding (DSA: no side effect). Honest validation: negative dims/samples → `GL_INVALID_VALUE`; ungenerated name → `GL_INVALID_OPERATION`; null query pointer → `GL_INVALID_VALUE`. Verified by `dsa_named_framebuffer_test`. |
| Texture parameter queries (SPEC §8.1) | Implemented | `glGetTexParameteriv(target, pname, params)` reads the bound texture for `target`; `glGetTextureParameteriv(texture, pname, params)` is the DSA variant (capability-gated by `DirectStateAccess`). Both read the frontend-tracked `params` map (written by `texParameteri`); null `params` → `GL_INVALID_VALUE`, missing texture → `GL_INVALID_OPERATION`, unknown pname returns 0 (GL default). Verified by `texparam_query_test`. |
| Rasterization controls (SPEC §11) | Implemented | `glPolygonMode(face, mode)` tracks per-face fill/line/point mode (frontend `PolygonModeState`, default `GL_FILL`); `GL_POLYGON_MODE` query returns `{front, back}`. `glSampleMaski(maskNumber, mask)` writes one 32-bit word of `GL_SAMPLE_MASK` state (`kMaxSampleMaskWords = 2`); out-of-range `maskNumber` → `GL_INVALID_VALUE`. `glMinSampleShading(value)` tracks the multisample shading rate; out-of-range value → `GL_INVALID_VALUE`; `GL_MIN_SAMPLE_SHADING` query. All three are tracked in `GLStateTracker` (`MultisampleRasterState`/`PolygonModeState`), pushed via `GLStateSink::polygonMode`/`sampleMaski`/`minSampleShading`; `sampleMaski` pushes only changed words. GLES backend honestly no-ops (no polygon mode / sample mask / min-sample-shading in GLES). Validation: `glPolygonMode` bad face/mode → `GL_INVALID_ENUM`. Verified by `rasterization_control_test` |
| GLES backend | Partial (runtime) | `src/backend/gles`; dlopen EGL/GLES, surfaceless EGL, capability detection, real program/shader compile + link. Real on Android/Mesa-GLES; initializes=false honestly where no driver |
| Shader translation | Emulated (ES) | `GLESShaderCompiler` compiles GLSL ES on driver; `TranslatingGLESShaderCompiler` runs desktop GLSL → glslang → SPIRV-Cross → GLSL ES 3.10. Uniform/storage blocks auto-bound via 420pack. Verified end-to-end on Mesa. |
| Vulkan backend | Not implemented | interfaces reserved in `src/backend/vulkan` |
| Structured logging (SPEC §20) | Implemented | `glcompat::log(category, level)` streaming API + `Logger` (`include/glcompat/core/log.hpp`, `src/core/log.cpp`); categories CORE/STATE/RESOURCE/SHADER/BACKEND/GLES/VULKAN/PLATFORM/EMULATION, levels Debug/Info/Warn/Error; configurable via `setStream`/`setLevel`/`enableCategory` and env `YAGLT_LOG_LEVEL`/`YAGLT_LOG_CATS`; `CapabilityTable::report()` logs selected feature implementations + activated fallbacks. Verified by `log_test` |

## Emulation roadmap (future heavy lifting)

Geometry shaders, tessellation (hull/domain), and compute shaders are currently
`Unsupported` (honestly reported). They MUST eventually be supported — the goal is
"not feature reduction" (SPEC §1). Planned approach: **heavy emulation**, not native
backend passthrough, because GLES has no geometry/tessellation/compute stages:

- **Geometry shaders** → expand primitives on CPU or via a vertex/fragment
  expansion pass; emit extra instances, feed a transformed vertex stream back
  through the GLES pipeline.
- **Tessellation** → CPU or compute-free evaluation of the patch + tessellation
  factors; generate the subdivided vertex grid and feed it as a draw.
- **Compute** → emulate via fragment-shader "transform feedback"-style passes or
  multi-pass rasterization into textures (GPU-driven), with a CPU fallback for
  platforms lacking the needed GLES features.

All three require the capability system to select the emulation path once, keeping
the scattered-version-branch rule (SPEC §4/§18) intact. Tracked as a phase-3+
milestone; not started.

## Compatibility Profile (deprecated fixed-function API)

Status: `Not implemented` (planned). This covers the OpenGL 4.6 **Compatibility
Profile** — the deprecated immediate-mode / fixed-function subset: `glBegin`/
`glEnd` and the `glVertex*`/`glColor*`/`glNormal*`/`glTexCoord*`/`glEdgeFlag*`
calls, client vertex arrays, display lists (`glNewList`/`glCallList`/`glGenLists`),
the matrix stack (`glMatrixMode`/`glLoadMatrix`/`glPushMatrix`/`glTranslate`/
`glRotate`/`glScale`), `glLight`/`glMaterial`/`glTexGen`/`glClipPlane`/`glFog`,
`GL_QUADS`/`GL_POLYGON`, wide `glLineWidth`/`glPointSize`, `glPolygonMode(POINT)`,
selection/feedback, accumulation/aux buffers, and evaluators.

### Activation: opt-in only

The compatibility profile is **opt-in, never on by default**. It must NOT be
exposed unless, under EGL, the user explicitly requests a compatibility profile
(e.g. an EGL context attribute / config that selects the compat profile, or an
explicit YAGLT configuration flag). A normal core-style or ES caller must not see
compat entry points or compat behavior unless that explicit selection happened.

This decision is resolved **once at context / EGL init**, behind the same
capability-abstraction layer used for Android-SDK and GLES-version decisions
(SPEC §4/§5/§18). The frontend asks a single "compat profile active?" question
from the capability system; scattered `if (compat)` branches must not appear
throughout the rendering code. Until that flag is set, compat calls either are
not dispatched or report `GL_INVALID_OPERATION` honestly.

**Implementation gating.** Compat-profile work must not start prematurely. It
should only be implemented once a **majority of the core profile is already
implemented** and the remaining core work has become harder and takes longer to
land than the compat emulation would. In other words, compat is the phase that
comes *after* core is substantially done and core progress has slowed — not
something pursued in parallel while core is still the cheaper, higher-value path.


### Emulation strategy (SPEC §1 / §12)

Backend-native compat support does not exist on GLES, so this is emulation, not
passthrough. Use the standard **record-then-replay-into-a-shader** approach:

1. **Collect.** During `glBegin`…`glEnd` (and the equivalent client-array and
   display-list captures) the frontend accumulates per-vertex attribute data
   (position, color, normal, texcoords, edge flags, …) into internal CPU buffers.
   The matrix stack and current color/material/lighting/texgen state are tracked on
   the frontend alongside the vertices.
2. **Replay as core draw.** At `glEnd` (or list playback / draw) the collected
   vertex data is uploaded as a vertex buffer and issued through the existing core
   draw path (`drawArrays`/`drawElements`), translating deprecated primitives
   (`GL_QUADS`/`GL_POLYGON`) into triangle lists.
3. **Special injected shader.** The deprecated fixed-function shading
   (lighting, texgen, fog, material/color handling, matrix application) is
   reproduced by a **special GLSL shader generated by the frontend**. That shader
   is compiled through the same `IShaderCompiler` path already used for desktop→ES
   translation (glslang + SPIRV-Cross), so compat emulation reuses the existing
   shader pipeline rather than inventing a new one.

### GLSL shaders in the tree are fine

There is **no policy problem with shipping GLSL shader sources** in the codebase.
YAGLT already vendors and generates GLSL (the glslang + SPIRV-Cross desktop→ES
translation). The compat emulation shaders are simply more generated GLSL; having
`.glsl`/generated shader sources in the repository is expected and acceptable.


## Legend

- `Native` — backend provides directly.
- `Emulated` — provided via shader/CPU/resource emulation.
- `Unsupported` — not available; reported honestly.
- `Not implemented` — designed for, not yet coded.
