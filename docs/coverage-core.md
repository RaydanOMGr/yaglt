# OpenGL 4.6 Core Profile — Implementation Coverage

Status assessment for **YAGLT** as of 2026-08-26. Companion to
`docs/feature-matrix.md` and `docs/agent-progress.md`. The goal of this
document is to answer one question quantitatively: **how much of the OpenGL
4.6 spec is implemented, specifically the core profile?**

## Method

1. The OpenGL 4.6 (Compatibility Profile) specification (`OpenGL-4.6-
   Compatibility.md`, 23 780 lines) was scanned for command prototypes of the
   form `void <CamelName>(` — these are the spec's own command declarations
   (the `gl` prefix is omitted in the prose). This yields the **command universe**.
2. The implemented frontend surface (`include/glcompat/frontend/gl_api.hpp`)
   was extracted as `gl*` entry-point names. Each was normalized to a spec
   command family and matched against the command universe.
3. The **core profile** was isolated by removing the compatibility-only
   (removed-in-core) commands enumerated in spec **Appendix E.2.2** (immediate
   mode, fixed-function matrix/lighting/texgen, display lists, selection/
   feedback, evaluators, attribute stacks, pixel drawing, accum, etc.).

### Caveat on the number

The spec text defines many type/vector variants as a single family prototype
(e.g. `void Uniform1f(` is present, but `Uniform1fv`/`UniformMatrix4fv` and the
`TexParameter*`/`Get*` variants are not all emitted as separate `void X(`
prototypes). So the extracted universe of **490 prototypes undercounts** the
real GL command set (the real API has ~700+ entry points). Consequently the
percentages below are an **optimistic proxy**: they measure how many of the
spec's *declared command prototypes* have a frontend entry point, not the true
entry-point count. The qualitative chapter breakdown is the more reliable
signal.

## Headline numbers

| Universe | Prototypes | With frontend entry point | Coverage |
|----------|-----------:|--------------------------:|---------:|
| Full spec (compat + core) | 490 | 111 | **22.7%** |
| Core profile only (spec − 55 removed commands) | 435 | 111 | **25.5%** |

All 100 covered commands are real `gl_api` entry points with frontend semantics
and tests (mock path, most also against Mesa GLES). None of the 55
compatibility-only removed commands are implemented (correct — they are
out of scope per `docs/feature-matrix.md`).

**Estimated true core coverage** (adjusting for the undercount above and for
core commands that exist as an entry point but are capability-gated to
*Unsupported*, e.g. instanced draw / transform feedback on the mock, and
geometry/tess/compute which are not even entry points): **low-to-mid teens
percent of the real ~700-entry GL core command set.**

## Core coverage by spec area

Status: ✅ Implemented · 🟡 Partial · ❌ Not implemented · 🚫 Honestly Unsupported

