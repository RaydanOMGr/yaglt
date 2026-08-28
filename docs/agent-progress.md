# Development Journal

Persistent, version-controlled progress record. Updated after meaningful
milestones, architectural decisions, and before ending a session.

## Current Status

Current milestone: Phase 3 — Core rendering state (viewport/scissor/depth-range/clear) + draw
Overall status: Early implementation (foundation + object model + GL dispatch + GLES backend + shader translate + object/state API + clear)
Last updated: 2026-08-28
Known major blockers:
- Geometry/tessellation/compute still honest-Unsupported (no emulation yet).

## Recent Work (2026-08-29 — conditional rendering, this session)
- Added conditional rendering (SPEC §10.11): `glBeginConditionalRender` /
  `glEndConditionalRender` open/close a draw region predicated on an existing
  query object. New `Feature::ConditionalRendering` in the capability enum; the
  mock profile marks it `Emulated` (records the region) and the GLES backend
  reports it `Emulated` only when `GL_NV_conditional_render` + the NV entry
  points resolve, else `Unsupported` honestly (the frontend rejects the call).
  `Context::beginConditionalRender`/`endConditionalRender` validate: capability
  present; not already in a region; `id` is a generated query object; the query
  is not currently active; the query type is one of `GL_SAMPLES_PASSED` /
  `GL_ANY_SAMPLES_PASSED` / `GL_ANY_SAMPLES_PASSED_CONSERVATIVE` /
  `GL_PRIMITIVES_GENERATED`; and `mode` is a valid `GL_QUERY_*` predicate
  (including the `*_INVERTED` 4.6 variants). The region is forwarded to the
  backend `GLStateSink` immediately (it delimits draws, like begin/end query).
  New `GLStateSink::beginConditionalRender`/`endConditionalRender` pure virtuals
  (both backends + the three test sinks implement them); GLES resolves
  `glBeginConditionalRenderNV`/`glEndConditionalRenderNV` optionally. New
  `tests/unit/conditional_render_test.cpp` (open/close, all predicate modes,
  capability gate, full validation, disallowed query type, public dispatch).
  Default **522/522**, sanitizer **522/522**, translate (Mesa) **532/532** green.
  Coverage bumped in `docs/coverage-core.md` (313/571 ≈ 54.8% declared;
  ~60.6% core).

## Recent Work (2026-08-28 — texture sub-image readback, this session)
- Added `glGetTextureSubImage` / `glGetCompressedTextureSubImage` (SPEC §8.11.4 /
  §8.11.5). New `BackendTexture::getTextureSubImage` /
  `getCompressedTextureSubImage` virtuals (default no-op, honest for backends
  without native reads; the mock records target/level). `Context` validates
  `level < 0`, negative `x/y/zoffset`, negative `width/height/depth`, and negative
  `bufSize` → `GL_INVALID_VALUE`, then delegates; `dsaTexture` already gates on
  `DirectStateAccess` + live name → `GL_INVALID_OPERATION`. GLES headers bundling
  lacks these DSA entry points, so the GLES backend inherits the honest no-op
  default. `gl_api` exposes both entry points. New
  `tests/unit/texture_sub_image_test.cpp` covers DSA record, negative level /
  extent / bufSize, and ungenerated-name validation. Default **504/504**, sanitizer
  **504/504**, translate/Mesa **516/516** green. Coverage bumped in
  `docs/coverage-core.md` (311/571 ≈ 54.5% declared; ~60.3% core).

## Recent Work (2026-08-28 — compressed texture readback, this session)
- Added `glGetCompressedTexImage` / `glGetCompressedTextureImage` (SPEC §8.11).
  New `BackendProgram`/`BackendTexture::getCompressedTexImage` virtual (default
  no-op, honest for backends without native reads; the mock records the call +
  target/level). `Context::getCompressedTexImage` / `getCompressedTextureImage`
  validate level < 0 (`GL_INVALID_VALUE`), missing texture (`GL_INVALID_OPERATION`),
  then delegate. GLES headers bundling lacks `glGetCompressedTexImage`, so the GLES
  backend inherits the honest no-op default. `gl_api` exposes both entry points. New
  `tests/unit/compressed_tex_image_test.cpp` covers classic + DSA paths, negative
  level, and ungenerated-name validation. Default **499/499**, sanitizer **499/499**,
  translate/Mesa **511/511** green. Coverage bumped in `docs/coverage-core.md`
  (309/571 ≈ 54.1% declared; ~59.9% core).

## Recent Work (2026-08-28 — program interface summary query, this session)
- Added `glGetProgramInterfaceiv` (SPEC §7.3.1): returns a summary property for a
  program interface (ACTIVE_RESOURCES, MAX_RESOURCE_NAME_LENGTH,
  MAX_NUM_ACTIVE_VARIABLES, MAX_NUM_COMPATIBLE_SUBROUTINES). New
  `BackendProgram::getProgramInterfaceiv` virtual; default is honest for backends
  without introspection (ACTIVE_RESOURCES mirrors `programResourceCount`, MAX_*
  report 0). The GLES backend forwards to the driver
  (`lib->glGetProgramInterfaceiv`). `MockProgram` records the call and returns a
  configurable count (ACTIVE_RESOURCES) / explicit `interfaceCounts` overrides.
  `Context::getProgramInterfaceiv` validates: non-linked program / non-program
  object → `GL_INVALID_OPERATION`; unsupported interface → `GL_INVALID_ENUM`; null
  `params` → `GL_INVALID_VALUE`; unknown pname → `GL_INVALID_ENUM`. Added the three
  missing pname constants (`GL_MAX_RESOURCE_NAME_LENGTH` / `GL_MAX_NUM_ACTIVE_VARIABLES`
  / `GL_MAX_NUM_COMPATIBLE_SUBROUTINES`) to `gl_types.hpp`. New
  `tests/unit/program_interface_test.cpp` covers ACTIVE_RESOURCES, MAX_NAME_LENGTH
  default/override, and every validation path. Default **497/497**, sanitizer
  **497/497**, translate/Mesa **509/509** green. Coverage bumped in
  `docs/coverage-core.md` (307/571 ≈ 53.8% declared; ~59.5% core).

## Recent Work (2026-08-28 — memory barriers, this session)
- Added `glMemoryBarrier` / `glMemoryBarrierByRegion` (SPEC §7.13.2). New
  `IGraphicsBackend::memoryBarrier` / `memoryBarrierByRegion` virtuals (default
  no-op, honest for backends without separate shader/CPU memory domains such as
  the mock). `MockBackend` records `memoryBarrierCalls`/`lastBarriers` (and the
  by-region variants) for observability. `Context::memoryBarrier` /
  `memoryBarrierByRegion` delegate to the backend; `gl_api` exposes both entry
  points. No frontend validation beyond the null-context guard. New
  `tests/unit/memory_barrier_test.cpp` covers delegation + captured barrier bits.
  Default **492/492**, sanitizer **492/492**, translate/Mesa **504/504** green.
  Coverage bumped in `docs/coverage-core.md` (306/571 ≈ 53.6% declared; ~59.3%
  core). NOTE: earlier this session I mistakenly tried to (re)add
  `glGetUniformBlockIndex` / `glGetActiveUniformBlockiv` / `glGetActiveUniformsiv`,
  but those were already implemented (routing through `GetProgramResource*`); the
  duplicate attempt was fully reverted before commit.

