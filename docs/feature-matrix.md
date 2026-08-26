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
| ImmutableBufferStorage | Native | (planned) |
| TextureObjects | Native | `MockResourceFactory::createTexture` |
| ImmutableTextureStorage | Native | (planned) |
| TextureMultisample | Native | (planned) |
| ShaderObjects | Native | `MockResourceFactory::createShader` |
| ProgramObjects | Native | `MockResourceFactory::createProgram` |
| GeometryShaders | Unsupported | — |
| TessellationShaders | Unsupported | — |
| ComputeShaders | Unsupported | — |
| VertexArrayObjects | Native | `MockResourceFactory::createVertexArray` |
| InstancedRendering | Native | (planned) |
| FramebufferObjects | Native | `MockResourceFactory::createFramebuffer` |
| RenderbufferObjects | Native | `MockResourceFactory::createRenderbuffer` |
| UniformBufferObjects | Native | (planned) |
| ShaderStorageBufferObjects | Native | (planned) |
| TransformFeedback | Native | (planned) |
| ImageLoadStore | Unsupported | — |
| IndirectDrawing | Unsupported | — |
| ProgramPipelines | Emulated | (planned) |
| DirectStateAccess | Emulated | (planned) |

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
| State tracking | Implemented | `GLStateTracker` + `GLStateSink`; Mock & GLES backends flush via `Context::flushState()`/`glFlushState()`. Viewport (`glViewport`), scissor box (`glScissor`), and depth range (`glDepthRange`/`glDepthRangef`) tracked and pushed only on change; the scissor *test* is `GL_SCISSOR_TEST` capability (enable/disable). Blend tracking extended to separate RGB/alpha factors and equations (`glBlendFuncSeparate`/`glBlendEquationSeparate`) plus a constant `glBlendColor`, pushed independently of the func/eq category so only changed state reaches the driver (SPEC §10/§17.3). Verified by `viewport_scissor_test` + `blend_test` |
| Draw calls | Implemented | `glDrawArrays`/`glDrawElements` + instanced variants on `IGraphicsBackend`; `Context` flushes tracked state + bound VAO/attribs + program then issues the draw; no active program → `GL_INVALID_OPERATION`; instanced gated by `InstancedRendering` capability. Verified by `draw_test`/`shader_program_test` |
| Clear | Implemented | `glClearColor`/`glClearDepth`/`glClearDepthf` record the per-context clear values in `GLStateTracker` (pushed only on change, SPEC §10); `glClear` validates the mask (bits outside color/depth/stencil → `GL_INVALID_VALUE`) then flushes tracked state and issues the native clear via `IGraphicsBackend::clear`. `glFlush`/`glFinish` forward to the backend command stream (SPEC §2.1). Mock records every call; GLES drives `glClearColor`/`glClearDepthf`/`glClear`/`glFlush`/`glFinish` through `GLESLib`. Verified by `clear_test` |
| Framebuffer readback | Implemented | `glReadPixels` flushes tracked state then reads the bound framebuffer via `IGraphicsBackend::readPixels`; non-positive width/height → `GL_INVALID_VALUE`. Mock records the call; GLES drives `glReadPixels` through `GLESLib`. Verified by `readpixels_test` |
| Shaders | Implemented | `glCreateShader`/`glShaderSource`/`glCompileShader`/`glGetShaderiv`; desktop GLSL translated via `IShaderCompiler` before the backend compiles. Capability-gated: `createShader` maps each stage to its `Feature` (vertex/fragment→ShaderObjects, geometry→GeometryShaders, tessellation→TessellationShaders, compute→ComputeShaders). Unsupported stage → `GL_INVALID_OPERATION`; unknown stage → `GL_INVALID_ENUM`. `compileShader` rejects GLSL versions beyond the translatable ceiling (desktop > 4.60, ES > 3.20) before translation, reporting via COMPILE_STATUS + info log. Verified by `shader_program_test`/`shader_stage_test`/`glsl_version_check_test`/`gles_e2e_program_test` |
| Programs | Implemented | `glCreateProgram`/`glAttachShader`/`glLinkProgram`/`glGetProgramiv`/`glGetAttribLocation`; link status gated by ProgramObjects; name → native id mapping for bind-at-draw. `glGetProgramiv` covers LINK_STATUS, DELETE_STATUS, ATTACHED_SHADERS, INFO_LOG_LENGTH, ACTIVE_UNIFORMS/ATTRIBUTES/UNIFORM_BLOCKS (delegated to backend; mock 0). Verified by `shader_program_test`/`gles_e2e_program_test`/`shader_program_query_test` |
| Shader/Program queries | Implemented | `glGetShaderiv` covers SHADER_TYPE, COMPILE_STATUS, DELETE_STATUS, SHADER_SOURCE_LENGTH, INFO_LOG_LENGTH; `glGetProgramiv` covers LINK_STATUS, DELETE_STATUS, ATTACHED_SHADERS, INFO_LOG_LENGTH, ACTIVE_UNIFORMS/ATTRIBUTES/UNIFORM_BLOCKS. Unknown pname → `GL_INVALID_ENUM`, unknown object → `GL_INVALID_OPERATION` (SPEC §7.3/§7.14). `glGetShaderInfoLog`/`glGetProgramInfoLog` copy the log (nul-terminated, length excludes nul; bufSize 0 writes nothing). Verified by `shader_program_query_test`/`infolog_test` |
| Transform feedback (SPEC §13.3) | Implemented | `glGenTransformFeedback`/`glBindTransformFeedback`/`glDeleteTransformFeedback`/`glBeginTransformFeedback`/`glEndTransformFeedback`/`glPauseTransformFeedback`/`glResumeTransformFeedback`; capability-gated (TransformFeedback); begin/end/pause/resume validated (already-active / not-active / not-paused → `GL_INVALID_OPERATION`); forwarded to backend `BackendTransformFeedback`. Verified by `transform_feedback_test` |
| Vertex attributes | Implemented | `glEnableVertexAttribArray`/`glDisableVertexAttribArray`/`glVertexAttribPointer` recorded on the bound VAO (requires a bound VAO) and pushed via `GLStateSink` at draw/flush. Verified by `shader_program_test`/`gles_e2e_program_test` |
| GLES backend | Partial (runtime) | `src/backend/gles`; dlopen EGL/GLES, surfaceless EGL, capability detection, real program/shader compile + link. Real on Android/Mesa-GLES; initializes=false honestly where no driver |
| Shader translation | Emulated (ES) | `GLESShaderCompiler` compiles GLSL ES on driver; `TranslatingGLESShaderCompiler` runs desktop GLSL → glslang → SPIRV-Cross → GLSL ES 3.10. Uniform/storage blocks auto-bound via 420pack. Verified end-to-end on Mesa. |
| Vulkan backend | Not implemented | interfaces reserved in `src/backend/vulkan` |

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

## Legend

- `Native` — backend provides directly.
- `Emulated` — provided via shader/CPU/resource emulation.
- `Unsupported` — not available; reported honestly.
- `Not implemented` — designed for, not yet coded.
