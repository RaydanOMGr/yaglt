# Development Journal

Persistent, version-controlled progress record. Updated after meaningful
milestones, architectural decisions, and before ending a session.

## Current Status

Current milestone: Phase 3 — Core rendering state (viewport/scissor/depth-range/clear) + draw
Overall status: Early implementation (foundation + object model + GL dispatch + GLES backend + shader translate + object/state API + clear)
Last updated: 2026-08-26
Known major blockers:
- Geometry/tessellation/compute still honest-Unsupported (no emulation yet).

## Toolchain & Environment

- Android NDK root (for building/testing the Android platform path):
  `/mnt/c/Users/Andreas/AppData/Local/Android/Sdk/ndk/29.0.14206865/`
  - This is a Windows NDK install reached via WSL. Its executables are
    `.exe` files but are runnable from this WSL environment directly.
  - Use it when compiling the `src/platform/android` layer or running
    Android-targeted builds; the host `android_stub` headers (see
    architecture.md) let that code compile here without the NDK libs.
- Host (Linux/WSL) build stays self-contained with the mock backend; no NDK
  required for the headless test suite.

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

- [x] GLES backend foundation (runtime-loaded)
  - `GLESLib` dynamic loader (dlopen EGL/GLES, dlsym ~45 entry points).
  - `GLESBackend`: surfaceless EGL context, capability detection from
    version/extensions, real resource factory, GLES shader compiler.
  - `tests/backend/gles_backend_test.cpp`: passes both with and without a
    driver (initialize() honest). 21/21 tests pass.
  - Builds linking only libdl; compiles where libGLESv2 dev libs are absent.
- [x] Vendored glslang headers
  - Copied full `glslang/` source-tree layout into `include/glslang/` so
    `glslang/Public/ShaderLang.h` resolves its relative includes.
  - Verified the public headers compile standalone. Full library at
    `../glslang-main` (build with ENABLE_OPT=OFF to skip SPIRV-Tools).
  - End-to-end desktop→ES translation still needs the built lib + SPIRV-Cross
    (not yet vendored) — recorded as blocked.

## In Progress

- [x] Wire `GLStateTracker::apply()` into backends: `GLESBackend` and
      `MockBackend` implement `GLStateSink` and push tracked state on
      `Context::flushState()` / `glFlushState()`, so redundant native calls are
      skipped. Tests verify only-changed-caps are pushed.

## Completed (this session)

- [x] Shader translation pipeline behind `IShaderCompiler` (Phase 4 per SPEC).
  - glslang (desktop GLSL → SPIR-V) + SPIRV-Cross (SPIR-V → GLSL ES 3.10)
    wired in via `YAGLT_SHADER_TRANSLATE=ON` (`src/CMakeLists.txt`).
  - `ShaderTranslator` (`src/shader/shader_translator.cpp`) uses the real
    glslang + SPIRV-Cross libraries (built from `../glslang-main` and
    `../SPIRV-Cross-main` via `add_subdirectory`, `ENABLE_OPT=OFF`).
  - `TranslatingGLESShaderCompiler` (`src/backend/gles/gles_translating_compiler.*`)
    passes already-ES sources straight to the driver and translates desktop
    sources first. Selected automatically when `YAGLT_SHADER_TRANSLATE=ON`.
  - `tests/backend/shader_translate_test.cpp` translates a `#version 330 core`
    vertex shader to GLSL ES and verifies `gl_Position` survives. Passes.
- [x] Removed conflicting partial vendored headers (`include/glslang`,
      `include/shaderc`, `include/spirv-tools`, top-level `include/spirv*.hpp`)
      that shadowed the real library headers and broke the translator build.
- [x] Built Mesa 26 (softpipe, surfaceless EGL, GLESv2) into a local prefix
      (`../mesa-26.2.1/build-mesa`, installed to `../mesa-26.2.1/install`).
      Provides host `libEGL.so.1` + `libGLESv2.so.2` so the GLES backend now
      initializes for real on this headless Linux box (renderer: softpipe,
      ES 3.1) — no Android device or GPU needed.
- [x] End-to-end GLES shader test (`tests/backend/gles_e2e_shader_test.cpp`):
      compiles a `#version 330 core` vertex shader through the GLES backend
      (desktop → glslang → SPIRV-Cross → GLSL ES → Mesa driver). Passes with
      the Mesa libs on `LD_LIBRARY_PATH`. Skips cleanly where no driver exists.

- [x] Centralized state tracking subsystem (`src/state`, `include/glcompat/state`).
      `GLStateTracker` tracks capabilities, active program, blend, depth,
      stencil, rasterization and pixel-store state; `set*` returns whether the
      value changed and `apply(sink)` pushes only categories that differ from
      the last applied state (SPEC §10: avoid redundant backend calls). `Context`
      owns one and routes `glEnable`/`glDisable`/`glBlendFunc`/`glUseProgram`/
      `glDepthFunc`/`glDepthMask`/`glCullFace`/`glFrontFace` through it.
       `tests/unit/state_test.cpp` verifies change detection and no-op applies.

- [x] Frontend shader/program/vertex-attrib API (SPEC §8, §2.1, next-agent task 1).
  - `ShaderObject` / `ProgramObject` added to `include/glcompat/frontend/objects.hpp`;
    each owns an opaque `BackendShader` / `BackendProgram` whose virtual
    `compile` / `attach` / `link` / `getAttribLocation` / `nativeId` do the real
    work. `Context` exposes `createShader` / `shaderSource` / `compileShader` /
    `createProgram` / `attachShader` / `linkProgram` / `getAttribLocation` /
    `deleteShader` / `deleteProgram`, all capability-gated (ShaderObjects /
    ProgramObjects). `gl_api` exposes the matching `gl*` entry points.
  - Compile runs the desktop source through `IShaderCompiler` (glslang +
    SPIRV-Cross under `YAGLT_SHADER_TRANSLATE`) before the backend compiles, so
    the backend never sees raw desktop GLSL.
  - On link / VAO creation the frontend registers the frontend name → native id
    mapping with the backend via `IGraphicsBackend::bindNativeObject`; the GLES
    backend then binds the real driver program/VAO at draw/flush time (SPEC §3/§11).
  - Vertex attribute state recorded on the bound `VertexArrayObject` and pushed
    via new `GLStateSink` methods (`bindVertexArray` / `enableVertexAttribArray` /
    `disableVertexAttribArray` / `vertexAttribPointer`); pushed only when dirty.
  - Shader translator now also injects default `layout(location=...)` for
    user `in`/`out` interface variables (glslang/SPIR-V requires located user I/O),
    so desktop fragment outputs translate without manual edits.
  - New `tests/unit/shader_program_test.cpp` (mock path) and
    `tests/backend/gles_e2e_program_test.cpp` (real Mesa program link + draw).
    All suites green: default 41/41, sanitizer 41/41, translate (Mesa) all pass.

- [x] Sampler objects (SPEC §8.2, Next Steps item 1).
   - `BackendSampler` resource + `IResourceFactory::createSampler`; frontend
      `SamplerObject` (params map + opaque backend). `Context` gained `genSampler`/
      `bindSampler`/`deleteSampler`/`samplerParameteri`/`getSamplerParameteriv`/
      `isSampler` (capability-gated by `SamplerObjects`; Native on GLES 3.0+).
      `bindSampler(unit, sampler)` records the per-unit binding in `GLStateTracker`
      and pushes via new `GLStateSink::bindSampler` only when changed (SPEC §10).
      `GL_SAMPLER_BINDING` query added to the tracker; out-of-range unit →
      `GL_INVALID_VALUE`, ungenerated name → `GL_INVALID_OPERATION`. `samplerParameteri`
      accepts only scalar sampler pnames (table 23.23); non-scalar/unknown →
      `GL_INVALID_ENUM`. GLES backend resolves `glGenSamplers`/`glDeleteSamplers`/
      `glBindSampler`/`glSamplerParameteri`/`glIsSampler` (optional); `GLESBackendSampler`
      drives the native object. `Feature::SamplerObjects` added to the capability enum
      and marked Native in the mock + GLES (ES3) profiles. New `tests/unit/sampler_test.cpp`.
   - Validation: default + sanitizer + translate (Mesa) suites all green.