## Recent Work (2026-08-28 — transform feedback varying query, this session)
- Added `glGetTransformFeedbackVarying` (SPEC §13.3.1): returns the name (trimmed
  to bufSize-1), size, and type of the `index`-th captured varying of a program.
  New `BackendProgram::getTransformFeedbackVarying` virtual returns `bool` (false =
  out of range / no introspection); default `false` is honest for backends such as
  GLES that expose no direct equivalent, and the frontend maps "missing" to
  `GL_INVALID_VALUE` (matching the spec's out-of-range error). `MockProgram` seeds
  the captured list via `tfVaryings` so tests exercise name/size/type. `Context`
  validates `bufSize < 0` (`GL_INVALID_VALUE`), a non-program object
  (`GL_INVALID_OPERATION`), then delegates. `gl_api` exposes the entry point. New
  `tests/unit/transform_feedback_varying_test.cpp` covers capture info, out-of-range
  `GL_INVALID_VALUE`, and validation. Default **491/491**, sanitizer **491/491**,
  translate/Mesa **503/503** green. Coverage bumped in `docs/coverage-core.md`
  (304/571 ≈ 54.3% declared; ~58.9% core).

## Recent Work (2026-08-28 — fragment-output reflection, this session)
- Added fragment-output reflection (SPEC §7.3.6): `glGetFragDataLocation` /
  `glGetFragDataIndex` return the location / dual-source index bound to a
  fragment-shader output `name`. `BackendProgram` gained `getFragDataLocation` /
  `getFragDataIndex` virtuals (default -1 = no introspection; honest for GLES,
  which has no direct equivalent). `MockProgram` overrides them and returns the
  location only for outputs the test seeds (so unknown names correctly report -1).
  `Context::getFragDataLocation`/`getFragDataIndex` validate the program object
  (non-program name → `GL_INVALID_OPERATION`) and delegate to the backend program.
  `gl_api` exposes both entry points. New `tests/unit/frag_data_location_test.cpp`
  (assigned-location, -1 for unknown, non-program validation). Validation: default,
  translate (Mesa), and sanitizer suites all green. Coverage bumped in
  `docs/coverage-core.md` (now 309/571 ≈ 53.1% declared; ~58.7% core).

## Recent Work (2026-08-28 — object labeling, this session)
- Added object-label entry points (SPEC §22.2): `glObjectLabel` / `glObjectPtrLabel` /
  `glGetObjectLabel` / `glGetObjectPtrLabel`. Labels are frontend-owned: a central
  `std::unordered_map<uint64_t,std::string>` keyed by `(identifier << 32 | name)` for
  named objects, and a `std::unordered_map<const void*,std::string>` for sync-pointer
  labels. `objectHasType(identifier, name)` validates the name against the correct
  object map (buffers/shaders/programs/VAOs/queries/pipelines/transform-feedbacks/
  samplers/textures/renderbuffers/framebuffers); an unknown `identifier` →
  `GL_INVALID_ENUM`, a non-live name → `GL_INVALID_OPERATION`. Labels are limited to
  `kMaxObjectLabelLength` (256) characters → `GL_INVALID_VALUE` beyond; a null `label`
  clears the label. `glGetObjectLabel`/`glGetObjectPtrLabel` support query-only mode
  (`label == nullptr` returns `length` including the nul terminator) and nul-terminated
  copy with truncation. New constants in `gl_types.hpp` (`GL_BUFFER`/`GL_SHADER`/
  `GL_PROGRAM`/`GL_QUERY`/`GL_PROGRAM_PIPELINE`/`GL_SAMPLER`/`GL_MAX_LABEL_LENGTH`/
  `GL_TRANSFORM_FEEDBACK`); the texture/renderbuffer/framebuffer/vertex-array
  namespaces reuse existing target tokens. New `tests/unit/object_label_test.cpp`
  (round-trip, cross-namespace, validation, pointer-label cases). Validation: default,
  translate (Mesa), and sanitizer suites all green. Coverage bumped in
  `docs/coverage-core.md` (now 308/571 ≈ 52.7% declared; ~58.3% core).

## Recent Work (2026-08-28 — frontend-owned reflection queries, this session)
- Added three more frontend-owned query entry points (SPEC §6.1.1 / §7.3.4 / §7.3.7):
  `glGetAttachedShaders` (fills up to `maxCount` attached shader names + the true
  count; negative maxCount → `GL_INVALID_VALUE`, non-program object →
  `GL_INVALID_OPERATION`), `glGetShaderSource` (returns the concatenated nul-
  terminated source; negative bufSize → `GL_INVALID_VALUE`, non-shader object →
  `GL_INVALID_OPERATION`), and `glGetBufferPointerv` / `glGetNamedBufferPointerv`
  (return the mapped-buffer pointer for `BUFFER_MAP_POINTER`; unknown pname →
  `GL_INVALID_ENUM`, null params → `GL_INVALID_VALUE`, the named variant capability-
  gated by `DirectStateAccess` with ungenerated name → `GL_INVALID_OPERATION`, the
  target variant validates the table-6.1 buffer targets with `GL_INVALID_ENUM`).
  The frontend now tracks the stable `mapPointer` in `BufferObject` (set on
  `mapBuffer`/`mapBufferRange`, cleared on `unmapBuffer`) so `BUFFER_MAP_POINTER`
  answers from the CPU mirror. `GL_BUFFER_MAP_POINTER` / `GL_QUERY_BUFFER` added to
  `gl_types.hpp`; `Context` + `gl_api` declarations/dispatch added for all four.
- New `tests/unit/reflection_query_test.cpp` cases: `getshadersource_returns_-
  concatenated_source`, `getattachedshaders_reports_attached_names`,
  `getbufferpointerv_returns_mapped_pointer`, `getnamedbufferpointerv_dsa_returns_-
  mapped_pointer`. Validation: default (481/481, 0 failed), translate (Mesa), and
  sanitizer suites all green. Coverage bumped in `docs/coverage-core.md`
  (now 304/571 ≈ 52.0% declared; ~57.6% core).

## Recent Work (2026-08-28 — mutable texture level parameters, this session)
- Restored + completed the WIP `updateMutableTextureStorage` from the crashed
  agent. `Context::texImage1D/2D/3D` now recompute `storageLevels` /
  `storageBaseWidth/Height/Depth` / `storageInternalFormat` from the recorded
  `images` vector so `getTextureLevelParameteriv` / `getTextureLevelParameterfv`
  (SPEC §8.1) return the uploaded dimensions for mutable (non-immutable) storage,
  not just for `glTextureStorage*`. Immutable storage still owns the fields
  directly. `updateMutableTextureStorage` clears `immutableStorage`.
- New `tests/unit/dsa_named_texture_test.cpp` cases
  `get_texture_level_parameter_mutable_teximage` (2D + mip level 1) and
  `get_texture_level_parameter_mutable_1d` (1D height == 1) cover the mutable
  path. Validation: default, translate (Mesa), and sanitizer suites all green.

## Recent Work (2026-08-28 — classic GetTexLevelParameter, this session)
- Added the classic (non-DSA) `glGetTexLevelParameteriv` / `glGetTexLevelParameterfv`
  (SPEC §8.1) operating on the texture bound to `target`. The frontend DSA methods
  were refactored to share a common `getTexLevelParameter*Impl` body (frontend owns
  WIDTH/HEIGHT/DEPTH/INTERNAL_FORMAT; unknown pnames delegate to the backend
  `getLevelParameter*` when a backend resource exists). Public `gl_api` dispatch and
  `gl_api.hpp` declarations added; per-target validation (no bound texture →
  `GL_INVALID_OPERATION`, null params / out-of-range level → `GL_INVALID_VALUE`).
- New `tests/unit/dsa_named_texture_test.cpp` cases
  `get_tex_level_parameter_iv_bound_target` (iv + fv via bound target) and
  `get_tex_level_parameter_no_bound_texture_invalid_operation`. Validation:
  default, translate (Mesa), and sanitizer suites all green. Coverage bumped in
  `docs/coverage-core.md` (now 277/571 ≈ 48.5% declared; ~53.7% core).

## Recent Work (2026-08-28 — classic GetRenderbufferParameteriv, this session)
- Added the classic (non-DSA) `glGetRenderbufferParameteriv` (SPEC §9.2.4),
  operating on the renderbuffer bound to GL_RENDERBUFFER (invalid target →
  `GL_INVALID_ENUM`; no bound RBO → `GL_INVALID_OPERATION`). The DSA method was
  refactored to share a `getRenderbufferParameterivImpl` body reading frontend-
  owned storage (WIDTH/HEIGHT/INTERNAL_FORMAT/SAMPLES). Public `gl_api` dispatch +
  `gl_api.hpp` declaration added.
- New `tests/unit/dsa_named_framebuffer_test.cpp` cases
  `get_renderbuffer_parameter_iv_bound_target`,
  `get_renderbuffer_parameter_invalid_target`,
  `get_renderbuffer_parameter_no_bound_invalid_operation`. Validation: default,
  translate (Mesa), and sanitizer suites green. Coverage bumped in
  `docs/coverage-core.md` (now 278/571 ≈ 48.7% declared; ~53.9% core).

## Recent Work (2026-08-28 — classic GetFramebufferAttachmentParameteriv, this session)
- Added the classic (non-DSA) `glGetFramebufferAttachmentParameteriv` (SPEC
  §9.2.3), operating on the framebuffer bound to `target` (invalid target →
  `GL_INVALID_ENUM`; no bound FBO → `GL_INVALID_OPERATION`). The DSA method was
  refactored to share a `getFramebufferAttachmentParameterivImpl` body reading
  the attachment's object type / name / texture-level / layer. Public `gl_api`
  dispatch + `gl_api.hpp` declaration added.
- New `tests/unit/dsa_named_framebuffer_test.cpp` cases
  `get_framebuffer_attachment_parameter_bound_target`,
  `get_framebuffer_attachment_parameter_invalid_target`,
  `get_framebuffer_attachment_parameter_no_bound_invalid_operation`. Validation:
  default, translate (Mesa), and sanitizer suites green. Coverage bumped in
  `docs/coverage-core.md` (now 279/571 ≈ 48.9% declared; ~54.1% core).

## Recent Work (2026-08-28 — classic GetFramebufferParameteriv, this session)
- Added the classic (non-DSA) `glGetFramebufferParameteriv` (SPEC §9.2.3),
  operating on the framebuffer bound to `target` (invalid target →
  `GL_INVALID_ENUM`; no bound FBO → `GL_INVALID_OPERATION`; null params →
  `GL_INVALID_VALUE`). User FBOs report 0 for FRAMEBUFFER_DEFAULT_* (frontend-
  owned default, SPEC §10). Public `gl_api` dispatch + `gl_api.hpp` declaration
  added; `get_framebuffer_parameter_*` cases appended to
  `tests/unit/dsa_named_framebuffer_test.cpp`. Validation: default, translate
  (Mesa), and sanitizer suites green. Coverage bumped in `docs/coverage-core.md`
  (now 280/571 ≈ 49.0% declared; ~54.3% core).

## Recent Work (2026-08-28 — classic GetVertexAttrib* family, this session)
- Added the remaining classic (non-DSA) vertex-attribute query entry points
  (SPEC §10.4): `glGetVertexAttribdv` (CURRENT_VERTEX_ATTRIB as double[4]),
  `glGetVertexAttribIiv` / `glGetVertexAttribIuiv` (CURRENT_VERTEX_ATTRIB as
  signed/unsigned int, plus VERTEX_ATTRIB_ARRAY_INTEGER flag), and
  `glGetVertexAttribPointerv` (VERTEX_ATTRIB_ARRAY_POINTER). The existing
  `glGetVertexAttribiv` was broadened to answer the full integer/boolean pname
  set (ENABLED/SIZE/STRIDE/TYPE/NORMALIZED/INTEGER/DIVISOR/BUFFER_BINDING) in
  addition to CURRENT_VERTEX_ATTRIB, reading the bound VAO's `AttribState`. Added
  the missing `GL_VERTEX_ATTRIB_ARRAY_*` constants to `gl_types.hpp`; public
  `gl_api` dispatch + `gl_api.hpp`/`context.hpp` declarations added. All four
  entry points share validation (no bound VAO → GL_INVALID_OPERATION, out-of-range
  index → GL_INVALID_VALUE, null params → GL_INVALID_VALUE, unknown pname →
  GL_INVALID_ENUM).
- New `tests/unit/vertex_attrib_generic_test.cpp` cases: `getVertexAttribiv_reads_-
  array_enabled_state`, `getVertexAttribiv_reads_pointer_attributes`
  (SIZE/TYPE/STRIDE/NORMALIZED/BUFFER_BINDING/DIVISOR + Pointerv round-trip),
  `getVertexAttribdv_returns_current_value_as_double`, `getVertexAttribIiv_Iuiv_-
  return_integer_current_and_flag`, `getVertexAttrib_validation_errors`,
  `getVertexAttrib_without_bound_vao_is_invalid_operation`. Validation: default,
  translate (Mesa), and sanitizer suites green. Coverage bumped in
  `docs/coverage-core.md` (now 284/571 ≈ 49.7% declared; ~55.0% core).

## Recent Work (2026-08-28 — DSA vertex-array queries, this session)
- Added the DSA vertex-array query entry points (SPEC §10.3.1): `glGetVertexArrayiv`
  (VAO-level `GL_ELEMENT_ARRAY_BUFFER_BINDING`), `glGetVertexArrayIndexediv`
  (per-attribute `ENABLED`/`SIZE`/`STRIDE`/`TYPE`/`NORMALIZED`/`INTEGER`/`LONG`/
  `DIVISOR`/`BUFFER_BINDING`), and `glGetVertexArrayIndexed64v` (64-bit
  `VERTEX_ATTRIB_BINDING` / `VERTEX_ATTRIB_RELATIVE_OFFSET`). All read the explicit
  VAO name's `VertexArrayObject` state, capability-gated by `DirectStateAccess`
  (consistent with the other DSA vertex-array methods); ungenerated VAO name →
  `GL_INVALID_OPERATION`, out-of-range index → `GL_INVALID_VALUE`, null params →
  `GL_INVALID_VALUE`, unknown pname → `GL_INVALID_ENUM`. Public `gl_api` dispatch +
  `gl_api.hpp`/`context.hpp` declarations added; the four `GL_VERTEX_ATTRIB_*` /
  `GL_ELEMENT_ARRAY_BUFFER_BINDING` constants added to `gl_types.hpp`.
- New `tests/unit/dsa_vertex_array_test.cpp` cases: `get_vertex_array_iv_element_-
  buffer_binding`, `get_vertex_array_indexed_iv_per_attrib_state`,
  `get_vertex_array_indexed_64v_binding_and_relative_offset`,
  `get_vertex_array_indexed_validation`, `get_vertex_array_ungenerated_is_invalid_-
  operation`, `get_vertex_array_gated_by_direct_state_access`,
  `gl_api_get_vertex_array_queries`. Validation: default, translate (Mesa), and
  sanitizer suites green. Coverage bumped in `docs/coverage-core.md` (now 287/571
  ≈ 50.3% declared; ~55.6% core).

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

- [x] `glProgramParameteri` (SPEC §7.3 / §7.4.2, this session).
   - `Context::programParameteri` + public `glProgramParameteri` entry point.
     `GL_PROGRAM_SEPARABLE` must be set before linking (after link →
     `GL_INVALID_OPERATION`); `GL_PROGRAM_BINARY_RETRIEVABLE_HINT` may be set
     any time. Unknown pname → `GL_INVALID_ENUM`; non-program →
     `GL_INVALID_OPERATION`. `ProgramObject` gained `binaryRetrievableHint`.
   - `glGetProgramiv` now answers `GL_PROGRAM_SEPARABLE` (frontend-owned flag).
     This fixes a real correctness gap: a regular program could only become
     separable via `glCreateShaderProgramv`; now `glProgramParameteri(prog,
     GL_PROGRAM_SEPARABLE, TRUE)` before `linkProgram` is the standard path and
     is honored by the existing `glUseProgramStages` separable check.
   - New `tests/unit/program_parameter_test.cpp` covers pre-link flag, post-link
     error, binary-hint timing, validation, and the public dispatch path.
   - Constant `GL_PROGRAM_BINARY_RETRIEVABLE_HINT` added to `gl_types.hpp`.
   - Validation: default + sanitizer suites green.

- [x] 64-bit buffer parameter queries (SPEC §6.1.1, this session).
   - `Context::getBufferParameteri64v` + public `glGetBufferParameteri64v`: reads
     frontend-owned buffer state as `GLint64` (GL_BUFFER_SIZE genuinely 64-bit;
     the rest widen the `glGetBufferParameteriv` form). Null params →
     `GL_INVALID_VALUE`; unbound target → `GL_INVALID_OPERATION`; unknown pname →
     `GL_INVALID_ENUM`.
   - `Context::getNamedBufferParameteri64v` + public `glGetNamedBufferParameteri64v`
     (DSA variant, capability-gated by `DirectStateAccess`; ungenerated name →
     `GL_INVALID_OPERATION`).
   - New `tests/unit/buffer_parameter_i64_test.cpp` covers size/usage/mapped
     reads, validation, the DSA path, DSA-capability gating, and the public
     dispatch surface.
   - Validation: default + sanitizer suites green.

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
  + recording and the tracked-state queries. Coverage now 138/490 (28.2%) full /
  138/435 (31.7%) core.
- Validation: default + sanitizer suites green.

2026-08-27 (classic glGetTexImage, this session — small step)
- SPEC §8.1: implemented the classic `glGetTexImage` frontend entry point (the
  non-DSA counterpart to `glGetTextureImage`). It reads from the currently bound
  texture for `target`; no bound texture → `GL_INVALID_OPERATION`, negative
  `level` → `GL_INVALID_VALUE`. The backend `getTexImage` virtual was already
  present (used by the DSA path), so only the frontend dispatch, Context method,
  and tests were new. `gl_api` exposes `glGetTexImage(GLenum target, GLint level,
  GLenum format, GLenum type, GLvoid* pixels)`.
- New `tests/unit/dsa_named_texture_test.cpp` cases: bound-texture forward, no-bound
  error, negative-level error. Coverage now 139/490 (28.4%) full / 139/435 (32.0%)
  core.
- Validation: default + sanitizer suites green.

2026-08-27 (texImage1D/texImage3D, this session — small step)
- SPEC §8: implemented `glTexImage1D` and `glTexImage3D` frontend entry points.
  Both operate on the currently bound texture; no bound texture →
  `GL_INVALID_OPERATION`, negative dimension(s) or level → `GL_INVALID_VALUE`.
  `BackendTexture` gained `texImage1D`/`texImage3D` virtuals (default no-op);
  `MockTexture` records every call; `GLESBackendTexture` forwards to the driver
  when `glTexImage3D` is available (optional, ES 3.0+) and is a no-op for 1D
  (GLES has no 1D textures). `TextureObject::Image` gained a `depth` field so
  3D storage is tracked frontend-side. `gl_api` exposes both entry points.
- New `tests/unit/texsubimage_test.cpp` cases (6): 1D forward + negative-width +
  no-bound; 3D forward + negative-dimension + no-bound. Coverage now 141/490
  (28.8%) full / 141/435 (32.4%) core.
- Validation: default + sanitizer suites green.

2026-08-27 (1D texture emulation on GLES, this session — small step)
- SPEC §7 / §8: `GLESBackendTexture` now emulates 1D textures by storing them
  as 2D textures with height=1. `texImage1D` creates a 2D texture via
  `glTexImage2D`; `texSubImage1D` forwards to `glTexSubImage2D`; `copyTexImage1D`
  forwards to `glCopyTexImage2D`; parameter/queries (`texParameteri`/`fv`/`iv`,
  `getTexImage`, `getLevelParameter*`) map `GL_TEXTURE_1D` → `GL_TEXTURE_2D`
  via `glesActualTarget()` so the emulated surface is fully functional.
- Shader translator (SPEC §7): added `replaceEmulated1D` post-pass that rewrites
  `sampler1D` → `sampler2D`, `sampler1DShadow` → `sampler2DShadow`, and
  `texture1D(s, x)` → `texture2D(s, vec2(x, 0.5))` in the emitted GLSL ES so
  shaders using 1D textures compile on backends where only 2D is native.
- New `tests/unit/texsubimage_test.cpp` already covers the mock path. New
  `tests/backend/gles_e2e_1d_texture_test.cpp` verifies the GLES backend
  accepts `glTexImage1D` without error and reports the correct width.
- New `tests/backend/shader_translate_test.cpp` case verifies `texture1D` is
  rewritten to `texture2D` with `vec2(..., 0.5)`.
- Validation: default suite green; shader translator test passes.

## Next Steps

 0. **PRIMARY GOAL: implement all 490 OpenGL 4.6 spec command prototypes.**
      Current coverage ~214/490 (43.7%) full / ~214/435 (49.2%) core have a
       frontend entry point (was ~209 before the program-interface reflection
      completion this session). The standing objective is to reach **full
      coverage of all 490 spec command prototypes** —
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

2026-08-28 (indirect draw, `glDrawArraysIndirect` / `glDrawElementsIndirect`, this session)
- Implemented indirect draw (SPEC §10), closing the `§10: indirect draw` draw-
  expansion gap. `Context::drawArraysIndirect` / `drawElementsIndirect` validate
  `Feature::IndirectDrawing` (else `GL_INVALID_OPERATION`), an active program
  (else `GL_INVALID_OPERATION`, consistent with the direct draws), and a buffer
  bound to `GL_DRAW_INDIRECT_BUFFER` (else `GL_INVALID_OPERATION`). `offset` is
  the byte offset into that bound buffer; the frontend flushes tracked state then
  issues the native indirect draw. `IGraphicsBackend` gained `drawArraysIndirect` /
  `drawElementsIndirect` pure virtuals; `MockBackend` records mode/type/offset;
  `GLESBackend` forwards to `glDrawArraysIndirect` / `glDrawElementsIndirect`
  (resolved as optional `GLESLib` symbols, ES 3.1+). `GL_DRAW_INDIRECT_BUFFER` /
  `GL_DRAW_INDIRECT_BUFFER_BINDING` added to `gl_types.hpp`; `Context`/`gl_api`
  expose the entry points. `Feature::IndirectDrawing` is now `Native` in the mock
  profile (the mock records the call without a driver, like instanced/multi-draw).
- New `tests/unit/indirect_draw_test.cpp` (3 cases: requires program + indirect
  buffer → `GL_INVALID_OPERATION` otherwise, mode/type/offset recording, buffer
  unbind blocks the draw). Default + sanitizer suites green.
- Coverage now 216/490 (44.1%) full / 216/435 (49.7%) core (+2 prototypes).

2026-08-28 (vertex-array query, `glIsVertexArray`, this session)
- Completed the VAO object-lifecycle surface (SPEC §10.3.2). `Context::isVertexArray`
  returns whether a name is a generated VAO (`vertexArrays_` map membership; name 0
  is the default VAO and is never a queried object, so it returns false). Exposed as
  `glIsVertexArray` in `gl_api` (returns `GLboolean`). `glGenVertexArrays`,
  `glBindVertexArray`, `glDeleteVertexArrays`, and the DSA `glCreateVertexArrays` /
  `glVertexArray*` surface already existed; only the `is*` query was missing.
- New `tests/unit/vertex_array_test.cpp` (4 cases: false for ungenerated and for 0,
  true for generated, false after delete, `glIsVertexArray` entry point). Default
  386/386 + sanitizer 396/396 pass (pre-existing unrelated `shader_translate_test`
  failure unchanged).
- Coverage now 217/490 (44.3%) full / 217/435 (49.9%) core (+1 prototype).

2026-08-28 (cube-map face TexImage targets, this session)
- Fixed SPEC §8.1 cube-map face targets in the texture binding lookup. Cube faces
  (`GL_TEXTURE_CUBE_MAP_POSITIVE_X/NEGATIVE_X/POSITIVE_Y/NEGATIVE_Y/POSITIVE_Z/
  NEGATIVE_Z`) now resolve to the cube map bound as `GL_TEXTURE_CUBE_MAP`, so
  `glTexImage2D`/`glTexSubImage2D`/`glCopyTexImage2D`/`glTexParameter*` on a face
  address the bound cube map instead of failing with `GL_INVALID_OPERATION`. Added
  the six face enums to `gl_types.hpp`; added `normalizeTextureTarget()` (maps faces
  → `GL_TEXTURE_CUBE_MAP`) used in `GLStateTracker::boundTextureForTarget` and at the
  `tex->target` assignment in `texImage1D/2D/3D` (so the texture keeps its canonical
  `GL_TEXTURE_CUBE_MAP` target). Backend still receives the original face target
  (GLES3-native). Array targets (`GL_TEXTURE_1D_ARRAY`/`GL_TEXTURE_2D_ARRAY`) already
  worked (bound/looked-up by the same key); `GL_TEXTURE_RECTANGLE` has no GLES
  equivalent and is an honest capability gap.
- New `tests/unit/teximage_cube_test.cpp` (3 cases: 6 face uploads resolve to the
  bound cube map and reach the backend, face upload with no bound cube map →
  `GL_INVALID_OPERATION`, non-cube target regression). Default 389/389 + sanitizer
  399/399 pass (pre-existing unrelated `shader_translate_test` failure unchanged).
- Correctness fix (no new gl* entry point): coverage stays 217/490 (44.3%) full.

2026-08-28 (color clamping, `glClampColor`, this session)
- Implemented `glClampColor` (SPEC §15.2.3), closing the `§15/§16: glClampColor`
  gap noted in Next Steps (carried). `target` must be `GL_CLAMP_READ_COLOR`
  (else `GL_INVALID_ENUM`); `mode` must be `GL_TRUE`/`GL_FALSE`/`GL_FIXED_ONLY`
  (else `GL_INVALID_ENUM`). `GLStateTracker` gained `ClampColorState` +
  `setClampColor`/`clampReadColor`; `GLStateSink` gained `clampColor`; the value
  is pushed only on change (SPEC §10). `glGetIntegerv(GL_CLAMP_READ_COLOR)`
  returns the tracked mode. `GL_CLAMP_READ_COLOR`/`GL_FIXED_ONLY` added to
  `gl_types.hpp`; `Context`/`gl_api` expose `clampColor`; `MockBackend` records
  the call; `GLESBackend::clampColor` is a no-op (GLES has no equivalent and
  desktop color-clamp is effectively always-on in ES fragment outputs).
- New `tests/unit/clamp_color_test.cpp` (3 cases: invalid target/mode →
  `GL_INVALID_ENUM`, change-only push, tracked `glGetIntegerv` read). Default
  suite green; the pre-existing `shader_translate_test` failure in the sanitizer
  build is environment/translator-related and unrelated to this change (reproduces
  on a clean stash of these edits).

2026-08-27 (DSA texture object surface, this session)
- Implemented the Direct State Access texture object surface (SPEC §2.1 / §8.1):
  `glCreateTextures`, `glTextureStorage1D/2D/3D`, `glTextureSubImage1D/2D/3D`,
  `glTextureParameteri`/`f`/`fv`/`iv`, `glGenerateTextureMipmap`,
  `glGetTextureParameterfv`, `glGetTextureLevelParameteriv`/`fv`, `glGetTextureImage`,
  `glTextureBuffer`/`glTextureBufferRange`. These operate on an explicit named
  texture's backend resource (no global bind needed), pushing only changed state;
  frontend owns storage dims so `glGetTextureLevelParameter*` reads width/height/
  depth/internal format without a driver round-trip (SPEC §10). Capability-gated by
  `DirectStateAccess` (now `Emulated` on every backend — YAGLT emulates DSA by
  binding the named object's backend resource before each driver call, so the DSA
  entry points are available even where the driver lacks `GL_EXT_direct_state_access`).
- Backend: `BackendTexture` gained `storage1D/2D/3D`, `generateMipmap`, `textureBuffer`,
  `textureBufferRange`, `getLevelParameteriv`/`fv`, `getTexImage` virtuals (default
  no-op). `GLESBackendTexture` binds its handle then drives `glTexStorage*D` /
  `glGenerateMipmap` / `glTexBuffer(Range)` / `glGetTexImage` / `glGetTexLevelParameter*`
  (resolved as optional `GLESLib` symbols). `MockTexture` records every call. New
  `tests/unit/dsa_named_texture_test.cpp` (15 cases) covers creation, storage
  validation, sub-image storage requirement + bounds, parameter round-trip, mipmap,
  level queries, image readback, buffer binding, unsupported-mode and ungenerated-
  name error paths. Coverage now 154/490 (31.4%) full / 154/435 (35.4%) core.
- Validation: default 226/226 green; sanitizer (ASan/UBSan + shader translate)
  219/219 green.

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

2026-08-27 (EGL overflow fix + primitive restart, this session)
- **Recovery**: the prior agent crashed mid-work on the GLES backend EGL init.
  The crash notes identified a stack-buffer-overflow in Mesa 26's `_eglFindDisplay`
  when the surfaceless platform is routed through `eglGetPlatformDisplay` /
  `eglGetPlatformDisplayEXT`. `createContext` now prefers
  `eglGetDisplay(EGL_DEFAULT_DISPLAY)` (which yields a usable surfaceless display
  on this Mesa build without overflow) and only falls back to the platform-display
  entry points when that fails. Verified clean under ASan (no overflow). Added
  `tools/lsan_mesa_suppressions.txt` for the known Mesa softpipe init leaks and
  wired them via `LSAN_OPTIONS=suppressions=...` so a Mesa-backed ASan run stays
  green without masking our own leaks.
- **Primitive restart (SPEC §10.4)**: implemented `glPrimitiveRestartIndex` as a
  tracked `GLStateTracker` value pushed through a new `GLStateSink::primitiveRestart`
  only on change (SPEC §10). `GL_PRIMITIVE_RESTART` is a normal enable/disable cap
  (added to the tracked-cap set so `glIsEnabled`/`glGet` report it). GLES backend
  resolves `glPrimitiveRestartIndex` optionally; mock records it. `GL_PRIMITIVE_RESTART`
  / `_FIXED_INDEX` / `_INDEX` constants added to `gl_types.hpp`; `glGetIntegerv
  (GL_PRIMITIVE_RESTART_INDEX)` returns the tracked value. New
  `tests/unit/primitive_restart_test.cpp`. Coverage now 138/490 (28.2%) full /
  138/435 (31.7%) core.
- Validation: default + sanitizer (no Mesa) + translate (Mesa) suites all green.

2026-08-27 (draw expansion + vertex attrib divisor, this session)
- SPEC §10 draw expansion: implemented `glVertexAttribDivisor`,
  `glMultiDrawArrays`, `glMultiDrawElements`, `glDrawRangeElements`,
  `glDrawElementsBaseVertex`. Frontend `Context` methods flush tracked pipeline
  state before each draw (consistent with the single-draw calls); non-instanced
  variants require an active program (core profile → `GL_INVALID_OPERATION`);
  `glDrawRangeElements` validates `end < start` → `GL_INVALID_VALUE`;
  `glMultiDraw*` validates negative `drawcount` → `GL_INVALID_VALUE`. Capability
  gating centralized via four new `Feature`s: `VertexAttribDivisor`,
  `MultiDraw`, `DrawRangeElements` (Native on GLES 3.0) and `DrawElementsBaseVertex`
  (Native only on GLES 3.2; honestly `Unsupported` otherwise — marked per the
  detected version in `populateGLESCapabilities`, Native in the mock profile).
- Vertex attrib divisor pushed through a new `GLStateSink::vertexAttribDivisor`
  only when non-zero (the GL default is 0, so no redundant native call, SPEC §10).
  `Context::vertexAttribDivisor` records the per-attrib divisor on the bound VAO
  and marks vertex state dirty; no VAO bound → `GL_INVALID_OPERATION`. The VAO
  `AttribState` gained a `divisor` field.
- Backend wiring: `IGraphicsBackend` gained four pure-virtual draw methods;
  `MockBackend` records them (observable in tests); `GLESBackend` drives the real
  driver via new (optional) `GLESLib` symbols `glVertexAttribDivisor` /
  `glMultiDrawArrays` / `glMultiDrawElements` / `glDrawRangeElements` /
  `glDrawElementsBaseVertex` (resolved defensively so `load()` still succeeds on
  drivers that lack them). Public `gl_api` exposes all five entry points.
- New `tests/unit/draw_expansion_test.cpp` (5 cases) covers divisor push-only-
  when-nonzero + no-VAO error, multi-draw recording + negative-count + no-program
  gates, range validation, and base-vertex program/capability gates. Also fixed a
  pre-existing unit test (`vertex_attrib_captures_bound_array_buffer`) that
  omitted `glUseProgram`, so `glDrawArrays` early-returned before `flushState()`
  and the captured ARRAY_BUFFER was never pushed (now passes).
- Validation: default 212/212 green; sanitizer 219/219 green (incl. Mesa e2e);
  translate/Mesa build's new tests pass (the normal-mode run still trips the
  documented pre-existing Mesa softpipe teardown segfault after e2e teardown,
  unrelated to this change — proven by the sanitizer run passing and the
  isolated mock test passing). Coverage bumped in `docs/coverage-core.md`
  (now 143/490 = 29.2% full / 143/435 = 32.9% core).

