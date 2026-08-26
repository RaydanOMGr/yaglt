# Next-Agent Prompt (YAGLT)

You are continuing work on **YAGLT — Yet Another GL Translator**, a backend-agnostic
OpenGL 4.6 compatibility/translation layer. The frontend is backend-independent;
backends (Mock, GLES, future Vulkan) sit behind stable interfaces. The primary
target is translating desktop OpenGL 4.6 → OpenGL ES (Android / host Mesa).

Repo: `/home/andre/projects/yaglt`. Libraries are built in sibling dirs:
`../glslang-main`, `../SPIRV-Cross-main`, `../SPIRV-Tools-main`, `../shaderc-main`,
`../mesa-26.2.1` (Mesa already built + installed to `../mesa-26.2.1/install`).

## What is already done (do not rebuild)

- **Foundation / interfaces**: `IGraphicsBackend`, `IResourceFactory`,
  `IShaderCompiler`, `ICapabilities`/`CapabilityTable`, `IPlatformCapabilities`.
  Mock backend (headless tests) + Linux platform capabilities.
- **Frontend object model** (`src/frontend`, `include/glcompat/frontend`):
  `Context` with name gen, bind tracking, delete-with-bound-reset, `GLError`
  handling. Object classes Buffer/Texture/RBO/FBO/VAO owning `unique_ptr<BackendX>`.
- **Public GL dispatch** (`gl_api`): glGen*/glBind*/glDelete*/glBufferData/glGetError
  + the new state calls (see below).
- **GLES backend** (`src/backend/gles`): runtime-loaded via `dlopen`
  (`GLESLib`), surfaceless EGL context, capability detection, real resource
  factory, shader compiler. Honest `initialize()` (false if no driver).
- **Shader translation pipeline** (`src/shader/shader_translator.cpp`,
  `src/backend/gles/gles_translating_compiler.*`): desktop GLSL → glslang (GLSL→
  SPIR-V) → SPIRV-Cross (SPIR-V→GLSL ES 3.10). Built under `YAGLT_SHADER_TRANSLATE=ON`
  via `add_subdirectory` of `../glslang-main` (`ENABLE_OPT=OFF`) and
  `../SPIRV-Cross-main`. SPIRV-Tools/shaderc NOT required.
- **Mesa 26** built locally (softpipe, surfaceless) → host `libEGL.so.1` +
  `libGLESv2.so.2`. GLES backend now initializes on this headless box
  (`renderer=softpipe, ES 3.1`).
- **State tracking** (`src/state`, `include/glcompat/state/gl_state.hpp`):
  `GLStateTracker` + `GLStateSink`. Tracks capabilities, active program, blend,
  depth, stencil, raster, pixel-store. `set*` returns changed-flag;
  `apply(sink)` pushes only dirty categories. `Context` owns one; `glEnable`/
  `glDisable`/`glBlendFunc`/`glBlendEquation`/`glUseProgram`/`glDepthFunc`/
  `glDepthMask`/`glCullFace`/`glFrontFace` route through it.

## Build & test

Default (self-contained, no GPU):
```
cmake -S . -B build -DYAGLT_BUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
Translation + end-to-end GLES (needs Mesa libs on path):
```
cmake -S . -B build_tx -DYAGLT_BUILD_TESTS=ON -DYAGLT_SHADER_TRANSLATE=ON
cmake --build build_tx -j$(nproc)
LD_LIBRARY_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu \
LIBGL_DRIVERS_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu/dri \
GALLIUM_DRIVER=softpipe \
ctest --test-dir build_tx --output-on-failure
```
Both currently green (default + 29/29 with Mesa). Use a sanitizer build
(`-DYAGLT_ENABLE_SANITIZERS=ON`) before considering work done.

## Recommended next tasks (in order)

1. **Wire `GLStateTracker::apply()` into backends.** Make `GLESBackend` (and
   `MockBackend`) implement `GLStateSink`, and flush the tracker's dirty state at
   draw / state-flush time so redundant `glEnable`/`glBlendFunc`/etc. calls are
   actually skipped. Today the tracker records state but nothing pushes it to a
   driver. Add a test that a `MockBackend`-backed `Context` issues a native
   enable/disable only when the capability changed.
2. **Expand desktop→ES feature mapping** behind `IShaderCompiler` / capability
   system: UBO/SSBO, geometry/tessellation (emulate or report Unsupported
   honestly per `CapabilityTable`), clip-distance, etc. Keep using
   glslang+SPIRV-Cross; extend `ShaderTranslator` options as needed.
3. **Continue SPEC phases** (§5 Android platform capabilities, §6 compatibility/
   emulation scaffolding, modern features). Follow the architecture: capability
   decisions centralized, backend handles never leak into the generic API.

## Hard rules (from prior agents)

- Frontend stays backend-agnostic. Never expose native handles in the public API.
- Never fake support — if a feature is unsupported, report it (error / Unsupported).
- `build/`, `build_tx/`, `build_tr/` are build dirs — do NOT commit them.
  `.gitignore` covers `build/`; `build_tx`/`build_tr` are untracked, leave them.
- Commit only when explicitly asked. Keep changes incremental and tested.
- Update `docs/agent-progress.md` (and `architecture.md` / `feature-matrix.md`
  when behavior changes) after each coherent step. The journal is the source of
  truth for status.
- Read `SPEC.md` (3422 lines) for authoritative requirements; it is the contract.

## Quick file map

- Interfaces: `include/glcompat/core/*`, `include/glcompat/backend/gles/*`
- Frontend: `include/glcompat/frontend/*`, `src/frontend/*`
- State: `include/glcompat/state/gl_state.hpp`, `src/state/gl_state.cpp`
- GLES backend: `src/backend/gles/*` (loader, capabilities, backend, compilers)
- Shader translate: `src/shader/shader_translator.*`
- Tests: `tests/unit/*`, `tests/backend/*` (self-contained framework in
  `tests/framework/test_framework.hpp`)
- Docs: `SPEC.md`, `docs/architecture.md`, `docs/feature-matrix.md`,
  `docs/agent-progress.md`

Begin by reading `docs/agent-progress.md` (current status + Next Steps) and
`SPEC.md` §10 (state) + §7 (shader system), then implement task 1 above.