- [x] DSA texture binding (SPEC §2.1, Next Steps item 1).
   - Direct State Access texture binds landed as a frontend emulation (no new
     backend sink method needed — DSA bindings live in the same per-unit binding
     table the `GLStateSink` flush already pushes, switching the driver active
     unit only when it differs). `GLStateTracker` gained `setTextureUnitBinding`
     / `setTextureBindings` / `boundTextureForUnitTarget`; `Context` gained
     `bindTextureUnit` / `bindTextures` / `boundTextureForUnitTarget`, both
     capability-gated by `DirectStateAccess` (Emulated in the mock). Validation:
     out-of-range unit → `GL_INVALID_VALUE`; ungenerated name →
     `GL_INVALID_OPERATION`; invalid target → `GL_INVALID_ENUM`;
     `glBindTextureUnit(unit, 0)` clears the whole unit. `gl_types.hpp` gained the
     remaining texture-target constants (1D/2D/3D/CUBE/RECT/ARRAY/MULTISAMPLE).
     Public `gl_api` exposes `glBindTextureUnit` / `glBindTextures`. New
     `tests/unit/dsa_texture_test.cpp`. Validation: default, sanitizer, and
     translate (Mesa) builds all green.

- [x] FBO completeness reasons (SPEC §9.4, Next Steps item 1).
   - `Context::checkFramebufferStatus` now reports honest per-attachment
     completeness instead of only the structural missing-attachment check. An
     attachment referencing a generated-but-not-yet-specified object (texture
     without `texImage2D` storage, or renderbuffer without `renderbufferStorage`)
     returns `GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT`; an FBO with no attachments
     still returns `GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT`. Driver-level
     format support is still delegated to the backend `checkStatus` when a backend
     resource exists. `isStructurallyComplete` kept for the non-empty + valid-name
     invariant. New `texture_fbo_test.cpp` cases cover the no-storage attachment
     path and the transition to COMPLETE after storage is allocated. Validation:
     default, sanitizer, and translate (Mesa) builds all green.

- [x] Texture parameter queries (SPEC §8.1).
   - `glGetTexParameteriv(target, pname, params)` reads the `params` map of the
     currently bound texture for `target`; `glGetTextureParameteriv(texture,
     pname, params)` is the DSA variant that reads an explicit texture object
     (capability-gated by `DirectStateAccess`, Emulated in the mock). Null
     `params` → `GL_INVALID_VALUE`; missing/unbound texture → `GL_INVALID_OPERATION`;
     unknown pname returns 0 (the GL default). Pushes nothing to the backend — the
     frontend owns the values (SPEC §10). `Context` gained `getTexParameteriv` /
     `getTextureParameteriv`; `gl_api` exposes both entry points. New
     `tests/unit/texparam_query_test.cpp`. Validation: default + sanitizer green.
   - `BackendSampler` resource + `IResourceFactory::createSampler`; frontend
     `SamplerObject` (params map + opaque backend). `Context` gained `genSampler`/
     `bindSampler`/`deleteSampler`/`samplerParameteri`/`getSamplerParameteriv`/
     `isSampler` (capability-gated by `SamplerObjects`; Native on GLES 3.0+).
     `bindSampler(unit, sampler)` records the per-unit binding in `GLStateTracker`
     and pushes via new `GLStateSink::bindSampler` only when changed (SPEC §10).
     `GL_SAMPLER_BINDING` query added to the tracker; out-of-range unit →
     `GL_INVALID_VALUE`, ungenerated name → `GL_INVALID_OPERATION`. `samplerParameteri`
     accepts only scalar sampler pnames (table 23.23); non-scalar/unknown →
     `GL_INVALID_ENUM`. GLES backend resolves `glGenSamplers`/`glDeleteSamplers`/
     `glBindSampler`/`glSamplerParameteri`/`glIsSampler` (optional); `GLESBackendSampler`
     drives the native object. `Feature::SamplerObjects` added to the capability enum
     and marked Native in the mock + GLES (ES3) profiles. New `tests/unit/sampler_test.cpp`.
   - Validation: default + sanitizer + translate (Mesa) suites all green.

## Known Issues

- Host previously had no `libGLESv2`, so the GLES backend returned false.
  **Resolved**: Mesa 26 (softpipe/surfaceless) is now built locally and the
  backend initializes on host (ES 3.1, renderer softpipe). Run tests with
  `LD_LIBRARY_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu
  LIBGL_DRIVERS_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu/dri
  GALLIUM_DRIVER=softpipe`.
- SPIRV-Tools / shaderc not built: not needed for the current pipeline. Add
  later only if shader optimization or shaderc's higher-level API is wanted.

## TODO

- [x] P0: Implement OpenGL 4.6 frontend API entry points (gen/bind/delete, buffer, texture, FBO, VAO, draw, shader/program).
- [x] P0: Implement core object model (buffers, textures, VAO, FBO, RBO).
- [x] P1: Implement GLES backend foundation + headless EGL validation.
- [x] P1: Implement shader translation pipeline behind `IShaderCompiler`.
- [x] P0: Implement uniform setting (`glUniform*`) on the active program.
- [x] P1: Implement renderbuffer storage + full draw (texture+program) e2e on GLES/Mesa.
- [x] P1: Android platform capabilities + SDK 21 fallback abstraction.
- [x] P2: Capability-driven emulation selection scaffolding (partial — `CapabilityTable` resolves Native/Emulated/Unsupported once at init and `CapabilityTable::report()` logs the chosen path + activated fallbacks; deeper per-feature emulation wiring lands with the emulation implementations, deferred per the Emulation roadmap).
- [x] P3: Structured logging categories (CORE/STATE/RESOURCE/...).

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
  Core API: partial — see `docs/coverage-core.md` for the quantitative assessment.
     Measurement (2026-08-26): of the 490 command prototypes the spec declares,
      111 (22.7%) have a frontend entry point; restricting to the core profile
      (435 prototypes after removing 55 compat-only commands from Appendix E.2.2)
       gives 111/435 ≈ 25.5% core prototype coverage. True core entry-point coverage
    is lower (the spec text undercounts type/vector variants, and geometry/
    tessellation/compute are honestly Unsupported). Implemented slice: object
    lifecycle, vertex+fragment shader pipeline (desktop→ES), uniforms, per-
    fragment/blend/depth/stencil/viewport/scissor state, transform feedback
    scaffolding, samplers, DSA texture bind, basic draws — all with dispatch +
    validation + tests.
  Core API detail: Object gen/bind/delete for buffers, textures, RBO, FBO, VAO;
    buffer data upload; texture image storage + parameters; FBO attachments +
    completeness; pixel store; draw calls; shader/program/attrib API all wired.
    (SPEC §2.1 surface implemented against backend abstraction + mock; GLES path
    real against Mesa softpipe.)
  Compatibility profile: not implemented (plan documented in
    docs/feature-matrix.md "Compatibility Profile" — opt-in via EGL only,
    gated behind a majority of core being done, emulated by record-then-replay
    into a generated GLSL shader)
  Shader stages: desktop GLSL → GLSL ES translation implemented (glslang +
    SPIRV-Cross), exercised by `shader_translate_test` + e2e program/shader tests.
    Translator injects default precision (fragment) + uniform locations so desktop
    shaders with uniforms compile on ES.
  Uniforms: `glGetUniformLocation` + `glUniform*` (f/i, vectors, 1fv/1iv, mat4)
    implemented on the active program (SPEC §8); real path via GLES backend.
   DSA: texture binding implemented (`glBindTextureUnit` / `glBindTextures`,
     SPEC §2.1; Emulated via the frontend per-unit binding table). Remaining DSA
     entry points (object-specific `gl*Texture*` / `gl*Named*` and vertex-array
     DSA) not yet implemented.
  Backend: Mock (headless) + GLES (runtime-loaded). Vulkan reserved.