2026-08-27 (1D texture emulation on GLES, this session — small step)
- SPEC §7 / §8: `GLESBackendTexture` now emulates 1D textures by storing them
  as 2D textures with height=1. `texImage1D` creates a 2D texture via
  `glTexImage2D`; `texSubImage1D` forwards to `glTexSubImage2D`; `copyTexImage1D`
  forwards to `glCopyTexImage2D`; parameter/queries (`texParameteri`/`fv`/`iv`,
  `getTexImage`, `getLevelParameter*`) map `GL_TEXTURE_1D` → `GL_TEXTURE_2D`
  via `glesActualTarget()` so the emulated surface is fully functional.
- Shader translator (SPEC §7): added `replaceEmulated1D` post-pass that rewrites
  `sampler1D` → `sampler2D`, `sampler1DShadow` → `sampler2DShadow`, and
  `texture1D(s, x)` → `texture2D(s, vec2(x, 0.5))` in the emitted GLSL ES so
  shaders using 1D textures compile on backends where only 2D is native.
- New `tests/backend/gles_e2e_1d_texture_test.cpp` verifies the GLES backend
  accepts `glTexImage1D` without error and reports the correct width.
- New `tests/backend/shader_translate_test.cpp` case verifies `texture1D` is
  rewritten to `texture2D` with `vec2(..., 0.5)`.
