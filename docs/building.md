# Building & Testing YAGLT

This page is the detailed, dependency-complete companion to the *Building* and
*Testing* sections of the top-level [`README.md`](../README.md). Read it when the
quick snippets there are not enough (missing drivers, translation pipeline, Android,
or CI setup).

## 1. Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | ≥ 3.16 | Top-level `cmake_minimum_required`. |
| C++ compiler | C++17 | GCC or Clang. |
| Make / Ninja | any | Generator of choice. |
| GLES dev libs | **not required** to build | GLES is runtime-loaded; only needed at *runtime* for e2e tests. |
| Mesa (softpipe) | optional | Required only for real GLES backend tests on a headless host. |
| glslang | optional | Required only when `YAGLT_SHADER_TRANSLATE=ON`. |
| SPIRV-Cross | optional | Required only when `YAGLT_SHADER_TRANSLATE=ON`. |
| SPIRV-Tools / shaderc | **not required** | glslang is built with `ENABLE_OPT=OFF`. |

## 2. Default build (mock backend, no GPU)

```sh
cmake -S . -B build -DYAGLT_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

This is the self-contained path used in CI and local dev. It builds the static
`yaglt_core` library, the mock + GLES backends, and the test suite. The GLES backend
is compiled but loaded lazily, so its absence at runtime does not break the build.

## 3. Sanitizer build

Run a sanitizer build before declaring any task complete:

```sh
cmake -S . -B build_san -DYAGLT_BUILD_TESTS=ON -DYAGLT_ENABLE_SANITIZERS=ON
cmake --build build_san -j"$(nproc)"
ctest --test-dir build_san --output-on-failure
```

ASan + UBSan are applied automatically in Debug builds when the compiler is GCC or
Clang. Keep the address/UB sanitizer suppressions in `tools/lsan_mesa_suppressions.txt`
handy if Mesa leaks surface under the GLES backend.

## 4. Shader-translation build

Desktop GLSL → GLSL ES translation is opt-in. It pulls glslang and SPIRV-Cross in via
`add_subdirectory` from **sibling** source trees (they are *not* vendored in this
repo):

```sh
# layout expected at build time:
#   ../yaglt/
#   ../glslang-main/        (configure with ENABLE_OPT=OFF)
#   ../SPIRV-Cross-main/
cmake -S . -B build_tx -DYAGLT_BUILD_TESTS=ON -DYAGLT_SHADER_TRANSLATE=ON
cmake --build build_tx -j"$(nproc)"
```

- `glslang` is built with `ENABLE_OPT=OFF` so **SPIRV-Tools is not required**.
- `TranslatingGLESShaderCompiler` is selected automatically under this flag; already-
  GLSL-ES input is passed through without translation.
- `ShaderTranslator` injects default `layout(binding=…)` on blocks lacking one so
  desktop block syntax is portable to GLSL ES 3.10.

## 5. Running the GLES backend end-to-end (Mesa)

On a headless Linux host, build Mesa (softpipe, surfaceless) and install it to
`../mesa-26.2.1/install`, then:

```sh
LD_LIBRARY_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu \
LIBGL_DRIVERS_PATH=../mesa-26.2.1/install/lib/x86_64-linux-gnu/dri \
GALLIUM_DRIVER=softpipe \
ctest --test-dir build_tx --output-on-failure
```

The GLES backend creates a surfaceless EGL display and reports its renderer
(`renderer=softpipe, ES 3.x`). If `libEGL`/`libGLESv2` or required symbols are
absent, `GLESBackend::initialize()` returns `false` — YAGLT does **not** fake a
driver.

## 6. Android (host stub build)

To compile against Mesa's `android_stub` headers on a Linux host (no Android NDK
required for the stub path):

```sh
cmake -S . -B build -DYAGLT_BUILD_TESTS=ON -DYAGLT_ANDROID_STUB=ON
```

The real Android backend sources (`src/platform/android`) are never compiled into
Linux builds; only the stub headers are consumed. A full NDK cross-compile follows
the same CMake options with a proper Android toolchain file.

## 7. Build variants cheat-sheet

| Dir | Command | Purpose |
|-----|---------|---------|
| `build` | default | Mock backend, no GPU. |
| `build_san` | `+ -DYAGLT_ENABLE_SANITIZERS=ON` | ASan/UBSan. |
| `build_tx` | `+ -DYAGLT_SHADER_TRANSLATE=ON` | Translation + GLES e2e (needs Mesa). |
| `build_tr` | translation + sanitizer | Combined. |

All `build*` directories are untracked and git-ignored (see [`../AGENTS.md`](../AGENTS.md)).

## 8. Test framework notes

- Tests live under `tests/{unit,integration,compatibility,backend}` and share a
  self-contained framework in `tests/framework/test_framework.hpp`.
- `EXPECT_*` macros do **not** throw. A failed expectation still prints `[PASS]` for
  that case but bumps the global failure count. **Trust the trailing
  `X/Y tests passed, Z failed` summary**, not the per-case lines.
- Run sanitizer + default + translation matrices before considering a task done.

## 9. Common pitfalls

- **GLES tests fail to initialize off-box.** You forgot the Mesa `LD_LIBRARY_PATH` /
  `LIBGL_DRIVERS_PATH` / `GALLIUM_DRIVER` env vars (§5).
- **Translation build can't find glslang/SPIRV-Cross.** They must be sibling dirs
  (`../glslang-main`, `../SPIRV-Cross-main`); they are not vendored.
- **Committed a `build/` dir.** Don't — it is git-ignored; see `../AGENTS.md`.
- **Green per-case lines but a non-zero failure count.** That is the framework
  no-throw behavior; read the summary line.