- [x] Draw-call frontend entry points (SPEC §2.1, task 2 from next-agent-prompt).
  - Renderbuffer storage + e2e FBO completeness + full draw (this session).
  - `BackendRenderbuffer::renderbufferStorage` virtual added; `RenderbufferObject`
    records storage (internalFormat/width/height). `Context::renderbufferStorage`
    (capability-gated, target==GL_RENDERBUFFER, no-bound→INVALID_OPERATION,
    negative size→INVALID_VALUE) + `glRenderbufferStorage` in the public API.
  - `GLESBackendRenderbuffer` binds its handle then calls the driver
    `glRenderbufferStorage` (added to the `GLESLib` runtime loader); `MockRenderbuffer`
    records the call.
  - New e2e test `gles_e2e_framebuffer_complete_and_full_draw` builds an FBO with a
    depth renderbuffer + color texture attachment, verifies COMPLETE against the real
    Mesa driver, then draws with the texture bound to a sampler and the program in use.
  - Validation: default, translate (Mesa) and sanitizer suites all green.
  - `IGraphicsBackend` gained `drawArrays`/`drawElements` + instanced variants
    (pure virtual; implemented by `MockBackend` and `GLESBackend`).
  - `GLESLib` resolves `glDrawArrays`/`glDrawElements` (required) and
    `glDrawArraysInstanced`/`glDrawElementsInstanced` (optional, ES 3.0+).
  - `Context::draw*` flushes tracked pipeline state to the backend first, then
    issues the native draw (SPEC §10: redundant state skipped). Drawing with no
    active program → `GL_INVALID_OPERATION`; instanced draws consult the
    `InstancedRendering` capability and report `Unsupported` honestly.
  - Public `gl_api` exposes `glDrawArrays`/`glDrawElements`/`glDrawArraysInstanced`/
    `glDrawElementsInstanced`. New `tests/unit/draw_test.cpp` covers flush-before-draw,
    recording, capability gating, and the no-program error path.
   - Validation: default 37/37, sanitizer 37/37, translate (Mesa) all green.

- [x] Texture / FBO / pixel-store / buffer-data API (SPEC §2.1, Next Steps item 1).
  - Frontend `Context` gained `texImage2D`, `texParameteri`, `framebufferTexture2D`,
    `framebufferRenderbuffer`, `checkFramebufferStatus`, `pixelStorei`, and a
    data-carrying `bufferData`. `TextureObject`/`FramebufferObject` now record
    per-level image storage, texture params, and FBO attachments (with a structural
    completeness check). `BackendBuffer`/`BackendTexture`/`BackendFramebuffer`/
    `BackendRenderbuffer` gained real virtual ops; default no-op so backends opt in.
  - GLES backend implements all of them via the runtime loader (`glTexImage2D`,
    `glTexParameteri`, `glFramebufferTexture2D`, `glFramebufferRenderbuffer`,
    `glCheckFramebufferStatus`); FBO attach resolves the frontend texture name to
    the native id before calling the driver.
  - Mock backend records every call (observable in tests). `pixelStorei` pushes to
    the backend only when the value changed (SPEC §10: no redundant native calls).
  - New `tests/unit/texture_fbo_test.cpp` (mock path) covers texImage params,
    negative-size `GL_INVALID_VALUE`, no-texture-bound `GL_INVALID_OPERATION`,
    buffer upload push, FBO texture + renderbuffer attachment + completeness, and
    the `gl*` surface. All suites green: default 52/52, sanitizer 52/52,
    translate (Mesa) all pass.

## Recent Work

2026-08-26 (glGet* state queries, this session)
- SPEC §22: implemented `glGetBooleanv`/`glGetIntegerv`/`glGetFloatv`/
  `glGetDoublev`/`glIsEnabled` against the centralized `GLStateTracker`
  (frontend owns these values, so glGet never round-trips to the backend driver,
  SPEC §10). Tracked caps (BLEND/CULL_FACE/DEPTH_TEST/STENCIL_TEST/SCISSOR_TEST),
  VIEWPORT, SCISSOR_BOX, blend src/dst factors + equations + color, DEPTH_FUNC/
  WRITEMASK/RANGE, COLOR/DEPTH_CLEAR_VALUE, CULL_FACE_MODE, FRONT_FACE,
  CURRENT_PROGRAM. Unknown pname -> GL_INVALID_ENUM (honest); null buffer ->
  GL_INVALID_VALUE; untracked cap in glIsEnabled -> GL_INVALID_ENUM. New GL
  query constants added to `gl_types.hpp`; `GLStateTracker` gained
  `getInteger/getBoolean/getFloat/getDouble/isCapabilityEnabled`. New
  `tests/unit/getstate_test.cpp`. Validation: default + sanitizer 113/113 green;
  translate (Mesa) all tests pass (pre-existing Mesa atexit segfault unrelated
  to this change, confirmed by stash baseline).

2026-08-26 (structured logging subsystem, this session)
- SPEC §20: implemented a structured logging subsystem. New
  `include/glcompat/core/log.hpp` + `src/core/log.cpp` provide categories
  (CORE/STATE/RESOURCE/SHADER/BACKEND/GLES/VULKAN/PLATFORM/EMULATION), levels
  (Debug/Info/Warn/Error), a streaming `log(cat, level) << ...` proxy, and a
  process-wide `Logger` that is configurable (setStream / setLevel /
  enableCategory) and reads `YAGLT_LOG_LEVEL` / `YAGLT_LOG_CATS` env vars.
  Defaults to stderr at Info so the release configuration does not spam.
- `CapabilityTable::report()` logs each feature's support classification and an
  activated-fallback summary at Debug (SPEC §20: selected feature implementations
  + activated fallbacks). `GLESBackend::initialize` logs backend selection +
  detected extensions (Debug) and a summary line (Info); `MockBackend::
  initialize` logs selection; `Context::compileShader` logs translation/compile
  failures at Error. Marked P3 done in TODO.
- New `tests/unit/log_test.cpp` covers level/category filtering, name helpers,
  and the capability report classification. Validation: default + sanitizer
  109/109 green.

2026-08-26 (renderbuffer storage + e2e FBO/draw, this session)
- Renderbuffer storage subsystem (SPEC §2.1): `renderbufferStorage` virtual on
  `BackendRenderbuffer`, `RenderbufferObject` storage state, `Context::renderbufferStorage`
  + `glRenderbufferStorage` (capability-gated, validation per spec), `GLESBackendRenderbuffer`
  and `MockRenderbuffer` implementations, `GLESLib::glRenderbufferStorage` loader entry.
- New `tests/unit/texture_fbo_test.cpp` cases: rbo storage records state, no-bound and
  negative-size errors, depth-rbo FBO completeness via mock.