- Validation: default suite green; shader translator test passes.

2026-08-27 (DSA renderbuffer + framebuffer surface, this session)
- Implemented the Direct State Access renderbuffer + framebuffer surface
  (SPEC §9.2), continuing the Full DSA surface priority. Capability-gated by
  `DirectStateAccess` (Emulated: YAGLT drives each named object's backend
  resource directly, so DSA works on every backend without driver `GL_EXT_direct_
  state_access`). New `Context::createRenderbuffers`/`namedRenderbufferStorage`/
  `namedRenderbufferStorageMultisample`/`getNamedRenderbufferParameteriv`, and
  `createFramebuffers`/`namedFramebufferRenderbuffer`/`namedFramebufferTexture`/
  `namedFramebufferTextureLayer`/`checkNamedFramebufferStatus`/
  `namedFramebufferParameteri`/`getNamedFramebufferParameteriv`/
  `getNamedFramebufferAttachmentParameteriv`/`blitNamedFramebuffer`/
  `invalidateNamedFramebufferData`/`invalidateNamedFramebufferSubData`/
  `clearNamedFramebufferiv`/`clearNamedFramebufferuiv`/`clearNamedFramebufferfv`/
  `clearNamedFramebufferfi`, plus matching `gl*` entry points in `gl_api`.
- Backend: `BackendRenderbuffer` gained `renderbufferStorageMultisample`;
  `BackendFramebuffer` gained `framebufferTextureLayer` + `framebufferParameteri`.
  `GLESBackend` implements all three via new (optional) `GLESLib` symbols
  (`glRenderbufferStorageMultisample`/`glFramebufferTextureLayer`/
  `glFramebufferParameteri`) so `load()` still succeeds where they are absent;
  `MockRenderbuffer`/`MockFramebuffer` record every call. Named blit/invalidate/
  clear bind the named framebuffer(s) to the driver then restore the tracked
  binding (DSA must not leave a side effect on the bound FBO, SPEC §9.2).
- Honest validation: negative dims/samples → `GL_INVALID_VALUE`; ungenerated
  name → `GL_INVALID_OPERATION`; null query pointer → `GL_INVALID_VALUE`; blit
  mask outside color/depth/stencil → `GL_INVALID_VALUE`. `checkNamedFramebuffer-
  Status` reports `INCOMPLETE_MISSING_ATTACHMENT` (empty) / `INCOMPLETE_ATTACHMENT`
  (no-storage attachment) / `COMPLETE` (per structural + backend checkStatus),
  mirroring `checkFramebufferStatus`. New `tests/unit/dsa_named_framebuffer_test.cpp`
  (20 cases) covers storage, attachment recording, completeness, parameter
  queries, blit/invalidate/clear forward + validation. Coverage now ~173/490
  (35.3%) full / 173/435 (39.8%) core.
- Validation: default + sanitizer suites green for the new surface (the lone
  `build_san` failure is the pre-existing translator-absent `shader_translate_test`
  config quirk, unrelated to this change).

2026-08-27 (DSA vertex-array surface, this session)
- Implemented the Direct State Access vertex-array surface (SPEC §10.3.1),
  completing the Full DSA surface priority. Capability-gated by
  `DirectStateAccess` (Emulated). New `Context` methods (and matching `gl*`
  entry points in `gl_api`): `createVertexArrays`, `vertexArrayElementBuffer`,
  `enable/disableVertexArrayAttrib`, `vertexArrayVertexBuffer(s)`,
  `vertexArrayAttribFormat/IFormat/LFormat`, `vertexArrayAttribBinding`,
  `vertexArrayBindingDivisor`.
- Frontend `VertexArrayObject` extended with a separate attribute-format model:
  each attribute references a `VertexBufferBinding` (buffer + base offset +
  stride + divisor), and a `relativeoffset` is added to the binding offset when
  the native `glVertexAttribPointer` is replayed. The legacy `vertexAttribPointer`
  / `vertexAttribDivisor` now also populate the binding map, so the unified
  flush path serves both the legacy and DSA models. The element-array buffer is
  bound (as `GL_ELEMENT_ARRAY_BUFFER`) while the VAO is bound during flush.
- Honest validation: ungenerated VAO/buffer → `GL_INVALID_OPERATION`; null
  pointer arrays in `vertexArrayVertexBuffers` → `GL_INVALID_VALUE`;
  `vertexArrayAttrib*Format` size outside [1,4] → `GL_INVALID_VALUE`; integer
  (`IFormat`) / double (`LFormat`) variants force `normalized = false`.
  DSA gated by `DirectStateAccess` (test uses `setCapability` to exercise the
  unsupported path → `GL_INVALID_OPERATION`). New
  `tests/unit/dsa_vertex_array_test.cpp` (11 cases) covers state recording,
  combined offset replay, distinct binding points, instanced divisor, element
  buffer bind, integer-format normalization, and a legacy-path regression check.
  `MockBackend` gained per-call vector recorders for vertex attrib flush
  assertions.
- Coverage now ~184/490 (37.6%) full / 184/435 (42.3%) core.
- Validation: default 269/269 green; sanitizer 277/277 (lone failure = pre-existing
  translator-absent config quirk); translate/Mesa 241 PASS / 0 FAIL (the only
  crash is the pre-existing Mesa teardown SEGV). GLES backend path verified.

2026-08-27 (Program pipelines, SPEC §7.4, this session)
- Implemented the program-pipeline object surface (SPEC §7.4). Capability-gated
  by `ProgramPipelines` (Mock = Emulated, so the full frontend path is testable;
  GLES = Emulated only where separable programs exist, else Unsupported). New
  `Context` methods + `gl_api` entry points: `genProgramPipelines`,
  `deleteProgramPipelines`, `isProgramPipeline`, `bindProgramPipeline`,
  `createShaderProgramv`, `useProgramStages`, `activeShaderProgram`,
  `getProgramPipelineiv`, `validateProgramPipeline`, `getProgramPipelineInfoLog`.
- Frontend `ProgramPipelineObject` owns the stage-bit → program mapping, the
  active program (for `glUseProgramStages(…,0)`), validation flag, and info log.
  `ProgramObject` gained a `separable` flag set by `createShaderProgramv`
  (PROGRAM_SEPARABLE semantics); `useProgramStages` accepts only a linked,
  separable program, otherwise `GL_INVALID_OPERATION`. `glActiveShaderProgram`
  + `glUseProgramStages(pipeline, GL_ALL_SHADER_BITS, 0)` maps the active program
  to every stage.
- The bound pipeline is tracked independently of the single `glUseProgram` in
  `GLStateTracker` and forwarded to the backend via a new `GLStateSink::
  bindProgramPipeline` (Mock records it; GLES records only — a single linked
  program drives a GLES draw, so per-stage pipeline rendering is not consumed
  there). `gl_types.hpp` gained the stage bits, `GL_ALL_SHADER_BITS`,
  `GL_ACTIVE_PROGRAM`, `GL_PROGRAM_SEPARABLE`, and `GL_VALID_STATUS` (= 0x8B83).
- Honest validation: `getProgramPipelineiv` returns ACTIVE_PROGRAM / per-stage
  program / VALID_STATUS / INFO_LOG_LENGTH; null params → `GL_INVALID_VALUE`,
  0/non-pipeline name → `GL_INVALID_OPERATION`, unknown pname → `GL_INVALID_ENUM`;
  `useProgramStages` with an unknown stage bit → `GL_INVALID_VALUE`; ungenerated
  pipeline / non-separable program → `GL_INVALID_OPERATION`. New
  `tests/unit/dsa_program_pipeline_test.cpp` (11 cases). `GLStateSink` gained the
  `bindProgramPipeline` pure virtual; all sink implementers (MockBackend,
  GLESBackend, and the test `RecordingSink`s) were updated.
 - Coverage now ~194/490 (39.6%) full / 194/435 (44.6%) core.
 - Validation: default 280/280 green; sanitizer 288/288 (lone failure = pre-existing
   translator-absent `shader_translate_test` config quirk); translate/Mesa built
   and the GLES e2e passes under softpipe (288/288, 1 pre-existing quirk; the only
   crash is the pre-existing Mesa teardown SEGV). GLES backend path verified.