| Spec area (chapter) | Status | Notes |
|---------------------|--------|-------|
| §2 Fundamentals / errors / strings / flush-finish | ✅ | `glGetError`, `glGetString`, `glFlush`, `glFinish`, `glEnable/Disable` (tracked caps), `glGetBooleanv/Integerv/Floatv/Doublev`, `glIsEnabled` |
| §6 Buffer objects | 🟡 | gen/bind/delete, `glBufferData`, `glBindBufferBase/Range`. Missing: `BufferSubData`, `BufferStorage` (immutable), `MapBuffer*`, `CopyBufferSubData`, `ClearBuffer*`, `InvalidateBuffer*`, `GetBufferSubData`, buffer queries |
| §7 Shaders / programs | 🟡 | create/source/compile/attach/link, `glGetShader*`, `glGetProgram*`, info logs, `glUseProgram`, `glGetAttribLocation`, `glGetUniformLocation`, full `glUniform*` (f/i/vec/mat4), GLSL version gate. Missing: `BindAttribLocation`, program **pipelines** (§7.4), **subroutines**, **compute** shaders, shader binaries |
| §8 Textures / samplers | 🟡 | gen/bind/delete, `glActiveTexture`, `glBindTexture` (per-unit), `glTexImage2D` (2D only), `glTexSubImage1D/2D/3D`, `glCopyTexImage1D/2D`, `glTexParameteri`/`f`/`fv`/`iv` (scalar + vector pnames), sampler objects, DSA texture bind (`glBindTextureUnit`/`glBindTextures`), texture-parameter queries (`glGetTexParameteriv`/`fv`). Missing: 1D/3D/cube/array/rect targets (texImage), full param coverage, `GetTexImage`, **multisample textures**, **buffer textures**, texture views, full **Named* (DSA object) surface** |
| §9 (program/pipeline — folded into §7.4) | ❌ | program pipeline objects not implemented |
| §10 Vertex spec / draw | 🟡 | VAO gen/bind/delete, `glVertexAttribPointer`, enable/disable attrib, `glDrawArrays`/`glDrawElements` (+ instanced). Missing: `DrawRangeElements`, `MultiDraw*`, **`DrawElementsBaseVertex`**, **indirect draw**, **primitive restart** (core `GL_PRIMITIVE_RESTART`), vertex attrib divisor, `VertexAttrib*` (except pointer), client array legacy |
| §11 (rasterization — points/lines/polygons) | 🟡 | `glPointSize` / `glLineWidth` / `glPolygonOffset` implemented (tracked scalar state, pushed only on change, GLES3-backed). Missing: `glPolygonMode` (GLES supports FILL only — honest-Unsupported candidate), provoking vertex, **multisample** raster state |
| §12 (fixed-function vertex / matrix / lighting / texgen) | 🚫 | entirely removed-in-core; not implemented (correct) |
| §13 Transform feedback | 🟡 | object lifecycle + begin/end/pause/resume + capability gate; forwards to backend. Missing: actual varying capture wiring to buffers, counter queries |
| §14 (rasterization per-fragment — depth/stencil/blend/scissor/viewport) | ✅ | `glDepthFunc/Mask/Range`, `glStencilFunc/Op/Mask`, `glBlendFunc(/Separate)`, `glBlendEquation(/Separate)`, `glBlendColor`, `glViewport`, `glScissor` (box), scissor test. Missing: **`glSampleCoverage`**, **`glMinSampleShading`**, `glPolygonOffset` |
| §15/§16 (per-fragment ops / whole framebuffer) | 🟡 | `glClear`(+values), `glReadPixels`, color/depth clear, `glDrawBuffers`/`glReadBuffer` (tracked state, pushed on flush). Missing: **`glBlitFramebuffer`** (and `BlitNamedFramebuffer`), `glInvalidateFramebuffer`, `glColorMask`/`glDepthMask` present, `glStencilMask` present; **logic op**, **sRGB/alpha-to-coverage**, **`glClampColor`** |
| §17 (fragment op details — alpha test, dither, logical op) | ❌ | alpha test removed-in-core; logical op / dither not implemented |
| §18 (pixels: ReadPixels done; Copy/DrawPixels removed-compat) | 🟡 | `glReadPixels` implemented; `glPixelStorei` implemented |
| §4 / §19 Sync objects & fences | ❌ | `glFenceSync`, `glClientWaitSync`, `glWaitSync`, `glDeleteSync`, `glIsSync`, `glGetSynciv` — not implemented |
| §4 / §20 Query objects (occlusion, timer, pipeline, primitiv
ive) | ❌ | `glGenQueries`, `glBeginQuery`/`BeginQueryIndexed`, `glEndQuery`, `glGetQuery*` — not implemented |
| §21 (evaluators / selection / feedback / display lists / hints) | 🚫 | removed-in-core; not implemented (correct) |
| §22 State queries (non-generic) | 🟡 | generic `glGet*` done; many specific `glGet*` (buffer params, internalformat, named-object params, shader interface queries like `glGetActiveUniform`, `glGetAttribLocation` done) not yet exposed |
| Shader stages | 🚫 | **Geometry, Tessellation, Compute** honestly **Unsupported** (no entry points; capability-gated). Only vertex + fragment stages translate (desktop→GLSL ES via glslang + SPIRV-Cross). |

## The 100 covered core command prototypes

ActiveTexture, AttachShader, BeginTransformFeedback, BindBuffer,
BindBufferBase, BindBufferRange, BufferStorage, BufferSubData,
BindFramebuffer, BindRenderbuffer,
CopyTexImage1D, CopyTexImage2D,
BindSampler, BindTexture, BindTextureUnit, BindTextures, BindTransformFeedback,
BindVertexArray, BlendColor, BlendEquation, BlendEquationSeparate, BlendFunc,
BlendFuncSeparate, BufferData, Clear, ClearColor, ClearDepth, ClearDepthf, CopyTexImage1D, CopyTexImage2D,
DrawBuffers, ReadBuffer,
CompileShader, CopyBufferSubData, CullFace, DeleteBuffers, DeleteFramebuffers,
DeleteProgram, DeleteRenderbuffers, DeleteSamplers, DeleteShader, DeleteTextures,
DeleteTransformFeedbacks, DeleteVertexArrays, DepthFunc, DepthMask, DepthRange,
DepthRangef, Disable, DisableVertexAttribArray, DrawArrays,
DrawArraysInstanced, DrawElements, DrawElementsInstanced, Enable,
EnableVertexAttribArray, EndTransformFeedback, Finish, Flush,
FramebufferRenderbuffer, FramebufferTexture2D, FrontFace, GenBuffers, LineWidth, PointSize, PolygonOffset,
GenFramebuffers, GenRenderbuffers, GenSamplers, GenTextures,
GenTransformFeedbacks, GenVertexArrays, GetBooleanv, GetBufferParameteriv,
GetDoublev, GetFloatv, GetIntegerv, GetProgramInfoLog, GetProgramiv,
GetShaderInfoLog, GetShaderiv, LinkProgram, MapBuffer, MapBufferRange,
PauseTransformFeedback, ReadPixels, RenderbufferStorage,
ResumeTransformFeedback, Scissor, ShaderSource, StencilFunc, StencilMask,
StencilOp, TexImage2D, TexSubImage1D, TexSubImage2D, TexSubImage3D, TexParameterf, TexParameterfv,
TexParameteriv, Uniform1f, Uniform1i, Uniform2f, Uniform2i, Uniform3f,
Uniform3i, Uniform4f, Uniform4i, UnmapBuffer, UseProgram, VertexAttribPointer, Viewport.

