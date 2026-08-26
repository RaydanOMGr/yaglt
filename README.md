# YAGLT — Yet Another GL Translator

A backend-agnostic OpenGL 4.6 compatibility and translation layer.

YAGLT exposes a desktop OpenGL 4.6-compatible API while translating operations
to one or more backend graphics APIs. The initial primary target is OpenGL ES
(especially Android, minimum SDK 21), but the architecture is not tied to any
single backend: GLES, Vulkan, desktop GL, Metal, software, and test backends
all sit behind the same stable interfaces.

## Goals

- Translate desktop OpenGL 4.6 (including compatibility profile semantics) to
  backends that may lack features, via native / extension / emulated paths.
- Never silently pretend unsupported functionality works.
- Keep the OpenGL-facing frontend independent of any backend's native types.
- Remain testable in a headless Linux environment (mock backend, no GPU).

## Status

This repository is in early foundational development. See
[docs/agent-progress.md](docs/agent-progress.md) and
[docs/feature-matrix.md](docs/feature-matrix.md) for an honest, current state.
It is **not** yet claimable as "OpenGL 4.6 compatible".

## Build

```sh
cmake -S . -B build -DYAGLT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

A sanitizer-enabled debug build is available with
`-DYAGLT_ENABLE_SANITIZERS=ON`.

## Layout

```
include/glcompat   public interfaces (frontend/backend/core)
src/core           capability table + shared frontend logic
src/backend/mock   headless test backend
src/platform/linux Linux platform capabilities
tests              unit / integration / compatibility / backend tests
docs               architecture, feature matrix, development journal
```

See [docs/architecture.md](docs/architecture.md) for the design.