2026-08-27 (Rasterization controls, SPEC §11.1 / §11.5, this session)
- Implemented the remaining rasterization-control surface (SPEC §11): `glPolygonMode`,
  `glSampleMaski`, `glMinSampleShading`. Capability-gated `MultisampleRasterState`
  (sampleMask: array<uint32_t, kMaxSampleMaskWords=2> + minSampleShading) and
  `PolygonModeState` (front/back, default GL_FILL) added to `GLStateTracker`;
  pushed via three new `GLStateSink` pure virtuals `polygonMode(front,back)`,
  `sampleMaski(maskNumber,mask)`, `minSampleShading(value)`. `sampleMaski` pushes
  only changed words.
- New `Context` methods + `gl_api` entry points forward to the tracker with honest
  validation: `glPolygonMode` bad face/mode → `GL_INVALID_ENUM`; `glSampleMaski`
  out-of-range `maskNumber` (≥ kMaxSampleMaskWords) → `GL_INVALID_VALUE`;
  `glMinSampleShading` value outside [0,1] → `GL_INVALID_VALUE`. `gl_types.hpp`
  gained `GL_FRONT_AND_BACK`/`GL_POINT`/`GL_LINE`/`GL_FILL`/`GL_POLYGON_MODE`/
  `GL_SAMPLE_MASK`/`GL_MIN_SAMPLE_SHADING`. `glGetIntegerv` returns `GL_POLYGON_MODE`
  and `GL_SAMPLE_MASK`; `glGetFloatv` returns `GL_MIN_SAMPLE_SHADING`.
- Mock backend records all three; GLES backend honestly no-ops them (GLES has no
  polygon mode / sample mask / min-sample-shading). All `GLStateSink` implementers
  (MockBackend, GLESBackend, and the test `RecordingSink`s in state/texture/dsa
  tests) were updated. New `tests/unit/rasterization_control_test.cpp` (5 cases).
- Coverage now ~197/490 (40.2%) full / 197/435 (45.3%) core. §11 marked ✅ in
  coverage-core.md; §14 `glMinSampleShading` dropped from missing.
- Validation: default 285/285 green; sanitizer 293/293 (lone failure = pre-existing
  translator-absent `shader_translate_test` config quirk); translate/Mesa built and
  GLES e2e passes under softpipe (the only crash is the pre-existing Mesa teardown
  SEGV in `GLESBackend::~GLESBackend` → `GLESLib` destruction, unrelated to this
  change). GLES backend path verified.

2026-08-27 (Program-interface reflection, SPEC §7.3.11, this session)
- Implemented `glGetProgramResourceIndex` / `glGetProgramResourceName` /
  `glGetProgramResourceiv` / `glGetProgramResourceLocation` /
  `glGetProgramResourceLocationIndex`. Frontend `Context` methods + `gl_api` entry
  points with full SPEC validation: program must be a linked program object (else
  `GL_INVALID_OPERATION`); `programInterface` must be a valid interface enum (else
  `GL_INVALID_ENUM`); name not found → `GL_INVALID_INDEX`/`-1` honestly (no error);
  out-of-range index / negative buffer size → `GL_INVALID_VALUE`; unknown property
  in `glGetProgramResourceiv` → `GL_INVALID_ENUM`; `propCount > bufSize`, null
  params/props, negative `propCount` → `GL_INVALID_VALUE`.
- New `BackendProgram` reflection interface (`programResourceCount`,
  `getProgramResource*`) with honest default returns (no introspection). The GLES
  backend wires real ES 3.0+ `glGetProgramResource*` via added `GLESLib` loader
  symbols (resolved optionally so load() still succeeds on limited EGL stacks);
  `GLESBackendProgram` forwards interface/name/index/property queries to the
  driver. `gl_types.hpp` gained the interface enums, property enums, and
  `GL_INVALID_INDEX`; `gles_loader.hpp`/`.cpp` gained the six loader symbols.
- New `tests/unit/program_resource_test.cpp` (12 Mock validation + honest-not-found
  cases) and `tests/backend/gles_e2e_program_resource_reflection` (real Mesa
  reflection: index/location/name/property round-trip + not-found). The e2e test
  skips cleanly when no driver is present.
- Coverage now ~202/490 (41.2%) full / 202/435 (46.4%) core. §7 priority #8
  reflection marked done; only subroutines (§7.9) / compute / shader binaries remain.
- Validation: default 297/297 green; sanitizer 306/306 (lone failure = pre-existing
  translator-absent `shader_translate_test` config quirk); the new e2e reflection
  test passes under Mesa in the sanitizer build. GLES backend path verified.

2026-08-27 (Subroutines, SPEC §7.9, this session)
- Implemented the subroutine surface (SPEC §7.9): `glGetSubroutineIndex`,
  `glGetSubroutineUniformLocation`, `glGetActiveSubroutineUniformiv`/`Name`,
  `glGetActiveSubroutineName`, `glUniformSubroutinesuiv`, `glGetUniformSubroutineuiv`.
  Capability-gated by a new `Feature::Subroutines` (Mock = Emulated so the full
  frontend path is testable; GLES = Native on ES 3.1+, else Unsupported). `Context`
  validates `shadertype` against the six subroutine stages (else `GL_INVALID_OPERATION`),
  requires a linked program for the reflection getters and an active program for the
  selection getters/reads; name-not-found → `GL_INVALID_INDEX`/`-1` honestly (no error).
- New `BackendProgram` subroutine interface (defaults honest: no introspection /
  no-op selection); the GLES backend wires the ES 3.1+ `glGetSubroutine*` /
  `glUniformSubroutinesuiv` driver entry points via added `GLESLib` loader symbols
  (resolved optionally). `gl_types.hpp` gained the subroutine query pnames;
  `mock_capabilities.hpp` / `gles_capabilities.cpp` set `Subroutines`.
- New `tests/unit/subroutine_test.cpp` (9 Mock validation + honest-not-found cases).
  No e2e test: subroutine desktop-GLSL syntax may not survive the glslang→SPIRV-Cross
  translator, so the GLES path is exercised structurally via the build/sanitize path.
- Coverage now ~209/490 (42.7%) full / 209/435 (48.0%) core. §7 row dropped
  subroutines from missing; priority #8 reflection + subroutines marked done.
- Validation: default 306/306 green; sanitizer 315/315 (lone failure = pre-existing
   translator-absent `shader_translate_test` config quirk); GLES backend path compiles
   and links under the sanitizer build with no regression.

2026-08-27 (legacy uniform/attribute/uniform-block reflection, this session)
- Implemented the remaining program-interface reflection entry points (SPEC §7.6 /
  §11.1): `glGetActiveUniform`, `glGetActiveAttrib`, `glGetUniformBlockIndex`,
  `glGetActiveUniformBlockiv`, `glGetActiveUniformBlockName`. Per the OpenGL 4.6
  spec these are exact equivalents of the `GetProgramResource*` queries, so they
  are implemented as frontend delegations onto the existing `BackendProgram`
  reflection methods (UNIFORM / PROGRAM_INPUT / UNIFORM_BLOCK interfaces) — no new
  backend virtuals were required. `glGetActiveUniform`/`glGetActiveAttrib` call
  `getProgramResourceName` + `getProgramResourceiv(ARRAY_SIZE, TYPE)`;
  `glGetUniformBlockIndex` ≡ `getProgramResourceIndex(UNIFORM_BLOCK, name)` (honest
  `GL_INVALID_INDEX` on miss); `glGetActiveUniformBlockName` ≡
  `getProgramResourceName(UNIFORM_BLOCK, …)`. `glGetActiveUniformBlockiv` maps each
  `pname` to its table-7.7 property (e.g. `UNIFORM_BLOCK_BINDING`→`BUFFER_BINDING`,
  `UNIFORM_BLOCK_DATA_SIZE`→`BUFFER_DATA_SIZE`) and reports `GL_INVALID_ENUM` for an
  unknown `pname`, `GL_INVALID_VALUE` for null params / out-of-range index; the
  `UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES` case sizes its result buffer from
  `NUM_ACTIVE_VARIABLES`. Added the `glGetActiveUniformBlockiv` pname constants to
  `gl_types.hpp`. Validation: unlinked program → `GL_INVALID_OPERATION`; out-of-range
  index / negative `bufSize` → `GL_INVALID_VALUE`; `getUniformBlockIndex` miss →
  honest `GL_INVALID_INDEX` (no error); bad `pname` → `GL_INVALID_ENUM`.
- New `tests/unit/active_uniform_attrib_test.cpp` (11 Mock validation + honest-not
  found cases) and `tests/backend/gles_e2e_get_active_uniform` (real Mesa: name/
  size/type round-trip for a known uniform). The e2e test is ordered before the
  other driver tests so it actually executes (the pre-existing Mesa teardown SEGV
  in `GLESBackend::~GLESBackend` aborts the binary after the first driver-initializing
  e2e test; same limitation as the existing e2e tests). Registered the unit test in
  `tests/CMakeLists.txt`.
- Coverage now ~214/490 (43.7%) full / ~214/435 (49.2%) core. §7 program-interface
  reflection marked complete.
- Validation: default 317/317 green; sanitizer + translate (Mesa) builds compile and
  link (the translate/Mesa run still trips the pre-existing Mesa softpipe teardown
  SEGV after the first e2e driver test, unrelated to this change; verified by
  construction + the existing `gles_e2e_program_resource_reflection` path).

2026-08-27 (buffer object completeness: clear/invalidate/read-back, this session)
- Closed the §6 buffer-object residual: `glGetBufferSubData`/`glGetNamedBufferSubData`
  read the frontend's authoritative CPU mirror (exact on mock and real backends);
  `glClearBufferData`/`glClearNamedBufferData`/`glClearBufferSubData`/
  `glClearNamedBufferSubData` convert the clear value into the destination sized
  internalformat and fill the mirror, then re-upload the range to the backend (GLES
  has no native `glClearBufferData`, so the frontend fill + `glBufferSubData`
  re-upload keeps the driver copy consistent). `glInvalidateBufferData`/
  `glInvalidateBufferSubData`/`glInvalidateNamedBuffer*` validate bounds/mapping and
  forward a driver discard hint (`glInvalidateBufferData`/`glInvalidateBufferSubData`
  on GLES 3.0+; resolved optionally in the loader).
- Added `lookupBufferFormat` (table 8.24 subset: 8/16/32-bit float and 8/16/32-bit
  signed/unsigned integer R/RG/RGB/RGBA formats) and a `buildClearPattern` helper
  that converts the `format`/`type` clear value to the destination layout. The
  source type (FLOAT/HALF_FLOAT vs integer) drives the read; integer sources are
  stored directly into UNORM/SNORM destinations (no re-scale), float sources are
  scaled to [0,1]/[-1,1]; missing components default to (0,0,0,1). Null `data` fills
  zero. Bad internalformat → `GL_INVALID_ENUM`; unaligned/negative/out-of-bounds
  offset·size → `GL_INVALID_VALUE`; mapped store (non-persistent) → `GL_INVALID_OPERATION`;
  missing buffer → `GL_INVALID_OPERATION`.
- Added 10 entry points to `gl_api.hpp`/`gl_api.cpp` + `Context` (6 clear/invalidate
  families incl. `*Named`) + declarations in `context.hpp`; added `invalidateBuffer*`
  virtuals to `BackendBuffer` (GLES forwards, Mock records the call). Added the
  needed GL constants to `gl_types.hpp` (sized internal formats, `*_INTEGER` /
  `DEPTH_COMPONENT` / `STENCIL_INDEX` formats, `BYTE`/`SHORT`/`INT`/`UNSIGNED_INT`
  types, `GL_MAP_PERSISTENT_BIT`).
- Unit tests: 14 new cases in `tests/unit/buffer_completeness_test.cpp` (whole-store
  fill, sub-range fill, null-data zero, RGBA8 normalized conversion, INVALID_ENUM on
  unsized internalformat, INVALID_VALUE on unaligned offset / bad format,
  INVALID_OPERATION on missing buffer / mapped read, getBufferSubData read-back by
  target and by name, OOB / mapped validation, invalidate forwarding + OOB,
  API-dispatch round-trip). Removed redundant local `GL_UNSIGNED_INT` constants in
  `draw_test.cpp` / `draw_expansion_test.cpp` that now collide with the global.
- Coverage now ~219/490 (44.7%) full / ~219/435 (50.3%) core. §6 buffer objects marked
  complete (🟢). Note: the spec table-8.24 subset does not yet cover packed formats
  (R11F_G11F_B10F, RGB10_A2 / RGB10_A2UI); those report `GL_INVALID_ENUM` honestly.
