# Architecture

YAGLT separates the OpenGL-facing frontend from backend implementations behind
stable, replaceable interfaces. The frontend owns OpenGL semantics, state
tracking, object identity/lifetime, validation, and error behavior. Backends
own native resource creation, command submission, and synchronization.

## Layers

```
Application
   │
   ▼
OpenGL 4.6 Compatibility API      (frontend; not yet implemented in Phase 1)
   │
   ▼
Frontend State + Object Management
   │
   ▼
Graphics Abstraction Layer        (IGraphicsBackend, IResourceFactory, ...)
   │
   ├── GLES Backend      (planned)
   ├── Vulkan Backend    (planned)
   ├── Mock Backend      (implemented; headless tests)
   └── Future Backends
```

## Core interfaces (Phase 1)

- `IGraphicsBackend` — entry point the frontend talks to. Exposes `api()`,
  `capabilities()`, `platform()`, `resourceFactory()`, `shaderCompiler()`,
  `initialize()`/`shutdown()`.
- `IResourceFactory` — creates opaque backend resource handles
  (`BackendBuffer`, `BackendTexture`, `BackendFramebuffer`, …). Frontend stores
  `unique_ptr<BackendX>` and never inspects internals, so native handles never
  leak into the generic API.
- `IShaderCompiler` — shader translation entry point. The real GLSL pipeline
  plugs in behind this later; the mock is a pass-through.
- `ICapabilities` / `CapabilityTable` — centralized feature resolution.
  Frontend asks `getFeatureSupport(Feature)` instead of checking GLES version,
  Android SDK, or extension strings directly.
- `IPlatformCapabilities` — platform differences resolved once at init
  (e.g. Android SDK 21 fallback strategy). `LinuxCapabilities` is the real
  headless impl; Android impl lives under `src/platform/android` and is never
  compiled into Linux builds.

## Capability-driven selection

Feature availability is decided during initialization and stored in a
`CapabilityTable`. Emulation implementations are chosen by the capability
system rather than scattered `if (glesVersion >= 31)` checks across the
renderer.

## Resource lifetime

OpenGL object identity is distinct from any backend handle. Backend resources
may be recreated, lazily allocated, or emulated; the frontend object model
owns the `unique_ptr` to the backend resource.

## Current gaps

The OpenGL 4.6 frontend API, real GLES backend, shader translation pipeline,
and most object/state subsystems are not yet implemented. Phase 1 establishes
the interfaces, the mock backend, the capability system, and the headless test
harness so subsequent phases build on a verified foundation.