- New `tests/backend/gles_e2e_framebuffer_complete_and_full_draw`: real Mesa driver builds
  an FBO (depth RBO + color texture), asserts COMPLETE, draws with texture+program.
- Validation: default green, translate (Mesa) green, sanitizer green.

2026-08-26 (uniforms, this session)
- Uniform setting subsystem (SPEC §8, next-agent Next Steps item 1). `BackendProgram`
  gained `getUniformLocation` + `uniform1f..4f`/`uniform1i..4i`/`uniform1fv`/`uniform1iv`/
  `uniformMatrix4fv` virtuals (default no-op). `Context` gained `getUniformLocation`
  (validates linked program) and `uniform*` setters that operate on the active program
  (no active program → `GL_INVALID_OPERATION`; -1 location → silent no-op). Public
  `gl_api` exposes `glGetUniformLocation` + all `glUniform*` variants.
- `GLESBackendProgram` implements the real path: binds its driver program only when it
  differs from `GLESLib::currentProgram` (redundant-bind avoidance, SPEC §10); the GLES
  loader resolves the 14 uniform entry points (optional, so load() still succeeds without
  them). `GLESBackend::useProgram` keeps `currentProgram` in sync.
- `MockProgram` records every uniform call (location/args/count) so tests assert behavior.
- New `tests/unit/uniform_test.cpp` (mock path: location stability, no-active-program and
  -1-location handling, all variants). New `gles_e2e_uniform_set` in
  `tests/backend/gles_e2e_program_test.cpp` exercises the real Mesa driver path.
- Shader translator (SPEC §7) hardened for the desktop→ES path: emit default
  `highp` precision for fragment float/int (ES requires it), and inject default
  `layout(location=...)` for bare `uniform` declarations (glslang/SPIR-V requires
  located non-block uniforms). Both fixes exercised by the e2e uniform test.
- Validation: default 58/58, sanitizer 58/58, translate (Mesa) 64/64 green.


2026-08-26 (this session)
- Texture / FBO / pixel-store / buffer-data frontend API (SPEC §2.1). Added
  `texImage2D`, `texParameteri`, `framebufferTexture2D`, `framebufferRenderbuffer`,
  `checkFramebufferStatus`, `pixelStorei`, and data-carrying `bufferData` to
  `Context` + `gl_api`. Backend resource virtuals implemented in mock (record) and
  GLES (real, via loader). New `tests/unit/texture_fbo_test.cpp` green.
- All suites pass: default 52/52, sanitizer 52/52, translate (Mesa) all pass.

2026-08-26 (prior agent)
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
- GLES backend foundation (runtime-loaded `GLESLib`, surfaceless EGL context).
- Architecture / feature-matrix / README docs.

2026-08-26 (continued)
- Wired shader translation pipeline: glslang + SPIRV-Cross via
  `YAGLT_SHADER_TRANSLATE=ON`. Fixed glslang C-API usage in
  `src/shader/shader_translator.cpp` (global `EShLanguage`/`EShMessages`,
  `TProgram` + `addShader`, `GlslangToSpv(*interm,...)`, `ResourceLimits.h`
  + `SPIRV/GlslangToSpv.h` includes). Fixed SPIRV-Cross options via
  `get_common_options`/`set_common_options`.
- Removed conflicting partial vendored headers (`include/glslang`,
  `include/shaderc`, `include/spirv-tools`, top-level `include/spirv*.hpp`).
- `build_tx` (translate) + `build` (default) both compile and pass tests.

2026-08-26 (state flush)
- Wired `GLStateTracker::apply()` into backends (SPEC §10, task 1 from
  next-agent-prompt). `IGraphicsBackend::stateSink()` returns the backend's
  `GLStateSink`; `Context::flushState()` / `glFlushState()` pushes only changed
  state. `MockBackend` records every push (observable in tests); `GLESBackend`
  issues native `gl*` calls via its runtime-loaded `GLESLib`.
- Split `GLStateSink` into its own header (`include/glcompat/state/
  gl_state_sink.hpp`) using plain integer types so backends can implement it
  alongside native GL headers without colliding with the frontend GL constant
  layer (`gl_types.hpp`). `GLESLib` gained the state-entry function pointers.
- Added `glEnable`/`glDisable`/.../`glFlushState` declarations to `gl_api.hpp`
  (previously defined but undeclared).
- New test `tests/unit/state_flush_test.cpp` verifies a `MockBackend`-backed
  `Context` issues a native enable/disable (and other state) only when the
  value actually changed.
- Validation: default 30/30, translate (Mesa) 32/32, and an ASan/UBSan build
  all green.

2026-08-26 (UBO/SSBO binding)
- Implemented capability-driven UBO/SSBO/transform-feedback indexed bindings
  (SPEC §8). Added `glBindBufferBase`/`glBindBufferRange` to the public API and
  `Context::bindBufferBase`/`bindBufferRange`, which consult the capability
  table: an unsupported target yields `GL_INVALID_OPERATION` honestly. Mapped
  `GL_UNIFORM_BUFFER`→UniformBufferObjects, `GL_SHADER_STORAGE_BUFFER`→
  ShaderStorageBufferObjects, `GL_TRANSFORM_FEEDBACK_BUFFER`→TransformFeedback.
- Extended `GLStateSink` with `bindBufferBase`/`bindBufferRange` (plain int
  types). `MockBackend` records them; `GLESBackend` issues native
  `glBindBufferBase`/`glBindBufferRange` (added to `GLESLib` loader).
- Added `GL_UNIFORM_BUFFER` / `GL_SHADER_STORAGE_BUFFER` /
  `GL_TRANSFORM_FEEDBACK_BUFFER` to the frontend GL constant layer.
- Exposed `populateGLESCapabilities` via a dedicated header so it is testable
  without a driver; `capabilities_test` now verifies UBO=Native on ES3.0 but
  SSBO=Unsupported on ES3.0 / Native on ES3.1.
- New `tests/unit/ubo_ssbo_test.cpp` covers recording, name validation, and the
  capability guard.
- ASan caught an out-of-bounds read: indexing the capability table with the
  `FeatureCount` sentinel for unknown targets. Fixed by short-circuiting
  `Feature::FeatureCount` before the table lookup.

2026-08-26 (desktop→ES uniform-block translation)
- Extended the shader pipeline (SPEC §7/§8) so desktop uniform/storage blocks
  translate to GLSL ES. glslang requires an explicit `layout(binding=...)` to
  emit SPIR-V, but desktop GLSL < 4.20 omits it. `ShaderTranslator` now runs a
  source transform `assignDefaultBindings` that injects a default `binding=`
  for every uniform/storage block lacking one and enables
  `GL_ARB_shading_language_420pack` after the `#version` line.
- Verified end-to-end: `shader_translate_test` translates a desktop UBO vertex
  shader to GLSL ES (block member preserved), and `gles_e2e_shader_test`
  compiles the translated UBO shader on the real Mesa/GLES driver (ES 3.1).

## Recent Work

2026-08-26 (Android platform capabilities + SDK 21 fallback, this session)
- SPEC §5/§18: centralized Android capability handling. New
  `AndroidCapabilities` (`src/platform/android/android_capabilities.cpp`,
  `include/glcompat/platform/android/android_capabilities.hpp`) implements
  `IPlatformCapabilities`, resolving SDK-level decisions once at init.
- New `ISharedMemory` abstraction + `createSharedMemory(sdk, size)` factory:
  API >= 26 -> `NativeSharedMemory` (ashmem), API 21-25 ->
  `FallbackSharedMemory`. Native Android API calls guarded for the device build;
  selection logic is portable and host-tested (SPEC §5 example pattern).