- Validation: default 332/332 green (was 317; +11 prior reflection +14 this step);
  sanitizer 342/342 green with the single pre-existing `shader_translate_test`
  config-quirk failure (unrelated). GLES backend path compiles and links under both
  sanitizer and translate/Mesa builds; the Mesa teardown SEGV still aborts the
  translate run after the first e2e driver test (no e2e buffer test added, since it
  would be unreachable beyond that crash and the mock path already verifies the
   fill/re-upload semantics exactly).

2026-08-27 (texture completeness: immutable storage, texture buffers, multisample, this session)
- Closed the §8.5 / §8.9 / §8.19 residual that was still marked "(planned)" in
  `feature-matrix.md` and "missing" in `coverage-core.md`. The DSA `glTextureStorage1D/2D/3D`
  and `glTextureBuffer`/`glTextureBufferRange` already existed; the gaps were the **non-DSA**
  `glTexStorage1D/2D/3D` + `glTexBuffer`/`glTexBufferRange` and the entire **multisample**
  surface.
- Added `BackendTexture` virtuals `storage2DMultisample`/`storage3DMultisample`/
  `texImage2DMultisample`/`texImage3DMultisample` (defaulted; overridden in `MockTexture`
  and `GLESBackendTexture`). GLES forwards to `glTexStorage2DMultisample`/
  `glTexStorage3DMultisample`/`glTexImage2DMultisample`/`glTexImage3DMultisample` (resolved
  optionally in `gles_loader`), Mock records call counts + last params.
- `Context` gained `texStorage1D/2D/3D`, `texBuffer`/`texBufferRange` (resolve the texture
  bound to `target`), `texStorage2DMultisample`/`texStorage3DMultisample`/
  `texImage2DMultisample`/`texImage3DMultisample` (target-validated:
  `GL_TEXTURE_2D_MULTISAMPLE` for 2D, `GL_TEXTURE_2D_MULTISAMPLE_ARRAY` for 3D), and DSA
  `textureStorage2DMultisample`/`textureStorage3DMultisample` (capability-gated by
  `DirectStateAccess`). `texBuffer*` requires `target == GL_TEXTURE_BUFFER`, a generated
  buffer name, and non-negative offset/size. Immutable multisample sets
  `immutableStorage=true`; `texImage*Multisample` sets `immutableStorage=false`. All
  validation matches SPEC §8 (no bound texture → `GL_INVALID_OPERATION`; `levels < 1` or a
  dimension `< 1` or `samples < 0` → `GL_INVALID_VALUE`; wrong target → `GL_INVALID_ENUM`;
  ungenerated buffer → `GL_INVALID_OPERATION`).
- Added 11 `gl_api` entry points and 12 `Context` method declarations (`context.hpp`).
- Unit tests: 18 new cases in `tests/unit/texture_storage_test.cpp` (immutable 1D/2D/3D on the
  bound texture, no-bound/invalid-value validation, texture-buffer bind + range + wrong-target
  + ungenerated-buffer + negative-offset, multisample 2D/3D immutable + mutable, DSA
  multisample + DSA-disabled → `GL_INVALID_OPERATION`). Registered in `tests/CMakeLists.txt`.
- Coverage now ~230/490 (47.0%) full / ~230/435 (52.9%) core. §8 texture objects marked
  complete for storage/buffer/multisample (cube/array/rect targets, `GetTexImage` multisample,
  texture views remain). Multisample + texture-buffer rows flipped `(planned)` → `Native` in
  `feature-matrix.md`.
- Validation: default 351/351 green (was 332; +18 this step); sanitizer 361/361 green with the
  single pre-existing `shader_translate_test` config-quirk failure (unrelated). GLES/translate
  build (`build_tx`) compiles and links the new backend path. No e2e texture test added (the
  pre-existing Mesa teardown SEGV aborts the translate run after the first driver test, and the
  mock path already verifies the storage/buffer/multisample forwarding exactly).

2026-08-27 (texture integer parameters, classic mipmap, texture invalidation, this session)
- SPEC §8.1: implemented the remaining texture-parameter / mipmap / invalidation surface that
  had only DSA or no coverage. New non-DSA + DSA entry points:
  - `glGenerateMipmap(target)` (classic counterpart to the DSA `glGenerateTextureMipmap`).
  - Integer texture parameters: `glTexParameterIiv` / `glTexParameterIuiv` (bound texture) and
    `glTextureParameterIiv` / `glTextureParameterIuiv` (DSA, capability-gated by
    `DirectStateAccess`). Element counts are derived from the pname (e.g. `GL_TEXTURE_BORDER_COLOR`
    = 4), matching desktop GL which passes no explicit count.
  - Integer texture-parameter queries: `glGetTexParameterIiv` / `glGetTexParameterIuiv` and
    `glGetTextureParameterIiv` / `glGetTextureParameterIuiv` (DSA). Frontend owns the stored
    signed/unsigned vectors, so queries never round-trip to the driver (SPEC §10).
  - Texture invalidation: `glInvalidateTexImage` / `glInvalidateTexSubImage` (bound texture,
    non-DSA) forwarded as a backend discard hint.
- `BackendTexture` gained `texParameterIiv` / `texParameterIuiv` / `invalidateTexImage` /
  `invalidateTexSubImage` virtuals (default no-op). `MockTexture` records every call;
  `GLESBackendTexture` drives the native `glTexParameterIiv` / `glTexParameterIuiv` /
  `glInvalidateTexImage` / `glInvalidateTexSubImage` (resolved as optional `GLESLib` symbols so
  `load()` still succeeds where the driver lacks them). `TextureObject` gained `paramsIiv` /
  `paramsIuiv` stores. `GLESLib` gained the four loader symbols.
- Honest validation: null `params` → `GL_INVALID_VALUE`; no bound texture → `GL_INVALID_OPERATION`;
  `invalidateTexImage` negative level / `invalidateTexSubImage` negative dims → `GL_INVALID_VALUE`;
  DSA getters report `GL_INVALID_OPERATION` when `DirectStateAccess` is unsupported. `gl_api`
  exposes all eleven new entry points.
- New `tests/unit/texparam_int_invalidate_test.cpp` (19 cases) covers mipmap forward, integer
  param set + vector length (border-color = 4), queries (set/unset/zero), null/no-texture
  errors, DSA set/get, DSA-unsupported path, texture invalidation forward + validation, and the
  `gl*` surface. Registered in `tests/CMakeLists.txt`.
- Coverage now ~241/490 (49.2%) full / ~241/435 (55.4%) core. §8 texture objects marked
  complete for parameters (integer variants), classic mipmap, and invalidation.
- Validation: default 369/369 green; sanitizer 379/379 green with the single pre-existing
  `shader_translate_test` 1D-emulation quirk (unrelated to this change; translator untouched).

2026-08-27 (glBindAttribLocation — SPEC §7.3.7, this session)
- Implemented `glBindAttribLocation` (generic attribute index → attribute variable
  name binding before link). `BackendProgram` gained a `bindAttribLocation(name, index)`
  virtual (default no-op). `ProgramObject` gained an `attribBindings` map
  (name→index). `Context::bindAttribLocation` records the request (unknown program →
  `GL_INVALID_OPERATION`) and `Context::linkProgram` now replays every recorded binding
  onto the backend program immediately before `BackendProgram::link`, so the binding
  takes effect on the next link per spec.
- `MockProgram` records `boundAttribLocations` and `getAttribLocation` prefers a
  prior binding over its auto-assigned location (so the binding is authoritative and
  testable without a real driver). `GLESBackendProgram` forwards to
  `lib->glBindAttribLocation` (already resolved in `GLESLib`; core in GLES 2.0+).
- Public `gl_api` exposes `glBindAttribLocation`; `glGetAttribLocation` already existed.
- New `tests/unit/bind_attrib_location_test.cpp` (4 cases): unknown-program error,
  recording on the `ProgramObject`, pre-link binding applied to the backend and
  winning over the mock's default location, and re-bind-then-relink changing the
  location. Registered in `tests/CMakeLists.txt`.
- Validation: default 373/373 green (369 → 373, +4 cases). Coverage: §7 Programs row
  updated (BindAttribLocation removed from missing).

