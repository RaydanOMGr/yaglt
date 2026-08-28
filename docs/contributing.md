# Contributing to YAGLT

Thanks for wanting to improve YAGLT. This page covers the human-facing development
workflow. Agent-specific hard rules live in [`../AGENTS.md`](../AGENTS.md); the
continuing-agent hand-off prompt is [`next-agent-prompt.md`](next-agent-prompt.md).

## Principles (non-negotiable)

1. **Frontend stays backend-agnostic.** Never expose a native backend handle
   (`EGL*`, `GL*` from a driver, `Vk*`, …) in the public API under `include/glcompat`.
   Backends own native types behind the `BackendX` opaque handles.
2. **Never fake support.** If a feature is not actually implemented *and tested*,
   report it (`GL_INVALID_OPERATION` or a capability query returning `Unsupported`).
   The feature matrix vocabulary is `Native` / `Extension` / `Emulated` /
   `Unsupported` / `Not implemented` — use it honestly.
3. **Capability-driven, not version-sniffing.** Decide feature availability in the
   `CapabilityTable` at init, not via scattered `if (glesVersion >= …)` checks.

## Workflow

1. **Read the contract first.** `SPEC.md` is authoritative. For behavior changes,
   also read `docs/architecture.md` and the relevant SPEC section cited in commit
   messages (e.g. `SPEC §8.11.4`).
2. **Pick a task.** The recommended order and current gaps are in
   `docs/agent-progress.md` (journal) and `docs/next-agent-prompt.md`. Prefer small,
   coherent, tested increments over large unreviewed drops.
3. **Build + test before you stop.**
   - Default: `ctest --test-dir build --output-on-failure`
   - Sanitizer: `ctest --test-dir build_san --output-on-failure`
   - Translation/GLES e2e (needs Mesa): `ctest --test-dir build_tx --output-on-failure`
   See [`building.md`](building.md) for setup.
4. **Update docs with the code.** When behavior changes, update
   `docs/feature-matrix.md` and `docs/architecture.md` as needed, and append a status
   note to `docs/agent-progress.md`. Coverage numbers in `docs/coverage-core.md` are
   regenerated from `SPEC.md`, not hand-edited.
5. **Commit discipline.**
   - Do **not** commit `build/`, `build_*/`, or `.kilo/` (git-ignored; see
     [`../AGENTS.md`](../AGENTS.md)).
   - Commit after each coherent, tested step. Keep changes incremental.
   - Reference the SPEC section in the commit body so the requirement is traceable.

## What "done" means for a feature

- An implementation behind the appropriate interface (frontend `Context` method,
  backend `BackendX` virtual, or capability entry).
- Honest error/capability behavior for unsupported backends.
- At least one test exercising it (mock backend, and GLES backend when the feature
  maps to real native calls).
- Green default + sanitizer runs; translation run green when the feature touches
  shaders/translation.
- Docs updated (feature matrix + journal; coverage regenerated separately).

## Repository map (where things go)

| Concern | Location |
|---------|----------|
| Public API | `include/glcompat/{core,backend/frontend,state,platform}/*` |
| Frontend logic | `src/frontend/*`, `src/objects/*`, `src/state/*` |
| Backends | `src/backend/{mock,gles,common,vulkan}/*` |
| Shader translation | `src/shader/shader_translator.*` |
| Emulation | `src/emulation/*` |
| Capabilities | `src/core/capabilities_table.cpp`, `include/glcompat/core/capabilities*.hpp` |
| Tests | `tests/{unit,integration,compatibility,backend}/*` + `tests/framework/*` |
| Spec / docs | `SPEC.md`, `OpenGL-4.6-Compatibility.md`, `docs/*` |

## Before opening a change

- `clang-format` / `.clang-tidy` settings in the repo root are the style baseline.
- Run the sanitizer build; a clean sanitizer run matters more than a green default
  run for memory-safety work.
- Make sure no `build*` directory or `.kilo/` content is staged.