- Compiled into `yaglt_core` unconditionally (no Android header deps); new
  `tests/unit/android_capabilities_test.cpp` covers identity, SDK->impl
  selection, and both native/fallback map paths.
- Validation: default + sanitizer suites green (66/66).

2026-08-26 (glGetString frontend entry point, this session)
- Implemented `glGetString` (SPEC §22.2) in the frontend dispatch. `Context::
  getString` returns VENDOR="YAGLT", RENDERER="YAGLT", VERSION=
  "4.6.0 Compatibility Profile YAGLT" (major.minor.release per spec, impl-
  dependent suffix), SHADING_LANGUAGE_VERSION="4.60", EXTENSIONS="" (honest:
  none exposed). Unknown name -> GL_INVALID_ENUM + nullptr.
- Added GLubyte + GL_VENDOR/GL_RENDERER/GL_VERSION/GL_EXTENSIONS/
  GL_SHADING_LANGUAGE_VERSION to `gl_types.hpp`. New
  `tests/unit/getstring_test.cpp` covers values, invalid-name error, no-context.
- Validation: default + sanitizer 69/69 green.

## Recent Work

2026-08-26 (honest-Unsupported shader stages, this session)
- Per-stage shader capability gating (SPEC §8/§19, journal Next Step item:
  geometry/tessellation/compute honest-Unsupported paths). `Context::createShader`
  now maps each stage to its `Feature`: vertex/fragment→ShaderObjects,
  geometry→GeometryShaders, tessellation→TessellationShaders,
  compute→ComputeShaders. Unsupported stage → `GL_INVALID_OPERATION` + name 0
  (no fake success); unknown stage → `GL_INVALID_ENUM`. Replaces the prior
  ShaderObjects-only check.
- New `tests/unit/shader_stage_test.cpp` covers vertex/fragment success, the
  three unsupported stages (mock profile marks them Unsupported), and unknown
  stage → INVALID_ENUM.
- Validation: default + sanitizer suites green.

## Recent Work

2026-08-26 (transform feedback API, this session)
- Implemented transform-feedback object lifecycle + capture (SPEC §13.3), closing
  the journal's transform-feedback item. Added `BackendTransformFeedback` resource
  (begin/end/pause/resume, default no-op) and `IResourceFactory::
  createTransformFeedback`; `MockTransformFeedback` records calls; `GLESBackend
  TransformFeedback` drives the real driver via newly resolved optional loader
  symbols (glGen/Bind/Begin/End/Pause/ResumeTransformFeedback, ES 3.0+).
  Frontend `TransformFeedbackObject` + `Context` gen/bind/delete and begin/end/
  pause/resume, all capability-gated by `TransformFeedback`; begin-while-active,
  end-while-inactive, pause/resume-when-illegal → `GL_INVALID_OPERATION`. Public
  `glGenTransformFeedback`/`glBindTransformFeedback`/`glDeleteTransformFeedback`/
  `glBeginTransformFeedback`/`glEndTransformFeedback`/`glPauseTransformFeedback`/
  `glResumeTransformFeedback` added. New `tests/unit/transform_feedback_test.cpp`.
- Validation: default + sanitizer + translate (Mesa) suites green.

2026-08-26 (GLSL version/profile capability check, this session)
- Journal Next Step #3: `Context::compileShader` now rejects shaders whose
  `#version` exceeds the translatable ceiling before involving the translator or
  backend (SPEC §8: fail fast, honest reporting). Desktop profiles capped at 4.60,
  ES at 3.20. A `parseVersionDirective` helper extracts `NNN [profile]`; rejection
  sets COMPILE_STATUS false + an info-log diagnostic (standard GL semantics, no
  glGetError). Supported versions still proceed to translation.
- New `tests/unit/glsl_version_check_test.cpp`. Validation: default + sanitizer +
  translate (Mesa) suites green.

2026-08-26 (info-log retrieval, this session)
- Added `glGetShaderInfoLog` / `glGetProgramInfoLog` (SPEC §7.3 / §7.14) to
  complement the GL_INFO_LOG_LENGTH query added previously. `Context` gained
  `getShaderInfoLog` / `getProgramInfoLog` that copy the log into the caller
  buffer (nul-terminated; `*length` excludes the nul; bufSize==0 writes nothing),
  validate the object (unknown → `GL_INVALID_OPERATION`), and otherwise leave
  pending errors untouched (standard glGetError semantics). Wired through `gl_api`.
- New `tests/unit/infolog_test.cpp` covers copy + length, truncation to bufSize,
  unknown-object error, and the program-link-failure log path.
- Validation: default + sanitizer + translate (Mesa) suites green.

2026-08-26 (shader/program query coverage, this session)
- Expanded `glGetShaderiv` / `glGetProgramiv` to full SPEC §7.3 / §7.14
  coverage. `Context::getShaderiv` returns SHADER_TYPE, COMPILE_STATUS,
  DELETE_STATUS, SHADER_SOURCE_LENGTH, INFO_LOG_LENGTH; `getProgramiv` returns
  LINK_STATUS, DELETE_STATUS, ATTACHED_SHADERS, INFO_LOG_LENGTH, and the
  active uniform/attribute/uniform-block counts (delegated to a new
  `BackendProgram::activeUniformCount`/`activeAttributeCount`/
  `activeUniformBlockCount`, default 0). Unknown pname → `GL_INVALID_ENUM`;
  unknown object → `GL_INVALID_OPERATION`. The mock reports 0 for the
  active counts (no reflection) and the GLES backend can override later.
- New `tests/unit/shader_program_query_test.cpp`. Validation: default +
  sanitizer + translate (Mesa) suites green.

## Recent Work

2026-08-26 (viewport + scissor state, this session)
- Implemented viewport (`glViewport`) and scissor box (`glScissor`) pipeline state
  tracking (SPEC §10). `GLStateTracker` gained `setViewport`/`setScissor`;
  `GLStateSink` gained `setViewport`/`setScissor`. The scissor *test* remains a
  `GL_SCISSOR_TEST` capability toggled by `glEnable`/`glDisable`, separate from
  the scissor box. Both pushed only when changed (SPEC §10: skip redundant native
  calls). `MockBackend` records them; `GLESBackend` drives `glViewport`/`glScissor`
  via the `GLESLib` runtime loader (added to the required symbol set). `Context`
  + `gl_api` expose `glViewport`/`glScissor`; state flushes at draw/flush time.
- New `tests/unit/viewport_scissor_test.cpp` covers change-only push, scissor-box
  vs scissor-test distinction, and flush-at-draw. Also removed a stray unused
  `GL_TRANSFORM_FEEDBACK` constant from the frontend type layer.
- Validation: default 90/90, sanitizer 90/90 green.

2026-08-26 (depth range state, this session)
- Added `glDepthRange`/`glDepthRangef` pipeline state (SPEC §10 depth-range
  category, previously untracked). `GLStateTracker` gained `setDepthRange`;
  `GLStateSink` gained `depthRange(double,double)`. Mock records it; GLESBackend
  drives `glDepthRangef` via the `GLESLib` loader. `gl_api` exposes both
  entry points; default range (0,1) matches the backend initial state so no
   redundant native push occurs. Verified by `viewport_scissor_test`.