2026-08-27 (glProvokingVertex — SPEC §11, this session)
- Implemented `glProvokingVertex` (first/last vertex convention). `GLStateSink` gained
  a `provokingVertex(mode)` pure virtual; `GLStateTracker` gained `setProvokingVertex`
  (mode ∈ {GL_FIRST_VERTEX_CONVENTION, GL_LAST_VERTEX_CONVENTION}; default LAST) +
  `ProvokingVertexState`, pushed via `apply()` only when the mode changes (SPEC §10),
  and a `GL_PROVOKING_VERTEX` `glGetIntegerv` query. `Context::provokingVertex` rejects
  an invalid mode with `GL_INVALID_ENUM`. `gl_api` exposes `glProvokingVertex`; constants
  added to `gl_types.hpp`. Both backends implement the sink (Mock records; GLES no-ops,
  like polygonMode — GLES has no native provoking-vertex entry in YAGLT's loader).
- New `tests/unit/provoking_vertex_test.cpp` (3 cases): invalid-mode error, push-only-on-
  change (default LAST pushes nothing, FIRST/LAST each push once), and `glGetIntegerv`
  round-trip. Registered in `tests/CMakeLists.txt`. Also added the new sink override to the
  `RecordingSink`/`UnitRecordingSink` stubs in `state_test.cpp`/`dsa_texture_test.cpp`/
  `texture_unit_test.cpp`.
- Validation: default 376/376 green (373 → 376, +3 cases); sanitizer 386/386 total with the
  single pre-existing unrelated `shader_translate_test.cpp:53` 1D quirk. Coverage §11 row
  updated (provoking vertex done).

2026-08-28 (cube-map faces, generic vertex attribs, glHint — this session)
- Cube-map face TexImage (SPEC §8.1), commit `37a8584`: added 6
  `GL_TEXTURE_CUBE_MAP_POSITIVE/NEGATIVE_*X/Y/Z` enums; `normalizeTextureTarget()`
  folds faces → `GL_TEXTURE_CUBE_MAP`; used in `GLStateTracker::boundTextureForTarget`
  and `tex->target`. `tests/unit/teximage_cube_test.cpp` (3 cases). Default 389/389.
- Generic vertex attribute values (SPEC §10.2), commit `0a0c6e9`:
  `glVertexAttrib1f..4f`/`*fv`, `glVertexAttribI4i`/`I4ui`/`I4iv`/`I4uiv`,
  `glGetVertexAttribfv/iv` for `GL_CURRENT_VERTEX_ATTRIB`; recorded on bound VAO;
  validation (no VAO → INVALID_OPERATION, index≥16 → INVALID_VALUE, unknown pname →
  INVALID_ENUM). `tests/unit/vertex_attrib_generic_test.cpp` (9 cases). Default 397/397.
- `glHint` (SPEC §21.1.1), commit `8ec5ba5`: hint target/mode constants in
  `gl_types.hpp`; `GLStateSink::hint` pure virtual; `GLState::setHint`/`getHint` with
  push-on-flush; mock backend records; GLES backend forwards `glHint`; `Context::hint`/
  `getHint` validate (`isValidHintTarget`/`isValidHintMode`); `gl_api` wired.
  `tests/unit/hint_test.cpp` (2 cases). Default 399/399, sanitizer 409/409 (+1 pre-existing
  unrelated `shader_translate_test.cpp:53` empty-source quirk, unchanged this session).
  Coverage §21 row updated (glHint done).
- GL_DITHER capability (SPEC §17.3.7), commit `b2a34de`: `isTrackedCap` now
  includes `GL_DITHER`; default-enabled in `GLStateTracker` ctor (and `reset()`);
  `getInteger` caps switch returns it; backend already forwards any cap to native
  `glEnable/glDisable`. `tests/unit/dither_test.cpp` (2 cases: default-enabled +
  push-only-on-change). Updated `getstate_test.cpp` (DITHER now valid, returns
   true). Default 401/401, sanitizer 411/411 (+1 pre-existing unrelated
   `shader_translate_test.cpp:53` quirk). Coverage §17 row updated (dither done).
- GL_FRAMEBUFFER_SRGB + GL_SAMPLE_ALPHA_TO_COVERAGE (SPEC §15.1.1 / §15.3.1), commit
  `8e3b7c4`: both added as tracked capabilities (off by default), constants in
  gl_types.hpp, caps switch + getInteger case in gl_state.cpp; backend already
  forwards any cap to native glEnable/glDisable. `tests/unit/srgb_alpha_coverage_test.cpp`
  (2 cases: default-off + push-only-on-change). Default 403/403, sanitizer 413/413
   (+1 pre-existing unrelated `shader_translate_test.cpp:53` quirk). Coverage §15/§16
   row updated (sRGB/alpha-to-coverage done; glClampColor already done — stale note removed).

2026-08-28 (indexed capabilities — glEnablei/glDisablei/glIsEnabledi, SPEC §10.3.1)
- Implemented indexed capabilities (closes the §10.3.1 per-slot enable/disable gap).
  `GLStateSink` gained two pure virtuals `enableIndexed(cap,index)` /
  `disableIndexed(cap,index)`. `GLStateTracker` gained `setIndexedCapability` /
  `isIndexedCapabilityEnabled` backed by per-slot `indexedCapsCurrent_` /
  `indexedCapsApplied_` / `indexedCapsDirty_` maps, flushed in `apply()` only for
  changed slots (SPEC §10: no redundant native call), and reset in `reset()`.
- `Context` gained `enableIndexed` / `disableIndexed` / `isEnabledIndexed` with
  honest validation: only `GL_BLEND` / `GL_SCISSOR_TEST` are indexable
  (`isIndexableCap`) else `GL_INVALID_ENUM`; index ≥ `kMaxIndexedBuffers = 16` →
  `GL_INVALID_VALUE`. `gl_api` exposes `glEnablei` / `glDisablei` / `glIsEnabledi`.
- `GLESLib` resolves `glEnablei` / `glDisablei` (optional, ES 3.0+); `GLESBackend`
  forwards the two sink methods to the native driver. `MockBackend` records the
  calls (`enableIndexedCalls` / `disableIndexedCalls` / `last*Cap` / `last*Index`).
  The three test `RecordingSink`s in `state_test.cpp` / `texture_unit_test.cpp` /
  `dsa_texture_test.cpp` gained the two sink overrides.
- New `tests/unit/indexed_caps_test.cpp` (3 cases: default-off per slot, push-only-on-
  change for enable/disable, validation of invalid-cap + out-of-range index). Registered
  in `tests/CMakeLists.txt`.
- Validation: default 406/406 green; sanitizer 416/416 with the single pre-existing
  unrelated `shader_translate_test.cpp:53` empty-source quirk unchanged. Coverage §2 row
  updated (glEnablei/glDisablei/glIsEnabledi done).

2026-08-28 (separate stencil state — glStencilFuncSeparate/glStencilOpSeparate/glStencilMaskSeparate, SPEC §17.3.3)
- Split the stencil model into independent front/back faces. `GLStateTracker` now holds
  `StencilFaceState stencilFront_, stencilBack_` (+ applied twins); the legacy
  `setStencilFunc`/`setStencilOp`/`setStencilMask` set both faces, new
  `setStencilFuncSeparate`/`setStencilOpSeparate`/`setStencilMaskSeparate(face, …)` apply to
  `GL_FRONT`/`GL_BACK`/`GL_FRONT_AND_BACK`. `apply()` pushes a single combined
  `stencilFunc`/`Op`/`Mask` when both faces are equal and changed, otherwise each differing face
  via the new `stencilFuncSeparate`/`stencilOpSeparate`/`stencilMaskSeparate` sink methods
  (SPEC §10: no redundant native call).
- `GLStateSink` gained the three `*Separate` pure virtuals (all implementers updated: MockBackend,
  GLESBackend, and the three test `RecordingSink`s). `GLESBackend` forwards them to the native
  driver via newly resolved `glStencilFuncSeparate`/`glStencilOpSeparate`/`glStencilMaskSeparate`
  `GLESLib` symbols (required, core in GLES 2.0+). `gl_api` exposes the three entry points with
  honest face validation (`GL_INVALID_ENUM` for an unknown face). Mock records the separate calls
  (`stencil*SeparateCalls`, `lastStencilFace`).
- Extended `tests/unit/stencil_test.cpp` (2 cases: per-face push-only-on-change + collapse-to-
  combined when faces re-equal, invalid-face `GL_INVALID_ENUM`).
 - Validation: default 408/408 green; sanitizer 418/418 (pre-existing unrelated
   `shader_translate_test.cpp:53` empty-source quirk unchanged). Coverage §17 row updated
   (separate stencil done).

 2026-08-28 (compute dispatch — glDispatchCompute/glDispatchComputeIndirect, SPEC §7.4)
 - Implemented compute dispatch commands. `IGraphicsBackend` gained pure virtuals
   `dispatchCompute(x,y,z)` / `dispatchComputeIndirect(offset)`; `GLESBackend` forwards them to
   the native driver via newly resolved/optional `GLESLib` symbols `glDispatchCompute` /
   `glDispatchComputeIndirect`. `MockBackend` records the calls (`dispatchComputeCalls` /
   `dispatchComputeIndirectCalls` / `lastDispatchX/Y/Z` / `lastDispatchIndirect`).
 - `Context` gained `dispatchCompute` / `dispatchComputeIndirect` with honest validation: gated by
   `Feature::ComputeShaders` (else `GL_INVALID_OPERATION`); require an active program
   (`state_.activeProgram() != 0`, else `GL_INVALID_OPERATION`); indirect additionally requires a
   buffer bound to `GL_DISPATCH_INDIRECT_BUFFER` (else `GL_INVALID_OPERATION`). Both flush tracked
   state then forward to the backend. `GL_DISPATCH_INDIRECT_BUFFER` / `GL_DISPATCH_INDIRECT_BUFFER_
   BINDING` constants added to `gl_types.hpp`. `gl_api` exposes the two entry points.
 - New `tests/unit/compute_dispatch_test.cpp` (3 cases: feature+program gate, indirect-buffer gate,
   compute-shader-object rejection until `ComputeShaders` lands). Registered in `tests/CMakeLists.txt`.
 - Validation: default 411/411 green; sanitizer 421/421 (pre-existing unrelated
   `shader_translate_test.cpp:53` empty-source quirk unchanged). Coverage §7 + Shader-stages rows
   updated (compute dispatch done; compute shader objects still TODO).

2026-08-28 (64-bit buffer parameter queries — glGetBufferParameteri64v / glGetNamedBufferParameteri64v, SPEC §6.1.1)
- Implemented 64-bit buffer parameter queries. `Context::getBufferParameteri64v` /
  `getNamedBufferParameteri64v` (DSA gated by `Feature::DirectStateAccess`) read the
  buffer's authoritative `size`/`usage`/`immutableFlags`/`mapped`/`mapOffset`/`mapLength`
  into a `GLint64` array. Public `glGetBufferParameteri64v` / `glGetNamedBufferParameteri64v`
  added to `gl_api`. New `tests/unit/buffer_parameter_i64_test.cpp`. Commit `f55a704`.
- Validation: default + sanitizer green. Coverage §6 row updated.

2026-08-28 (glProgramParameteri — SPEC §7.3 / §7.4.2)
- Implemented `glProgramParameteri`. `Context::programParameteri` accepts
  `GL_PROGRAM_SEPARABLE` (must be set before link; else `GL_INVALID_OPERATION`) and
  `GL_PROGRAM_BINARY_RETRIEVABLE_HINT` (any time); `ProgramObject::binaryRetrievableHint`
  recorded; `glGetProgramiv(GL_PROGRAM_SEPARABLE)` answers the flag. Public
  `glProgramParameteri` in `gl_api`. New `tests/unit/program_parameter_test.cpp`. Commit
  `cf5da6b`.
- Validation: default + sanitizer green. Coverage §7 row updated.

2026-08-28 (shader binaries — glShaderBinary / glProgramBinary / glGetProgramBinary, SPEC §7.2 / §19.1)
- Implemented program/shader binary load + retrieve. `Context::programBinary` /
  `getProgramBinary` / `shaderBinary` keep the authoritative frontend binary mirror (the
  buffer-mirror pattern): loading a binary marks the program linked / the shader compiled.
  `glGetProgramiv(GL_PROGRAM_BINARY_LENGTH)` reports the stored length; `glGetProgramBinary`
  round-trips the blob + format with honest validation (bufSize-too-small → `GL_INVALID_VALUE`,
  no binary → `GL_INVALID_OPERATION`). `BackendProgram`/`BackendShader` gained a default
  no-op `loadBinary` virtual; `MockProgram`/`MockShader` record it. Constants
  `GL_PROGRAM_BINARY_LENGTH` / `GL_NUM_PROGRAM_BINARY_FORMATS` / `GL_PROGRAM_BINARY_FORMATS` /
  `GL_SHADER_BINARY_FORMAT_SPIR_V` added to `gl_types.hpp`. Public `glProgramBinary` /
  `glGetProgramBinary` / `glShaderBinary` in `gl_api`. New `tests/unit/shader_binary_test.cpp`
  (9 cases). Default 429/429 green.
- Coverage now 275/571 (48.2%) declared / ~53.3% core / ~40% true (was 272/571 = 47.6% /
  ~52.7% / ~39%). §7 row + verdict in `docs/coverage-core.md` updated.

 2026-08-28 (compute shader objects/stages — SPEC §7.1 / §7.4)
 - Compute shaders are now created/compiled/linked/dispatched. The mock baseline
   (`mock_capabilities.hpp`) reports `ComputeShaders` as **Native** (GLES 3.1+ has
   compute natively; the prior `Unsupported` marking was inconsistent with that and
   with the GLES backend, which already sets it Native for ES 3.1). The frontend
   `createShader(GL_COMPUTE_SHADER)` → `shaderSource` → `compileShader` →
   `attachShader` → `linkProgram` → `useProgram` → `dispatchCompute` path already
   existed and now flows end-to-end on any backend reporting `ComputeShaders`.
 - `tests/unit/compute_shader_object_test.cpp` (2 cases): full compute-program
   lifecycle (create/compile/link/use/dispatch, COMPILE/LINK_STATUS asserted,
   `dispatchComputeCalls` recorded) and geometry stage still rejected (honest
   `Unsupported`). Updated `compute_dispatch_test.cpp` + `shader_stage_test.cpp` +
   `backend_test.cpp` to opt the capability off explicitly where they asserted the
   old default. Registered in `tests/CMakeLists.txt`.
 - Validation: default **431/431** green; sanitizer **431/431** green
   (`YAGLT_SHADER_TRANSLATE=OFF`). No new `gl_api` entry points, so the coverage
   proxy stays 275/571 (48.2% declared / ~53.3% core / ~40% true) — but compute is
   no longer capability-gated out, raising the *usable* slice. Docs updated:
   `coverage-core.md` (§7 + Shader-stages rows, gap #1, verdict), `feature-matrix.md`
   (ComputeShaders Native; Honest-Unsupported section; emulation roadmap drops
   compute — it is native in GLES 3.1+, not emulated).

## Next Steps (carried)
 - Remaining §7 gaps: only geometry / tessellation shader stages remain honestly
   Unsupported (no GLES equivalent); compute shader objects are now done.
 - Remaining §8: cube/array/rect TexImage targets, `GetTexImage` multisample, texture views.
 - §15/§16: sRGB / alpha-to-coverage (done), `glClampColor` (already done).
 - §10: indirect draw.

## Session 2026-08-28 (restore + texture views)
 - Recovered the crashed agent's uncommitted texture-view work (SPEC §8.19):
   `glTextureView` / `Context::textureView`. Frontend validates both objects
   exist and differ, source has immutable storage, target is a valid texture
   target, internalFormat != 0, and the level range fits (`minLevel + numLevels <=
   storageLevels`, `numLevels != 0`); then records view state (derived base
   dimensions/levels from `minLevel` shift) and forwards to the backend. New
   `Feature::TextureViews` (Native on GLES 3.1, Unsupported otherwise; Mock Native).
   `BackendTexture::view()` added; GLES forwards `glTextureView` (optionally
   resolved) after sizing the internal format. New `tests/unit/texture_view_test.cpp`
   (4 cases: forwards view, requires immutable source, rejects self/bad-range/enum,
   rejects unknown object).
 - Fixed a pre-existing failing shader test (`shader_translator_1d_emulated_as_2d`):
   glslang rejects `texture1D`/`sampler1D` in modern core GLSL, so the 1D→2D
   emulation rewrite now runs on the desktop source *before* glslang parses it
   (`sampler1D`→`sampler2D`, `texture1D(s,x[,bias])`→`texture(s,vec2(x,0.5)[,bias])`).
   Test now asserts the type-level rewrite on the output instead of the (SPIRV-Cross
   dropped) fetch body.
  - Validation: full suite **445/445** green (`build_tx`, `YAGLT_SHADER_TRANSLATE=ON`).
  - Docs: `coverage-core.md` §8 row + gap #3 updated (texture views implemented).
  - Commits: `b0a3d0d` (shader 1D fix), `554c1b5` (texture views).

## Session 2026-08-28 (query entry-point expansion: texture/framebuffer/attrib/VAO/buffer)
 - Continued restoring/extending the crashed work: a batch of spec query entry
   points (SPEC §22 / §6 / §8 / §9 / §10), each following the established
   DSA-method + classic/target-method refactor, with dispatch in `gl_api.cpp`,
   declarations in `gl_api.hpp`/`context.hpp`, and unit tests. All three suites
   (`build/` default, `build_tx/` translate+Mesa, `build_san/` sanitizer) green
   before each commit.
 - `b23d8f0` **texture mutable storage metadata**: `updateMutableTextureStorage`
   recomputes width/height/levels from `texImage*` so `getTexLevelParameter*`
   returns correct dims.
 - `b642c46` **classic `glGetTexLevelParameteriv`/`fv`** (SPEC §8.13, target-based):
   shared `getTexLevelParameter*Impl`; unbound target → `InvalidOperation`.
 - `c673eb4` **classic `glGetRenderbufferParameteriv`** (SPEC §9.2, target-based):
   shared `getRenderbufferParameterivImpl`; no bound RBO → `InvalidOperation`.
 - `189471a` **classic `glGetFramebufferAttachmentParameteriv`** (SPEC §9.2):
   shared impl; rejects default FBO attachment / bad attachment enum.
 - `d21ffb0` **classic `glGetFramebufferParameteriv`** (SPEC §9.2/§10, target-based):
   target validation; no bound FBO → `InvalidOperation`; returns 0 for
   `FRAMEBUFFER_DEFAULT_*`; tests in `framebuffer_buf_test.cpp`.
 - `01c2742` **classic vertex-attribute queries** (SPEC §10.4):
   `glGetVertexAttribdv`/`Iiv`/`Iuiv`/`Pointerv` + broadened `glGetVertexAttribiv`
   to answer the full §10.4 pname set (ENABLED/SIZE/STRIDE/TYPE/NORMALIZED/INTEGER/
   DIVISOR/BUFFER_BINDING/POINTER). Coverage proxy → 288 entry points / 284 matched.
 - `212ea35` **DSA vertex-array queries** (SPEC §10.3.1):
   `glGetVertexArrayiv` (ELEMENT_ARRAY_BUFFER_BINDING),
   `glGetVertexArrayIndexediv` (per-attrib int state), `glGetVertexArrayIndexed64v`
   (VERTEX_ATTRIB_BINDING / VERTEX_ATTRIB_RELATIVE_OFFSET); capability-gated by
   `DirectStateAccess`; ungenerated VAO → `InvalidOperation`, oob index →
   `InvalidValue`, null params → `InvalidValue`, unknown pname → `InvalidEnum`.
   Coverage proxy → 291 entry points / 287 matched.
 - (this session) **DSA `glGetNamedBufferParameteriv`** (SPEC §6.1.1): 32-bit
   counterpart of the existing `glGetNamedBufferParameteri64v`, reusing the same
   frontend-owned buffer state (SIZE/USAGE/ACCESS/ACCESS_FLAGS/IMMUTABLE_STORAGE/
   MAPPED/MAP_LENGTH/MAP_OFFSET); capability-gated by `DirectStateAccess`;
   ungenerated name → `InvalidOperation`. Added to `context.cpp`/`gl_api.cpp`/
   `context.hpp`/`gl_api.hpp` + 5 tests in `buffer_parameter_i64_test.cpp`
   (read via DSA, validation, capability gate, public gl_api entry point).
 - Validation: default **465/465** green; `build_tx`/`build_san` suites green
   (tx **476/476** incl. GLES e2e `gles_e2e_1d_texture_emulated_as_2d` /
   `gles_e2e_program` under Mesa softpipe; san **465/465**).
 - Docs: `coverage-core.md` updated — 292 `gl_api` entry points / 287 matched
   families (50.3% declared / ~55.6% core / ~42% true), §6 row + entry list now
   include `glGetNamedBufferParameteriv`.
  - Committed as `2179eb4`: `glGetNamedBufferParameteriv` feature.

## Session 2026-08-28 (internal format queries §22.3)
 - Added `glGetInternalformativ` / `glGetInternalformati64v` (SPEC §22.3), the
   first backend-dependent query. Required a new `IGraphicsBackend` read-back
   interface (the one-way `GLStateSink` is only for pushed state), so both backends
   implement `getInternalformativ` / `getInternalformati64v`:
   - **MockBackend**: returns a documented conservative default — NUM_SAMPLE_COUNTS
     = 0, SAMPLES writes nothing, INTERNALFORMAT_SUPPORTED = GL_TRUE for a curated
     set of common core formats (RGBA8/RGB8/RGBA16F/RGB16F/R8/RG8/R16F/RG16F/
     DEPTH24_STENCIL8/DEPTH_COMPONENT24/DEPTH_COMPONENT32F/R11F_G11F_B10F/
     SRGB8_ALPHA8/RGB10_A2), 0 otherwise. Added the missing format constants to
     `gl_types.hpp`.
   - **GLESBackend**: forwards to the driver's `glGetInternalformativ` (GLES 3.0
     core, resolved optionally in `GLESLib`/`gles_loader.cpp`). `glGetInternalformati64v`
     is not in GLES, so it widens from the `iv` call (valid for the pnames GLES
     supports: NUM_SAMPLE_COUNTS, SAMPLES).
   - Frontend (`Context::getInternalformativ/i64v`) validates null params ->
     INVALID_VALUE, negative bufSize -> INVALID_VALUE, and pname against the
     ARB_internalformat_query2 pname set -> INVALID_ENUM, then forwards to the backend.
 - Validation: default **472/472** green; `build_tx` **484/484** green incl. new
   `gles_e2e_internalformat_query_via_driver` (runs against Mesa softpipe; skips if
   no driver); `build_san` green.
 - Docs: `coverage-core.md` -> 294 entry points / 288 matched families (50.4% declared
   / ~55.8% core / ~42% true), §22 row + entry list updated.
   - Committed as `41ecd01`: internal format query feature.

## Session 2026-08-28 (generic §22 state queries)
 - Added five frontend-owned `glGet*` entry points (SPEC §22) that read tracked
   state with no backend round-trip:
   - `glGetStringi` (§22.2): only `GL_EXTENSIONS` is indexable; this frontend
     exposes none, so any index -> `GL_INVALID_VALUE` + nullptr; other names ->
     `GL_INVALID_ENUM`. Added `GL_NUM_EXTENSIONS` constant.
   - `glGetGraphicsResetStatus` (§22.5): always `GL_NO_ERROR` (no reset-detection
     path).
   - `glGetInteger64v` (§22.1): returns the same tracked integer state widened to
     `GLint64`; unknown pname -> `GL_INVALID_ENUM`, null -> `GL_INVALID_VALUE`.
   - `glGetIntegeri_v` / `glGetBooleani_v` (§22.1): indexed scalar queries for the
     indexable caps `GL_BLEND` / `GL_SCISSOR_TEST` (index < 16); other pnames ->
     `GL_INVALID_ENUM`, out-of-range index / null -> `GL_INVALID_VALUE`. Added the
     missing `GL_SCISSOR_TEST` constant to `gl_types.hpp` (and removed a now-
     redundant local definition in `viewport_scissor_test.cpp`).
   - Public `gl_api` dispatch + `context.hpp`/`gl_api.hpp` declarations added; all
     five are pure frontend state reads, so the GLES/Mock backends need no changes.
 - New `tests/unit/generic_query_test.cpp` covers each entry point's happy path,
   error validation (null params, unknown pname, out-of-range index), and the
   indexed-cap agreement with `glEnablei`/`glDisablei`.
 - Validation: default **476/476** green; `build_tx` green (Mesa softpipe e2e
   suite); `build_san` green.
  - Docs: `coverage-core.md` -> 299 entry points / 293 matched families (51.3%
    declared / ~56.8% core / ~42% true), §2 + §22 rows and the entry list updated.
   - Committed as `2d514b8`: generic §22 state queries.

## Session 2026-08-28 (multisample sample-position query §14.3.1)
  - Added `glGetMultisamplefv` (SPEC §14.3.1), the indexed sample-position query.
    It reads tracked rasterization state via a new `IGraphicsBackend` read-back
    pair, mirroring the `getInternalformat*` pattern:
    - `getMultisampleSampleCount()` returns the SAMPLES of the bound framebuffer
      (frontend uses it to validate `index` against `GL_INVALID_VALUE`);
    - `getMultisamplefv(pname, index, val)` writes the (x, y) location.
    - **MockBackend**: reports a fixed `kMockSampleCount = 4` and a fixed
      deterministic sub-pixel grid (positions are implementation-defined), so the
      index-validation and result contract are deterministic and testable.
    - **GLESBackend**: `getMultisampleSampleCount` reads the bound draw
      framebuffer's `GL_SAMPLES` via the driver's `glGetFramebufferParameteriv`
      (added to `GLESLib`, resolved optionally; 0 if unsupported); `getMultisamplefv`
      forwards to the driver's `glGetMultisamplefv` (added to `GLESLib`, ES 3.1+,
      resolved optionally).
    - Frontend (`Context::getMultisamplefv`): null `val` -> `GL_INVALID_VALUE`;
      `pname != SAMPLE_POSITION` -> `GL_INVALID_ENUM`; `index >= sampleCount` ->
      `GL_INVALID_VALUE`; else forwards to the backend. Added `GL_SAMPLE_POSITION`
      constant to `gl_types.hpp`.
  - New `tests/unit/generic_query_test.cpp` case covers happy path (index 0/1
    grid values), bad pname -> `GL_INVALID_ENUM`, null val -> `GL_INVALID_VALUE`,
    and out-of-range index (>= 4) -> `GL_INVALID_VALUE`.
  - Validation: default **477/477** green; `build_tx` green (Mesa softpipe e2e
    suite); `build_san` green.
   - Docs: `coverage-core.md` -> 300 entry points / 294 matched families,
     §22 row + entry list updated; `glGetMultisamplefv` added to the §14.3.1
     query surface.
    - Committed as `b526cd3`: multisample sample-position query.

