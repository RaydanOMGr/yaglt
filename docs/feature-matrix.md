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

## OpenGL-facing support (frontend)

| Subsystem | Status | Notes |
|-----------|--------|-------|
| OpenGL 4.6 API entry points | Partial | `glcompat` dispatch for buffers/textures/RBO/FBO/VAO gen-bind-delete, glBufferData, glGetError (tests pass via mock) |
| Shader translation | Not implemented | `IShaderCompiler` exists; pipeline pending |
| Frontend Context (name gen / bind / delete) | Partial | `Context` in `include/glcompat/frontend/context.hpp`; tested via mock |
| Object model (Buffer/Texture/RBO/FBO/VAO) | Partial | `src/frontend` objects hold `unique_ptr<BackendX>`; gen/bind/delete done |
| Error handling (GLError) | Partial | `getError`/`setError`; InvalidOperation on bad bind |
| State tracking | Not implemented | `src/state` reserved |
| Shader translation | Not implemented | `IShaderCompiler` exists; pipeline pending |
| GLES backend | Partial (runtime) | `src/backend/gles`; dlopen EGL/GLES, surfaceless EGL, capability detection. Real on Android/Mesa-GLES; initializes=false honestly where no driver |
| Shader translation | Emulated (ES) / Unsupported (desktop) | `GLESShaderCompiler` compiles GLSL ES on driver; no desktop→ES translation (glslang absent) |
| Vulkan backend | Not implemented | interfaces reserved in `src/backend/vulkan` |

## Legend

- `Native` — backend provides directly.
- `Emulated` — provided via shader/CPU/resource emulation.
- `Unsupported` — not available; reported honestly.
- `Not implemented` — designed for, not yet coded.