- [x] Framebuffer clear (SPEC §2.1, this session).
  - `glClearColor`/`glClearDepth`/`glClearDepthf` record the per-context clear
    values in `GLStateTracker` (new `ClearColorState`/`ClearDepthState`), pushed
    only on change through `GLStateSink` (new `clearColor`/`clearDepth`, SPEC §10).
  - `glClear(mask)` validates the mask (bits outside color/depth/stencil →
    `GL_INVALID_VALUE`, no native call) then flushes tracked state and issues the
    native clear via the new `IGraphicsBackend::clear(uint32_t)` pure virtual.
  - `GLESLib` resolves `glClearColor`/`glClearDepthf`/`glClear` (required);
    `GLESBackend` drives them (depth promoted to float for GLES). `MockBackend`
    records color/depth/clear. New `tests/unit/clear_test.cpp` covers change-only
    push, invalid-mask error, and clear-after-flush.
  - Validation: default 96/96, sanitizer 96/96, translate (Mesa) 103/103 green.

- [x] Command stream flush/finish (SPEC §2.1, this session).
  - `glFlush`/`glFinish` forward to the backend via new `IGraphicsBackend::
    flush()`/`finish()` pure virtuals. `GLESLib` resolves `glFlush`/`glFinish`
    (required); `GLESBackend` drives them; `MockBackend` records the calls.
    `Context`/`gl_api` expose `flushCommands`/`finishCommands`/`glFlush`/`glFinish`.
  - Validation: default 97/97, sanitizer 97/97 green.

- [x] Framebuffer readback (SPEC §2.1, this session).
  - `glReadPixels` flushes tracked state then reads the bound framebuffer via the
    new `IGraphicsBackend::readPixels` pure virtual. Non-positive width/height →
    `GL_INVALID_VALUE`, no backend call. `GLESLib` resolves `glReadPixels`
    (required); `GLESBackend` drives it; `MockBackend` records the call.
    `Context`/`gl_api` expose `readPixels`/`glReadPixels`. New
    `tests/unit/readpixels_test.cpp` covers forward-after-flush and invalid-size.
  - Validation: default 99/99, sanitizer 99/99, translate (Mesa) 106/106 green.

- [x] Blend state completeness: separate factors/equations + constant color (SPEC
   §10 / §17.3, this session).
   - Extended `GLStateTracker::BlendState` to carry independent RGB/alpha factors
     and equations; added `setBlendFuncSeparate`/`setBlendEquationSeparate`/
     `setBlendColor`. `glBlendFunc`/`glBlendEquation` now set both RGB and alpha
     equal (unchanged semantics). `apply()` pushes `blendFuncSeparate` +
     `blendEquationSeparate` as one category and `blendColor` as an independent
     category, so only changed state reaches the driver (SPEC §10).
   - `GLStateSink` interface renamed `blendFunc`→`blendFuncSeparate`,
     `blendEquation`→`blendEquationSeparate` and gained `blendColor` (plain int/
     float types). `MockBackend` records all three (and the resolved RGB/alpha
     factors + equations + constant color); `GLESBackend` drives the real driver
     via `glBlendFuncSeparate`/`glBlendEquationSeparate`/`glBlendColor` (resolved
     as optional symbols in `GLESLib`, with glBlendFunc/glBlendEquation fallback
     when the separates are absent). Added the three GL constants + blend factor/
     equation constants to `gl_types.hpp`.
   - Public `gl_api` exposes `glBlendFuncSeparate`/`glBlendEquationSeparate`/
     `glBlendColor`. New `tests/unit/blend_test.cpp` covers separate RGB/alpha
     factors, `glBlendFunc` mapping to equal pairs, separate equations, constant
     color push-only-on-change, and color-vs-func independence.
   - Validation: default 104/104, sanitizer 104/104 green.

- [x] Stencil API exposure (SPEC §17.3.3, this session).
   - The tracker, `GLStateSink`, `MockBackend` and `GLESBackend` already tracked
     stencil func/op/mask (pushed by `apply()`), but the public surface lacked the
     entry points. Added `glStencilFunc`/`glStencilOp`/`glStencilMask` to
     `gl_api`; they route into `GLStateTracker::setStencil*`, which sets front and
     back stencil state identically (per the GL spec) and is flushed at
     draw/flush time. Added the stencil comparison/operation constants
     (`GL_NEVER`…`GL_ALWAYS`, `GL_KEEP`…`GL_DECR_WRAP`, `GL_STENCIL_TEST`) to
     `gl_types.hpp`. New `tests/unit/stencil_test.cpp` verifies the stencil
     category is pushed only when it changes and that defaults (func=ALWAYS,
     ops=KEEP, mask=all-ones) produce no push.
   - Validation: default 105/105, sanitizer 105/105 green.

## Recent Work

2026-08-26 (texture units + active texture, this session)
- Implemented texture image units (SPEC §2.1). `glActiveTexture(GL_TEXTURE0+i)`
  selects the active unit; an out-of-range value reports `GL_INVALID_ENUM`
  honestly. `glBindTexture(target, tex)` binds to (active unit, target); the
  frontend now tracks per-unit, per-target bindings via `GLStateTracker` instead
  of a single global bound texture. `glGetIntegerv(GL_ACTIVE_TEXTURE)` and
  `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` read the tracked state.
- `GLStateSink` gained `activeTexture(unit)` + `bindTexture(target, texture)`;
  `apply()` now pushes changed per-unit bindings to the backend at flush time,
  switching the driver active unit only when it differs from the last pushed
  unit (SPEC §10: no redundant native calls). Implemented by `MockBackend`,
  `GLESBackend` (resolves `glActiveTexture`, converts the frontend name to the
  native id via the registered name map), and the test `RecordingSink`.
- Fixed a latent backend bug: `GLESBackendTexture::texImage2D`/`texParameteri`
  now bind the texture to the target on the active unit before the driver call,
  so multi-texture uploads target the correct texture (previously relied on
  whatever was bound on the driver).
- New `tests/unit/texture_unit_test.cpp` covers active-unit selection + query,
  out-of-range `GL_INVALID_ENUM`, out-of-range unit validation, per-unit binding
  push correctness (incl. unbinding via delete), and the `glActiveTexture`/
  `glBindTexture` public surface. `glBindTexture`/`boundTexture` callers in
  existing tests updated to the (target, name) signature.
- Validation: default 121/121, sanitizer 121/121, and translate (Mesa) e2e all
  green.

2026-08-26 (buffer object completeness, this session)
- Implemented the core buffer-data surface (SPEC §6): `glBufferSubData`,
  `glBufferStorage` (immutable, capability-gated by `ImmutableBufferStorage`),
  `glCopyBufferSubData`, `glGetBufferParameteriv`, and `glMapBuffer` /
  `glMapBufferRange` / `glUnmapBuffer`. Added backend interface methods on
  `BackendBuffer` (`bufferSubData`, `bufferStorage`, `copySubData`,
  `mapBufferRange`, `unmapBuffer`) and wired `GLESBackendBuffer` to forward to
  `glBufferSubData` / `glBufferStorage` / `glCopyBufferSubData` / `glMapBufferRange`
  / `glUnmapBuffer` (resolved optionally in `GLESLib::load`). The frontend
  `BufferObject` owns an authoritative CPU data store so subdata/copy/map/query
  semantics are fully defined on the mock and mirror the driver copy on real
  backends. Full validation: bound-buffer requirement, in-bounds checks,
  immutable re-allocation rejection, unsupported `ImmutableBufferStorage`
  reporting `GL_INVALID_OPERATION`, double-map / unmap-not-mapped errors.
- New `tests/unit/buffer_completeness_test.cpp` (12 cases) covers all of the
  above plus the public `gl*` surface. Verified: default + sanitizer suites
  green (162/162). Coverage reassessed in `docs/coverage-core.md` (now 100/435
  ≈ 22.3% core prototype coverage).

## Recent Work