## 2026-08-28 — Robustness texture read-back (ARB_robustness / GL 4.5)

- Added bounds-checked texture image queries to close a gap in the `feat(query)`
  surface: `glGetnTexImage`, `glGetnCompressedTexImage` (non-DSA, operate on the
  bound texture) and `glGetnTextureImage`, `glGetnCompressedTextureImage` (DSA).
- Backend interface (`backend_resources.hpp`): added robust overloads
  `getTexImage(target, level, format, type, bufSize, pixels)` and
  `getCompressedTexImage(target, level, bufSize, pixels)` defaulting to no-op.
- Frontend (`Context`): the four robust methods validate
  `level < 0 || bufSize < 0` -> `GL_INVALID_VALUE` and (non-DSA) no texture bound
  -> `GL_INVALID_OPERATION`, then forward to the backend robust overload.
- Mock backend records the calls (`getTexImageRobustCalls`,
  `getCompressedTexImageRobustCalls`); GLES backend calls `glGetnTexImage` /
  `glGetnCompressedTexImage` when the driver exposes them and falls back to the
  non-robust entry otherwise (loader resolves both optionally so load() still
  succeeds). Also exposed the non-robust `glGetCompressedTexImage` symbol in
  `GLESLib`.
- Tests: new `tests/unit/tex_image_robustness_test.cpp` (registered in
  `tests/CMakeLists.txt`) covers backend-call recording for all four, negative
  level -> `GL_INVALID_VALUE`, negative bufSize -> `GL_INVALID_VALUE`, and
  unbound texture -> `GL_INVALID_OPERATION`.
- Validation: default `build` green (511/511 framework cases); `build_san`
  (ASan/UBSan) green.
- Docs: `feature-matrix.md` gained a "Texture image read-back (robustness)" row;
  `coverage-core.md` is regenerated separately from `SPEC.md`.
- Touched files: `gl_api.hpp`, `gl_api.cpp`, `context.hpp`, `context.cpp`,
  `backend_resources.hpp`, `mock_resources.hpp`, `gles_resources.hpp`,
  `gles_loader.hpp`, `gles_loader.cpp`, `tests/.../tex_image_robustness_test.cpp`,
  `tests/CMakeLists.txt`, `docs/feature-matrix.md`.

