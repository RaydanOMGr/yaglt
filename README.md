# YAGLT — Yet Another GL Translator

> A backend-agnostic **OpenGL 4.6 compatibility and translation layer**.

YAGLT presents a desktop **OpenGL 4.6 (compatibility profile)** API to applications
while translating operations onto one or more interchangeable *backend* graphics
APIs. The primary target today is translating desktop OpenGL → **OpenGL ES** (host
Mesa / Android), but the design is not tied to any single backend: a mock test
backend, GLES, and a planned Vulkan backend all sit behind the same stable
interfaces, and desktop GL / Metal / software backends can be added the same way.

---

## Contents

- [What is YAGLT?](#what-is-yaglt)
- [Why YAGLT exists](#why-yaglt-exists)
- [Project status](#project-status)
- [Features & design principles](#features--design-principles)
- [Supported backends](#supported-backends)
- [Architecture at a glance](#architecture-at-a-glance)
- [Directory layout](#directory-layout)
- [Building](#building)
- [Testing](#testing)
- [Quick start](#quick-start)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

---

## What is YAGLT?

YAGLT is a C++17 library that implements the *frontend* of an OpenGL driver:
object identity and lifetime, capability resolution, state tracking, validation,
and OpenGL error semantics. It does **not** itself talk to a GPU. Instead it routes
work through a small set of abstract interfaces (`IGraphicsBackend`,
`IResourceFactory`, `IShaderCompiler`, `ICapabilities`) to a concrete backend that
owns native resource creation, command submission, and synchronization.

The intent is that an application (or a drop-in `libEGL.so` shim — see
[architecture.md](docs/architecture.md)) calls standard OpenGL functions, and YAGLT
decides, per feature and per backend capability, whether to:

- issue the call **natively** on the backend,
- use a backend **extension**,
- **emulate** the feature (e.g. 1-D textures as sliced 2-D textures on GLES), or
- report it **Unsupported** honestly (`GL_INVALID_OPERATION` or a capability query),

…without ever silently pretending unsupported functionality works.

## Why YAGLT exists

- **Run desktop OpenGL where only GLES exists.** Mobile, embedded, and many
  headless/CI environments expose OpenGL ES but not a full desktop GL driver.
- **Compatibility-profile semantics on core-only drivers.** Legacy applications
  that rely on compatibility behavior can be served by a layer that understands
  those semantics rather than failing at link/runtime.
- **A validation & translation harness.** A capability-driven, backend-agnostic
  frontend makes it possible to test OpenGL semantics headlessly (mock backend,
  no GPU) and to translate desktop GLSL → GLSL ES deterministically.
- **Honesty over fakery.** The project's core rule is: if a feature is not actually
  implemented and tested, it is reported as unsupported — never faked.

## Project status

This repository is in **early foundational development**. The interfaces, the mock
backend, the GLES backend, the shader-translation pipeline, centralized state
tracking, and a substantial slice of the OpenGL 4.6 query/object API surface are
implemented and tested. It is **not yet claimable as "OpenGL 4.6 compatible."**

For an honest, machine-generated, current snapshot see:

- [`docs/coverage-core.md`](docs/coverage-core.md) — how much of the OpenGL 4.6
  core-profile command surface is implemented (regenerated from the spec).
- [`docs/feature-matrix.md`](docs/feature-matrix.md) — per-feature support status,
  implementation location, and tests.
- [`docs/agent-progress.md`](docs/agent-progress.md) — development journal / source
  of truth for status and next steps.

## Features & design principles

- **Backend-agnostic frontend.** The OpenGL-facing API is independent of any
  backend's native types; native handles never leak into the public surface.
- **Capability-driven selection.** Feature availability is resolved once at init
  into a `CapabilityTable`; emulation paths are chosen by the capability system,
  not scattered `if (version >= …)` checks.
- **Honest capability reporting.** Unsupported features fail loudly or report
  `Unsupported`, never fake success.
- **Headless-testable.** A mock backend enables the full frontend + state + object
  model to be exercised without a GPU.
- **Runtime-loaded GLES.** EGL/GLES are resolved via `dlopen`/`dlsym`, so YAGLT
  builds on hosts without GLES dev libraries; absent drivers → honest init failure.
- **Deterministic shader translation.** Desktop GLSL → glslang (GLSL→SPIR-V) →
  SPIRV-Cross (SPIR-V→GLSL ES 3.10) under `YAGLT_SHADER_TRANSLATE=ON`.
- **C++17, CMake, static core library** (`yaglt_core`), permissive build options.

## Supported backends

| Backend | Status | Notes |
|---------|--------|-------|
| **Mock** | Implemented | Headless test backend; records every call, drives the test suite. |
| **GLES** | Implemented | Runtime-loaded (`dlopen`); surfaceless EGL context; works on host Mesa (softpipe, ES 3.1) and Android. |
| **Vulkan** | Planned | Interface scaffolding present (`src/backend/vulkan`); not yet functional. |
| Desktop GL / Metal / Software | Future | Same stable interfaces; no implementation yet. |

## Architecture at a glance

```
Application
   │   standard OpenGL 4.6 calls (gl*)
   ▼
OpenGL 4.6 Compatibility Frontend        (object model, validation, errors)
   │
   ▼
Frontend State + Capability tracking     (GLStateTracker, CapabilityTable)
   │
   ▼
Graphics Abstraction Layer               (IGraphicsBackend, IResourceFactory,
   │                                       IShaderCompiler, ICapabilities)
   ├── GLES Backend      (runtime-loaded, surfaceless EGL)
   ├── Mock Backend      (headless tests)
   └── Future Backends   (Vulkan, …)
```

The frontend owns OpenGL semantics; backends own native resources. See
[`docs/architecture.md`](docs/architecture.md) for the full design (layers,
resource lifetime, shader translation, state management, indexed buffer bindings,
and the planned `libEGL.so` drop-in shim).

## Directory layout

```
yaglt/
├── CMakeLists.txt                 # top-level build (options + tests)
├── SPEC.md                        # authoritative requirements contract
├── OpenGL-4.6-Compatibility.md    # vendored spec used for coverage analysis
├── AGENTS.md                      # agent/dev rules (build dirs, commits)
├── include/glcompat/              # public API
│   ├── core/                      # backend.hpp, factory.hpp, capabilities*.hpp, platform.hpp, log.hpp
│   ├── backend/gles/              # GLES backend interface + loader + capabilities
│   ├── frontend/                  # context.hpp, gl_api.hpp, gl_types.hpp, objects.hpp, error.hpp
│   ├── platform/android/          # Android platform capabilities / shared memory
│   └── state/                     # gl_state.hpp, gl_state_sink.hpp
├── src/
│   ├── core/                      # capability table, logging
│   ├── backend/{mock,gles,common,vulkan}
│   ├── frontend/                  # context.cpp, gl_api.cpp
│   ├── objects/                   # frontend object model
│   ├── platform/{linux,android}
│   ├── shader/                    # shader_translator.{hpp,cpp}
│   ├── state/                     # gl_state.cpp
│   └── emulation/                 # feature emulation (e.g. 1-D → 2-D textures)
├── tests/                         # unit / integration / compatibility / backend
│   └── framework/                 # self-contained test framework (test_framework.hpp)
├── third_party/                   # (vendored as needed)
└── docs/                          # architecture, feature-matrix, coverage, journal, …
```

## Building

Requirements: **CMake ≥ 3.16** and a **C++17** compiler (GCC or Clang).

Build options (see `CMakeLists.txt` / `src/CMakeLists.txt`):

| Option | Default | Meaning |
|--------|---------|---------|
| `YAGLT_BUILD_TESTS` | `ON` | Build the unit/integration/backend test suite. |
| `YAGLT_ENABLE_SANITIZERS` | `OFF` | ASan + UBSan in Debug builds. |
| `YAGLT_BACKEND_GLES` | `ON` | Build the OpenGL ES backend (runtime-loaded). |
| `YAGLT_SHADER_TRANSLATE` | `OFF` | Build glslang + SPIRV-Cross for shader translation. |
| `YAGLT_ANDROID_STUB` | `OFF` | Build against Mesa `android_stub` headers on host. |

### Default build (self-contained, no GPU)

```sh
cmake -S . -B build -DYAGLT_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
```

This builds `yaglt_core` plus the mock + GLES backends and the test suite. The
GLES backend is runtime-loaded, so the build itself needs **no** GLES dev
libraries.

### Sanitizer build (recommended before considering work done)

```sh
cmake -S . -B build_san -DYAGLT_BUILD_TESTS=ON -DYAGLT_ENABLE_SANITIZERS=ON
cmake --build build_san -j"$(nproc)"
```

### Shader-translation build (needs glslang + SPIRV-Cross)

Enable `YAGLT_SHADER_TRANSLATE=ON`. The translator is pulled in via
`add_subdirectory` from sibling source trees:

- `../glslang-main` (built with `ENABLE_OPT=OFF`, so SPIRV-Tools is **not**
  required), and
- `../SPIRV-Cross-main`.

```sh
cmake -S . -B build_tx -DYAGLT_BUILD_TESTS=ON -DYAGLT_SHADER_TRANSLATE=ON
cmake --build build_tx -j"$(nproc)"
```

### Running the GLES backend end-to-end (needs Mesa)

The GLES backend initializes against a real driver. On a headless Linux box, build
Mesa (softpipe, surfaceless) and point the loader at it:

```sh
LD_LIBRARY_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu \
LIBGL_DRIVERS_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu/dri \
GALLIUM_DRIVER=softpipe \
ctest --test-dir build_tx --output-on-failure
```

> **Build directories are untracked.** `build/`, `build_*/`, and `.kilo/` must
> never be committed — see [`AGENTS.md`](AGENTS.md). They are already git-ignored.

For a step-by-step, dependency-complete walkthrough, see
[`docs/building.md`](docs/building.md).

## Testing

The suite is built with CTest and split across `tests/unit`, `tests/integration`,
`tests/compatibility`, and `tests/backend`. Run any variant with:

```sh
ctest --test-dir build --output-on-failure
```

Recommended matrix (all expected green):

1. **Default** (`build`) — mock backend, no GPU.
2. **Sanitizer** (`build_san`) — ASan/UBSan; run before declaring work done.
3. **Translation + GLES e2e** (`build_tx`) — with Mesa on the library path.

> **Framework note.** The test `EXPECT_*` macros do **not** throw; a failed
> expectation still prints `[PASS]` for that case but increments the global
> failure count. Always trust the trailing **`X/Y tests passed, Z failed`**
> summary, not the per-case `[PASS]` lines.

## Quick start

Minimal illustrative use of the frontend API (real symbols from
`include/glcompat`):

```cpp
#include <glcompat/backend/gles/gles_backend.hpp>
#include <glcompat/frontend/context.hpp>
#include <glcompat/frontend/gl_api.hpp>

int main() {
    // Select a backend. GLESBackend loads EGL/GLES at runtime and returns
    // false honestly if no driver is available.
    glcompat::GLESBackend backend;
    if (!backend.initialize()) {
        // No GLES driver on this host — handle gracefully.
        return 1;
    }

    // Bind the backend to a frontend context and make it current.
    glcompat::Context ctx(backend);
    glcompat::setCurrentContext(&ctx);

    // Standard OpenGL calls are now routed through the frontend.
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    // Queries and errors follow desktop GL semantics.
    if (glGetError() != GL_NO_ERROR) { /* … */ }

    glcompat::setCurrentContext(nullptr);
    return 0;
}
```

The free `gl*` functions dispatch through the *current* context
(`glcompat::getCurrentContext()`); an unbound context yields `GL_INVALID_OPERATION`.
The full frontend surface is in `include/glcompat/frontend/gl_api.hpp`.

## Documentation

| Document | Purpose |
|----------|---------|
| [`SPEC.md`](SPEC.md) | Authoritative requirements contract (the spec). |
| [`docs/architecture.md`](docs/architecture.md) | Layers, interfaces, state, shader translation, planned shim. |
| [`docs/feature-matrix.md`](docs/feature-matrix.md) | Per-feature support status + implementation location. |
| [`docs/coverage-core.md`](docs/coverage-core.md) | Core-profile command coverage (regenerated). |
| [`docs/agent-progress.md`](docs/agent-progress.md) | Development journal / status + next steps. |
| [`docs/next-agent-prompt.md`](docs/next-agent-prompt.md) | Hand-off prompt for continuing agent work. |
| [`docs/building.md`](docs/building.md) | Detailed build, dependencies, cross-compile, test matrix. |
| [`docs/contributing.md`](docs/contributing.md) | How to contribute / development workflow. |
| [`AGENTS.md`](AGENTS.md) | Hard rules for agents (build dirs, commits). |

## Contributing

Contributions follow the design principles above: keep the frontend backend-agnostic,
never expose native handles in the public API, and never fake unsupported features.
See [`docs/contributing.md`](docs/contributing.md) for the workflow, build/test
expectations, and documentation-update rules.

## License

No license is specified in this repository yet. Clarify licensing before
redistributing or incorporating YAGLT into another project.

## Acknowledgements

- **Khronos Group** — the OpenGL 4.6 specification and the vendored `GL`/`GLES`/
  `EGL`/`KHR`/`Vulkan` headers.
- **Mesa** — software/headless GLES (softpipe) used to exercise the real backend.
- **glslang** and **SPIRV-Cross** — the desktop-GLSL → GLSL-ES translation pipeline.