2026-08-26 (rasterization scalar state, this session)
- Implemented §11 rasterization scalar controls (SPEC §11): `glPointSize`,
  `glLineWidth`, `glPolygonOffset`. `GLStateTracker` gained `setPointSize` /
  `setLineWidth` / `setPolygonOffset`; `GLStateSink` gained `pointSize` /
  `lineWidth` / `polygonOffset`. The three are independent scalar values pushed
  to the backend only when the relevant one changed (SPEC §10: no redundant
  native calls). `GL_POLYGON_OFFSET_FILL` added to the tracked-capability set so
  `glEnable`/`glDisable`/`glIsEnabled`/`glGet` treat it honestly. `glGet*` (int/
  float/ double) supports `GL_POINT_SIZE`, `GL_LINE_WIDTH`,
  `GL_POLYGON_OFFSET_FACTOR`, `GL_POLYGON_OFFSET_UNITS` (frontend owns the values).
- GLES backend drives `glPointSize` / `glLineWidth` / `glPolygonOffset` via new
  `GLESLib` loader symbols (all core in GLES 2.0+); `MockBackend` records every
  push. Public `gl_api` exposes the three entry points.
- New `tests/unit/raster_test.cpp` covers push-only-on-change (per-value), glGet
  round-trips, and `GL_POLYGON_OFFSET_FILL` capability. Added the new sink
  overrides to the test `RecordingSink` / `UnitRecordingSink` stubs.
- Validation: default 162/162, sanitizer 162/162, translate (Mesa) all green.
- Coverage bumped in `docs/coverage-core.md` (now 100/490 = 20.4% full,
  100/435 = 23.0% … actually 23.0% core prototype coverage).

2026-08-26 (texture sub-image + copy specification, this session)
- Implemented texture sub-image and copy-from-framebuffer commands (SPEC §8.5/
  §8.6): `glTexSubImage1D`/`glTexSubImage2D`/`glTexSubImage3D` and
  `glCopyTexImage1D`/`glCopyTexImage2D`. Frontend `Context` validates (bound
  texture required, level must be pre-allocated by a TexImage, non-negative
  level/offset/dims, region must fit inside the allocated level →
  GL_INVALID_VALUE, no texture → GL_INVALID_OPERATION; CopyTexImage border must
  be 0). Backend `BackendTexture` gained the 5 virtuals; `MockTexture` records
  each; `GLESBackendTexture` drives `glTexSubImage2D`/`glTexSubImage3D`/
  `glCopyTexImage2D` via newly resolved `GLESLib` loader symbols (1D is a no-op
  on GLES, which has no 1D textures). `TextureObject` records subimage metadata.
  New `tests/unit/texsubimage_test.cpp` (10 cases) covers upload recording,
  missing-level / no-texture / negative-dim / out-of-region errors, and copy
   paths. Coverage bumped to 105/490 (21.4%) full / 105/435 (24.1%) core.
- Validation: default 174/174 green; sanitizer 174/174 green.

2026-08-26 (texture parameter float/vector expansion, this session)
- Expanded texture parameter setting to full SPEC §8 surface: `glTexParameterf`,
  `glTexParameterfv`, `glTexParameteriv`, and `glGetTexParameterfv`. `BackendTexture`
  gained `texParameterf`/`texParameterfv`/`texParameteriv` virtuals; `MockTexture`
  records each; `GLESBackendTexture` drives `glTexParameterf`/`glTexParameterfv`/
  `glTexParameteriv` via newly resolved `GLESLib` loader symbols. `TextureObject`
  now stores scalar-float, float-vector and int-vector params. Frontend validates
  (bound texture required, null/non-positive count → GL_INVALID_VALUE). `glGetTexParameterfv`
  reads the stored float scalar or first component of a float vector (0.0f default).
  Extended `tests/unit/texparam_query_test.cpp` (10 new cases). Coverage now
  109/490 (22.2%) full / 109/435 (25.1%) core.
- Validation: default 180/180 green; sanitizer 180/180 green.

2026-08-26 (whole-framebuffer buffer selection, this session)
- Implemented `glDrawBuffers` / `glReadBuffer` (SPEC §15 / §16). Frontend tracks
  the draw-buffer set and read buffer in `GLStateTracker` (new `FramebufferBufferState`,
  change-detected like the rest of the pipeline); the selection is pushed through
  `GLStateSink::drawBuffers` / `readBuffer` at the next state flush (SPEC §10:
  redundant native calls skipped). `MockBackend` and `GLESBackend` implement the
  sink methods (`glDrawBuffers` / `glReadBuffer` via newly resolved `GLESLib`
  loader symbols). Validation: non-positive count or null bufs → GL_INVALID_VALUE;
  an invalid draw/read-buffer enum → GL_INVALID_ENUM. Added `GL_NONE`, `GL_FRONT*`,
  `GL_BACK*`, `GL_COLOR_ATTACHMENT1..15` constants to `gl_types.hpp`. New
  `tests/unit/framebuffer_buf_test.cpp` (7 cases). Coverage now 111/490 (22.7%)
  full / 111/435 (25.5%) core.
- Validation: default 187/187 green; sanitizer pending.

2026-08-27 (color logic op + blit/invalidate framebuffer — recovered from a
crashed agent, this session)
- SPEC §17.3.4 / §15 / §16: implemented `glLogicOp`, `glBlitFramebuffer`,
  `glInvalidateFramebuffer`, `glInvalidateSubFramebuffer`. A prior agent added
  these plus the supporting `GLStateSink::logicOp` (plain int) and
  `IGraphicsBackend::blitFramebuffer` / `invalidateFramebuffer` (full + sub
  forms) but left the tree in a broken state (build failed). Recovered:
  - Fixed a redefinition error: `GL_INVERT` (0x150A) was declared twice in
    `gl_types.hpp` (stencil-op block + logic-op block); it is shared by
    `glStencilOp` and `glLogicOp`, so it is now declared once (kept in the
    stencil block, referenced from the logic-op block).
  - Added the missing `GLStateSink::logicOp` override to the three test
    `UnitRecordingSink` stubs (`state_test`, `texture_unit_test`,
    `dsa_texture_test`) so the test suite links again.
- Frontend behavior: `Context::logicOp` is capability-gated by `Feature::LogicOp`
  (Native on GLES 3.0+; recorded in `GLStateTracker::setLogicOp`, pushed via the
  sink only when the mode changes, SPEC §10). `blitFramebuffer` validates the mask
  (bits outside color/depth/stencil → GL_INVALID_VALUE, no backend call) then
  flushes tracked state and forwards. `invalidateFramebuffer`/`invalidateSubFrame-
  buffer` validate numAttachments<0 or null-attachments-with-count>0 →
  GL_INVALID_VALUE and negative rect → GL_INVALID_VALUE. Backend resolves
  `glLogicOp`/`glBlitFramebuffer`/`glInvalidateFramebuffer`/`glInvalidateSubFrame-
  buffer` as optional `GLESLib` symbols (so load() still succeeds on drivers
  lacking them; capability reports Unsupported). `glGetIntegerv(GL_LOGIC_OP_MODE)`
  returns the tracked mode.
- New `tests/unit/logicop_blit_invalidate_test.cpp` (4 cases) covers push-only-on-
  change for logic op (default GL_COPY), capability gate, blit forwarding + mask
  validation, and both invalidate forms + null-attachment validation.
  Coverage now 135/490 (27.6%) full / 135/435 (31.0%) core.
- Validation: default 199/199 green; sanitizer 199/199 green.

