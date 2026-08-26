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

## Frontend GL type layer

The public API surface (`include/glcompat/frontend/gl_api.hpp`) uses a small,
self-contained type/constant layer in `gl_types.hpp` (`GLenum`, `GLuint`,
`GL_ARRAY_BUFFER`, …) whose values match the desktop GL specification. This
keeps the frontend backend-agnostic and free of any single native header set.

## Native API headers (vendored)

`include/` also vendors Khronos native headers for backend implementations:
`GL/` (desktop reference), `GLES/`, `GLES2/`, `GLES3/` (GLES backend),
`EGL/` (headless surfaceless contexts on Linux/Android), and `vulkan/`
(future backend). `KHR/khrplatform.h` is provided so `glext.h` / `GLES3/gl3.h`
/ `EGL/egl.h` compile. These are consumed by backends, not by the frontend
type layer. The Vulkan subset is currently partial (some `vk_video/*`
sub-headers are absent); it becomes relevant only when the Vulkan backend
starts.

## GLES backend

`src/backend/gles` implements the real OpenGL ES backend behind the standard
interfaces (`IGraphicsBackend`, `IResourceFactory`, `IShaderCompiler`).

- **Dynamic loading.** EGL and GLES are resolved at runtime via `dlopen` +
  `dlsym` (`gles_loader.hpp` / `GLESLib`), not linked at build time. This lets
  YAGLT compile on hosts lacking the GLES dev libraries (and on Android, where
  the drivers exist at runtime). If `libEGL`/`libGLESv2` or required symbols
  are absent, `GLESBackend::initialize()` returns `false` honestly — no fake
  support.
- **Headless context.** A surfaceless EGL display
  (`EGL_PLATFORM_SURFACELESS_MESA`) with a pbuffer-compatible config is used so
  the backend works without a window system (Linux/Mesa, Android).
- **Capability detection.** After context creation the backend queries
  `GL_VERSION` / `GL_RENDERER` / `GL_EXTENSIONS` and populates `CapabilityTable`
  (see `gles_capabilities.cpp`): ES 3.x → most features Native; ES 3.1 →
  SSBO/compute/indirect/image-load native; geometry/tessellation Unsupported
  (no GLES equivalent); DSA Unsupported unless `GL_EXT_direct_state_access`.
- **Shader compiler.** `GLESShaderCompiler` compiles GLSL ES on the real driver
  and surfaces the driver log. It does **not** translate desktop GLSL → GLSL ES
  (that needs glslang, not yet available), so desktop inputs fail at the driver
  rather than being silently accepted.

## Current gaps

The OpenGL 4.6 frontend API is partially exposed (object management + error
path). Real GLES backend, shader translation pipeline, and most object/state
subsystems are not yet implemented. Phase 1 establishes the interfaces, the
mock backend, the capability system, and the headless test harness so
subsequent phases build on a verified foundation.
