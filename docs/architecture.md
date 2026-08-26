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
  and surfaces the driver log. Under `YAGLT_SHADER_TRANSLATE`, the backend uses
  `TranslatingGLESShaderCompiler` instead, which translates desktop GLSL → GLSL
  ES via glslang (GLSL→SPIR-V) + SPIRV-Cross (SPIR-V→GLSL ES) before compiling
  on the driver. Already-GLSL-ES input is passed through without translation.

## Shader translation

`IShaderCompiler` is the entry point for the translation pipeline (SPEC §7).
The mock backend passes source through; the GLES backend compiles GLSL ES
directly on the driver. Real desktop GLSL → GLSL ES translation is implemented
by `ShaderTranslator` (`src/shader/shader_translator.cpp`), built when
`YAGLT_SHADER_TRANSLATE=ON`. It uses glslang (from `../glslang-main`, built with
`ENABLE_OPT=OFF` to skip SPIRV-Tools) to produce SPIR-V, then SPIRV-Cross (from
`../SPIRV-Cross-main`) to emit GLSL ES 3.10. `TranslatingGLESShaderCompiler`
selects it automatically under that flag. The library headers are consumed via
`CMake add_subdirectory` — no vendored copies are kept in the repo.

Desktop GLSL older than 4.20 omits `layout(binding=...)` on uniform/storage
blocks, but glslang requires one to emit SPIR-V. `ShaderTranslator` therefore
runs a small source transform (`assignDefaultBindings`) that injects a default
`binding=` for every uniform/storage block lacking one, and enables
`GL_ARB_shading_language_420pack` after the `#version` line. The frontend UBO
feature mapping is thus completed by the pipeline: capability classification
decides support (SPEC §8), and the translator makes desktop block syntax
portable to GLSL ES 3.10.

## State management

`src/state` (`GLStateTracker`, `GLStateSink`) is the centralized OpenGL pipeline
state (SPEC §10). It tracks capabilities, the active program, and blend/depth/
stencil/rasterization/pixel-store state. `set*` methods report whether a value
changed; `apply(sink)` pushes only the categories whose state differs from the
last applied snapshot, so backends avoid redundant native calls. `Context` owns
a tracker and routes `glEnable`/`glDisable`/`glBlendFunc`/`glUseProgram`/
`glDepthFunc`/`glDepthMask`/`glCullFace`/`glFrontFace` through it. A backend that
implements `GLStateSink` can flush the tracker's dirty state at draw/flush time.

The state sink is decoupled from the frontend GL constant layer: `GLStateSink`
(methods in `src/state/gl_state_sink.hpp`) takes plain integer types so a backend
can implement it even while including native GL headers. `IGraphicsBackend::
stateSink()` returns the sink (or `nullptr`); `Context::flushState()` calls
`GLStateTracker::apply(sink)`, pushing only the categories whose state changed.
`MockBackend` records every push (observable in tests); `GLESBackend` issues the
corresponding native `gl*` calls through its runtime-loaded `GLESLib`. The public
`glFlushState()` in `gl_api` is the explicit flush entry point.

## Indexed buffer bindings (UBO / SSBO / transform feedback)

`GLStateSink` also carries `bindBufferBase`/`bindBufferRange` so backends bind
buffers to indexed targets. The frontend `Context::bindBufferBase` /
`bindBufferRange` map the target (`GL_UNIFORM_BUFFER`, `GL_SHADER_STORAGE_BUFFER`,
`GL_TRANSFORM_FEEDBACK_BUFFER`) to the corresponding `Feature` and consult the
capability table first: an unsupported target yields `GL_INVALID_OPERATION`
honestly instead of a native call the driver would reject. The GLES backend
issues the native `glBindBufferBase` / `glBindBufferRange`; the mock records them.

## Current gaps

The OpenGL 4.6 frontend API is partially exposed (object management + error
path + state routing). The GLES backend, shader translation pipeline, and
centralized state tracking are implemented; backends do not yet flush the
tracker to the driver, and most of the OpenGL 4.6 draw/object API surface
remains to be built. Phase 1 establishes the interfaces, the mock backend, the
capability system, and the headless test harness so subsequent phases build on
a verified foundation.
