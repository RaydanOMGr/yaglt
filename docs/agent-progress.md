# Development Journal

Persistent, version-controlled progress record. Updated after meaningful
milestones, architectural decisions, and before ending a session.

## Current Status

Current milestone: Phase 3 — Core Rendering (API surface + GLES backend prep)
Overall status: Early implementation (foundation + object model + GL dispatch)
Last updated: 2026-08-26
Known major blockers:
- OpenGL 4.6 public API entry points (glGenBuffers etc.) not yet exposed.
- Real GLES backend not yet implemented (interfaces reserved).
- Shader translation pipeline not yet implemented.

## Completed

- [x] Repository setup + local Git identity (AI Graphics Agent)
  - Git initialized; project-local user configured (no global changes).
- [x] CMake build system (foundation)
  - Top-level project, `src` and `tests` subdirs, tests via CTest.
  - Options: `YAGLT_BUILD_TESTS`, `YAGLT_ENABLE_SANITIZERS`.
- [x] Backend abstraction interfaces
  - `IGraphicsBackend`, `IResourceFactory`, `IShaderCompiler`,
    `ICapabilities`, `IPlatformCapabilities`, opaque backend resources.
  - Headers under `include/glcompat/core`.
- [x] Capability system
  - `FeatureSupport`, `Feature` enum, `CapabilityTable` impl.
  - `tests/unit/capabilities_test.cpp` covers default/set/query/name.
- [x] Mock backend (headless test backend)
  - `MockBackend`, `MockResourceFactory`, `MockShaderCompiler`,
    `MockPlatformCapabilities`, mock capability profile.
  - `tests/unit/backend_test.cpp` covers api, features, resource ids,
    shader compiler, platform.
- [x] Headless Linux platform capabilities
  - `LinuxCapabilities` under `src/platform/linux`.
- [x] Self-contained test framework
  - Header-only, no external deps (`tests/framework/test_framework.hpp`).
- [x] Frontend object model (Phase 2)
  - `Context` with name gen / bind tracking / delete-with-bound-reset.
  - Object classes (Buffer/Texture/RBO/FBO/VAO) own `unique_ptr<BackendX>`.
  - `GLError` getError/setError; InvalidOperation on binding ungenerated name.
  - `tests/unit/object_test.cpp` covers gen/bind/delete/error vs mock.
- [x] Public GL dispatch surface (`glcompat` namespace)
  - `gl_api.hpp/.cpp`: glGen*/glBind*/glDelete* for buffers, textures,
    RBO, FBO, VAO; glBufferData; glGetError; current-context registry.
  - Self-contained `gl_types.hpp` GL type/constant layer (values match spec).
  - `tests/unit/gl_api_test.cpp` exercises dispatch + error via mock.
- [x] Vendored Khronos native headers
  - `include/GL`, `GLES`, `GLES2`, `GLES3`, `EGL`, `vulkan` added; added
    missing `KHR/khrplatform.h` so glext/GLES3/EGL compile.
  - Used by backends only; frontend keeps its own lightweight type layer.

## In Progress

- [ ] Real GLES backend foundation (consume `GLES3`/`EGL` headers; headless
      surfaceless context on Linux/Mesa; capability detection).
- [ ] State tracking subsystem (`src/state`).
- [ ] Shader translation pipeline behind `IShaderCompiler`.

## TODO

- [ ] P0: Implement OpenGL 4.6 frontend API entry points (Phase 2 start).
- [ ] P0: Implement core object model (buffers, textures, VAO, FBO, RBO).
- [ ] P1: Implement GLES backend foundation + headless EGL validation.
- [ ] P1: Implement shader translation pipeline behind `IShaderCompiler`.
- [ ] P1: Android platform capabilities + SDK 21 fallback abstraction.
- [ ] P2: Capability-driven emulation selection scaffolding.
- [ ] P3: Structured logging categories (CORE/STATE/RESOURCE/...).

## Known Issues

- Mock backend capability profile is a fixed beliefable baseline, not derived
  from real backend detection. Real backends must populate `CapabilityTable`
  from version/extension detection at init.

## Architecture Decisions

Decision: Backend-native handles never appear in the generic frontend API.
Reason: Keep frontend backend-independent per SPEC §3; allow resource
recreation/lazy allocation/emulation without breaking OpenGL object identity.
Consequence: Frontend stores `unique_ptr<BackendX>` opaque handles.

Decision: Capability checks centralized in `CapabilityTable`, not scattered
`if (glesVersion >= 31)` / `if (sdk >= 26)` branches.
Reason: SPEC §4/§5/§18 require version/platform decisions happen once.
Consequence: Frontend asks `getFeatureSupport(Feature)`; emulation selection
driven by capability system.

Decision: Test framework is self-contained (no FetchContent/googletest).
Reason: Headless Linux CI must build without network or GPU.
Consequence: Minimal macro-based framework; sufficient for unit/integration.

## Compatibility Progress

OpenGL 4.6:
  Core API: not implemented
  Compatibility profile: not implemented
  Shader stages: not implemented (interface only)
  DSA: not implemented (marked Emulated in mock capabilities only)
  Backend: Mock only (headless). GLES/Vulkan reserved.

## Recent Work

2026-08-26
- Repository + Git identity.
- CMake build + tests wiring.
- Backend abstraction interfaces.
- Capability system + CapabilityTable.
- Mock backend + mock capability profile.
- Linux platform capabilities.
- Self-contained test framework.
- Phase 2: Frontend `Context`, object model (Buffer/Texture/RBO/FBO/VAO),
  `GLError` handling, object lifecycle tests.
- Public GL dispatch surface (`gl_api`) over `Context`; gl_types layer.
- Vendored Khronos native headers (GL/GLES/EGL/Vulkan) + KHR/khrplatform.h.
- Architecture / feature-matrix / README docs.

## Next Steps

1. Begin GLES backend foundation: consume `GLES3`/`EGL` headers, create a
   headless surfaceless EGL context on Linux/Mesa, populate CapabilityTable
   from real version/extension detection.
2. Add state-tracking subsystem (buffers/textures/bindings) in `src/state`.
3. Implement shader translation pipeline behind `IShaderCompiler`.
4. Commit each coherent step; update this journal.