2026-08-27 (color write mask, this session — small step)
- SPEC §17.3.6: implemented `glColorMask`. Frontend tracks the four per-channel
  booleans in `GLStateTracker::setColorMask`; pushed through the new
  `GLStateSink::colorMask(bool,bool,bool,bool)` only when the set of masked
  channels changes (SPEC §10). `MockBackend` records it; `GLESBackend` drives
  `glColorMask` via a newly resolved (required, core in GLES 2.0) `GLESLib`
  symbol. `glGetIntegerv`/`glGetBooleanv(GL_COLOR_WRITEMASK)` return the tracked
  channels (4 values). `GL_COLOR_WRITEMASK` (0x0C23) added to `gl_types.hpp`.
  Public `gl_api` exposes `glColorMask(GLboolean,GLboolean,GLboolean,GLboolean)`.
- New `tests/unit/colormask_test.cpp` (2 cases) covers push-only-on-change +
  channel recording and the `GL_COLOR_WRITEMASK` query. Coverage now 136/490
  (27.8%) full / 136/435 (31.3%) core.
- Validation: default + sanitizer suites green.

2026-08-27 (sample coverage, this session — small step)
- SPEC §17.3.6 multisample: implemented `glSampleCoverage`. Frontend tracks the
  coverage value (float, default 1.0) + invert flag in `GLStateTracker::
  setSampleCoverage`; pushed through the new `GLStateSink::sampleCoverage(float,
  bool)` only when the value or invert changes (SPEC §10). `MockBackend` records
  it; `GLESBackend` drives `glSampleCoverage` via a newly resolved (core in GLES
  2.0) `GLESLib` symbol. `glGetFloatv(GL_SAMPLE_COVERAGE_VALUE)` /
  `glGetBooleanv(GL_SAMPLE_COVERAGE_INVERT)` return the tracked state. Query
  constants added to `gl_types.hpp`. Public `gl_api` exposes
  `glSampleCoverage(GLfloat, GLboolean)`.
- New `tests/unit/samplecoverage_test.cpp` (2 cases) covers push-only-on-change
  + recording and the tracked-state queries. Coverage now 137/490 (28.0%) full /
  137/435 (31.5%) core.
- Validation: default + sanitizer suites green.

## Next Steps

 0. **PRIMARY GOAL: implement all 490 OpenGL 4.6 spec command prototypes.**
     Per `docs/coverage-core.md` (2026-08-27) now 137/490 (28.0%) have a
      frontend entry point; core-only is 137/435 (31.5%). The standing
     objective is to reach **full coverage of all 490 spec command prototypes** —
    core profile fully, plus the compatibility-profile (removed-in-core)
    commands from Appendix E.2.2 once the core majority is landed (gated per
    `docs/feature-matrix.md` "Compatibility Profile"). Track progress against
    the 100 covered / 390 remaining prototypes. Work the priority gaps listed in
    `docs/coverage-core.md` (texture completeness, full DSA `Named*`
    surface, queries + sync fences, `DrawBuffers`/`BlitFramebuffer`, draw
    expansion, program pipelines, geometry/tessellation/compute emulation).
1. **Texture units + DSA done.** Sampler objects and DSA texture binding
     (`glBindTextureUnit` / `glBindTextures`, SPEC §2.1) implemented (see
     Completed above). Next texture-correctness item: `glActiveTexture`
     interaction with the FBO / texture-completeness queries (e.g. resolve a
     texture's per-unit binding when attaching to an FBO, and surface
     `GL_FRAMEBUFFER_INCOMPLETE_*` reasons beyond the structural check).
2. Continue SPEC phases (§5 Android platform capabilities + SDK 21 fallback
   abstraction, §6 compatibility/emulation scaffolding, geometry/tessellation
   honest-Unsupported paths, SSBO storage-block translation / transform-feedback).
3. Add a GLSL `version`/`profile` capability check so the frontend can reject
   unsupported desktop features before translation rather than at link time.
4. **Planned deployment mode:** build YAGLT as a `libEGL.so` drop-in shim — renamed
   shared lib placed next to any program (or via `LD_LIBRARY_PATH`/`LD_PRELOAD`),
   forwarding the real system EGL/GLES driver internally while routing GL calls
   through YAGLT's frontend/backend. Like Mesa's `libEGL` loader: API surface +
   dispatcher to the real driver. Enables transparent context wrapping + call
   interception for unmodified apps. Thin dispatch layer, not a new backend.
   (See `docs/architecture.md` "Planned: libEGL.so drop-in wrapper".)
 5. Commit each coherent step; update this journal.
 6. **Compatibility Profile** (deprecated fixed-function API) is planned but
    gated — see `docs/feature-matrix.md` "Compatibility Profile": only enable it
    when EGL explicitly selects a compat profile; only begin implementation once
    a majority of core is done and remaining core is slower/harder; emulate via
    record-then-replay into a generated GLSL shader. Shipping GLSL in the tree is
    fine.

## Recent Work

2026-08-26 (query objects + sync fences, this session)
- Implemented Query objects (SPEC §4 / §19) and Sync fences (SPEC §4 / §20,
  ARB_sync) — closing the journal's queries+sync priority gap and adding 20
  command prototypes toward full 490 coverage (now 131/490 = 26.7% full,
  131/435 = 30.1% core).
- **Query objects**: `glGenQuery`/`glGenQueries`/`glDeleteQuery`/`glDeleteQueries`/
  `glIsQuery`/`glBeginQuery`/`glEndQuery`/`glBeginQueryIndexed`/`glEndQueryIndexed`/
  `glGetQueryiv`/`glGetQueryObjectiv`/`glGetQueryObjectuiv`/`glGetQueryObjecti64v`/
  `glGetQueryObjectui64v`. Object model `QueryObject` owns an opaque
  `BackendQuery`; one active query per target; begin-already-active / end-with-
  none / begin-ungenerated-id → `GL_INVALID_OPERATION`; indexed variants require a
  counter target (PRIMITIVES_GENERATED / TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN)
  else `GL_INVALID_ENUM`. `getQueryiv` CURRENT_QUERY reads the active id (frontend
  state, SPEC §10); results read the cached backend value. `Feature::Queries`
  added; Native in mock + GLES (ES 3.0).
- **Sync fences**: `glFenceSync`/`glClientWaitSync`/`glWaitSync`/`glDeleteSync`/
  `glIsSync`/`glGetSynciv`. `fenceSync` returns an opaque `GLsync` that is the
  frontend-owned `SyncObject` pointer (the raw pointer never reaches the backend,
  SPEC §3) and flushes the backend so the fence will eventually be signaled;
  unknown condition → `GL_INVALID_ENUM`. `getSynciv` reports SYNC_STATUS /
  SYNC_CONDITION / SYNC_FLAGS; non-sync → `GL_INVALID_OPERATION`, `waitSync`
  non-sync → `GL_INVALID_VALUE`; `deleteSync` on a non-sync is a silent no-op.
  `Feature::SyncObjects` added; Native in mock + GLES (ES 3.0).
- **Backend**: `BackendQuery` + `IResourceFactory::createQuery`; `MockQuery`
  records begin/end and a test-injected result; `GLESBackendQuery` drives
  `glGenQueries`/`glBeginQuery`/`glEndQuery`/`glGetQueryObjectuiv(ui64v)` via
  optional `GLESLib` symbols (resolved defensively so load() still succeeds when
  absent). `GLsync`/`GLint64`/`GLuint64` added to the frontend type layer.
- New `tests/unit/query_sync_test.cpp` (mock path: gen/delete/is, begin/end
  lifecycle + validation, indexed-target gating, getQuery* result/counter reads,
  unsupported-capability paths, full sync fence lifecycle + error paths). All
  suites green: default 187→**, sanitizer, and translate (Mesa) builds pass.