(Plus the type/vector variants already present in `gl_api`: `glUniform1fv`,
`glUniform1iv`, `glUniformMatrix4fv`, `glGetString`, `glGetError`,
`glGenTransformFeedback`, `glBindTransformFeedback`, `glDeleteTransformFeedback`,
`glGenSampler`, `glDeleteSampler`, `glIsSampler`, `glGenTextures` family, etc.)

## Major unimplemented core areas (priority order for next steps)

1. **Buffer object completeness** — `BufferSubData`, `BufferStorage`
    (immutable), `MapBuffer*`/`MapBufferRange`/`UnmapBuffer`, `CopyBufferSubData`,
    and `glGetBufferParameteriv` are implemented (frontend owns an authoritative
    CPU data store mirrored to the backend). Remaining: `ClearBuffer*`,
    `InvalidateBuffer*`, `GetBufferSubData`. (§6)
 2. **Texture completeness** — all non-2D targets, `TexParameterf`/vector
    pnames + full param coverage, `GetTexImage`, multisample & buffer textures.
    `TexSubImage1D/2D/3D` and `CopyTexImage1D/2D` are now implemented (frontend
    validation + backend virtualization, mock path tested). (§8)
3. **Full DSA surface** — `gl*Texture*`, `gl*Named*` (storage, subimage,
   parameterc, framebuffer/rendererbuffer DSA, vertex-array DSA). (§2.1/§8/§9)
4. **Queries & sync** — occlusion/timer/pipeline queries, sync fences. (§4/§19/§20)
5. **Whole-framebuffer ops** — `DrawBuffers`, `ReadBuffer`, `BlitFramebuffer`,
   `InvalidateFramebuffer`, logic-op/sRGB. (§15/§16/§17)
6. **Draw expansion** — primitive restart, `MultiDraw*`, `DrawElementsBaseVertex`,
   indirect draw, vertex attrib divisors. (§10)
7. **Compute / geometry / tessellation** — currently honestly Unsupported;
   requires the emulation roadmap in `docs/feature-matrix.md`. (§7/§13)
8. **Program pipelines & subroutines** — `glBindProgramPipeline`,
   `glActiveShaderProgram`, `glGetProgramResource*`. (§7.4)
9. **Rasterization controls** — `PolygonMode`, polygon offset, `PointSize`,
   `LineWidth`, multisample raster state. (§11)
10. **Specific `glGet*` coverage** — buffer/texture/internalformat/named-object
    parameter queries, program-interface reflection (`glGetActiveUniform`,
    `glGetActiveAttrib`, `glGetUniformBlockIndex`, …). (§22)

## Verdict

YAGLT implements a **foundational but early slice of the OpenGL 4.6 core
profile**: object lifecycle, a working vertex+fragment shader pipeline with
desktop→ES translation, uniforms, the core per-fragment/blend/depth/stencil/
viewport/scissor state, transform feedback scaffolding, sampler objects, DSA
texture binding, and basic draws — all with dispatch, validation, and tests.

It is **not** close to a complete 4.6 core implementation. Roughly **one in
five of the spec's declared core command prototypes** has a frontend entry
point (true entry-point coverage is lower once type/vector variants and the
honestly-Unsupported stages are counted). The largest gaps are buffer/texture
completeness, the full DSA surface, queries/sync, whole-framebuffer ops, draw
expansion, program pipelines, and the three unsupported shader stages.

Per project policy (`docs/feature-matrix.md`), the **compatibility profile**
(deprecated fixed-function API) remains intentionally unimplemented and is
gated behind a majority of core being done.
