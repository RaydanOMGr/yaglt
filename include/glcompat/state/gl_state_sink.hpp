#pragma once

#include <cstdint>

namespace glcompat {

// Backend-facing sink for tracked pipeline state (SPEC §10). It deliberately
// uses plain integer types (not the frontend GL* typedefs/constants) so a
// backend can implement it even while it includes native GL headers (whose
// GL_* macros would otherwise collide with the frontend's lightweight GL
// constant layer). The frontend GLStateTracker converts its GLenum/GLObjectName
// values to these integers when pushing state.
class GLStateSink {
public:
    virtual ~GLStateSink() = default;

    virtual void enable(uint32_t cap) = 0;
    virtual void disable(uint32_t cap) = 0;

    // Indexed capabilities (glEnablei / glDisablei / glIsEnabledi, SPEC §10.3.1).
    // `cap` is a per-buffer/per-viewport capability (e.g. GL_BLEND,
    // GL_SCISSOR_TEST); `index` selects the buffer/viewport slot.
    virtual void enableIndexed(uint32_t cap, uint32_t index) = 0;
    virtual void disableIndexed(uint32_t cap, uint32_t index) = 0;

    virtual void useProgram(uint32_t prog) = 0;

    // Program pipeline (glBindProgramPipeline, SPEC §7.4). The frontend tracks
    // the bound pipeline independently of the single active program and forwards
    // it here so a backend that supports separable programs can install it.
    // Backends without separable-program support record the binding but cannot
    // consume it for drawing (reported honestly via the ProgramPipelines
    // capability).
    virtual void bindProgramPipeline(uint32_t pipeline) = 0;

    virtual void blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB,
                                    uint32_t srcAlpha, uint32_t dstAlpha) = 0;
    virtual void blendEquationSeparate(uint32_t modeRGB, uint32_t modeAlpha) = 0;
    virtual void blendColor(float r, float g, float b, float a) = 0;

    // Indexed blending (SPEC §15.3 / §17.3.4). `buf` selects the draw-buffer
    // slot; buffer 0 is equivalent to the non-indexed blend setters. Backends
    // without per-buffer blend (e.g. GLES pre-3.2) may fall back to the
    // single-buffer call for buf == 0 and record the rest without applying it.
    virtual void blendFuncSeparatei(uint32_t buf, uint32_t srcRGB,
                                    uint32_t dstRGB, uint32_t srcAlpha,
                                    uint32_t dstAlpha) = 0;
    virtual void blendEquationSeparatei(uint32_t buf, uint32_t modeRGB,
                                        uint32_t modeAlpha) = 0;

    virtual void depthFunc(uint32_t func) = 0;
    virtual void depthMask(bool enabled) = 0;
    virtual void depthRange(double nearVal, double farVal) = 0;

    virtual void stencilFunc(uint32_t func, int32_t ref, uint32_t mask) = 0;
    virtual void stencilOp(uint32_t sfail, uint32_t dpfail, uint32_t dppass) = 0;
    virtual void stencilMask(uint32_t mask) = 0;
    // Per-face variants (glStencil*Separate, SPEC §17.3.3): face is
    // GL_FRONT (0x0404) or GL_BACK (0x0405).
    virtual void stencilFuncSeparate(uint32_t face, uint32_t func, int32_t ref,
                                     uint32_t mask) = 0;
    virtual void stencilOpSeparate(uint32_t face, uint32_t sfail, uint32_t dpfail,
                                   uint32_t dppass) = 0;
    virtual void stencilMaskSeparate(uint32_t face, uint32_t mask) = 0;

    // Color write mask (glColorMask, SPEC §17.3.6). Each channel is an
    // independent boolean pushed only when the set of masked channels changes.
    virtual void colorMask(bool r, bool g, bool b, bool a) = 0;

    // Sample coverage (glSampleCoverage, SPEC §17.3.6 multisample). Pushed only
    // when the value or invert flag changes.
    virtual void sampleCoverage(float value, bool invert) = 0;

    virtual void cullFace(uint32_t mode) = 0;
    virtual void frontFace(uint32_t mode) = 0;

    // Rasterization scalar state (SPEC §11). Independent values pushed only when
    // the relevant one changed (the frontend compares each field individually).
    virtual void pointSize(float size) = 0;
    virtual void lineWidth(float width) = 0;
    virtual void polygonOffset(float factor, float units) = 0;

    // Point parameters (SPEC §10.2, glPointParameter*). Pushed only when any
    // field changed; backends without point-parameter support (e.g. GLES) record
    // without a native call, matching the honest capability.
    virtual void pointParameters(float sizeMin, float sizeMax, float fadeThreshold,
                                 GLenum spriteCoordOrigin) = 0;

    // Clip control (SPEC §12.1, glClipControl). Pushed only when the origin or
    // depth mode changed; backends without a native glClipControl (e.g. GLES)
    // record without a native call, matching the honest capability.
    virtual void clipControl(GLenum origin, GLenum depth) = 0;

    // Polygon render mode (glPolygonMode, SPEC §11.1). `front`/`back` are the
    // GL_POINT/GL_LINE/GL_FILL modes pushed for the respective sides. Backends
    // without polygon-mode support (e.g. GLES) record but do not apply it.
    virtual void polygonMode(uint32_t front, uint32_t back) = 0;

    // Multisample raster state (SPEC §11.5). `sampleMaski` pushes one mask word
    // (index `maskNumber`); `minSampleShading` pushes the [0,1] fraction. GLES
    // has no equivalent and records these without applying them.
    virtual void sampleMaski(uint32_t maskNumber, uint32_t mask) = 0;
    virtual void minSampleShading(float value) = 0;

    // Provoking vertex convention (SPEC §11, glProvokingVertex). `mode` is
    // GL_FIRST_VERTEX_CONVENTION / GL_LAST_VERTEX_CONVENTION. GLES has no
    // equivalent and records it without applying it.
    virtual void provokingVertex(uint32_t mode) = 0;

    // Texture units (SPEC §2.1). `activeTexture` selects the unit (unit =
    // GL_TEXTURE0 + i); `bindTexture` binds `texture` (frontend object name) to
    // `target` on the currently selected unit. The backend converts the frontend
    // name to its native id, mirroring useProgram/bindVertexArray.
    virtual void activeTexture(uint32_t unit) = 0;
    virtual void bindTexture(uint32_t target, uint32_t texture) = 0;

    // Sampler objects (SPEC §8.2). Binds the sampler `sampler` (frontend object
    // name, resolved to the native id by the backend) to texture unit `unit`
    // (the zero-based unit index, not GL_TEXTURE0+unit). Pushed only when the
    // binding changes (SPEC §10).
    virtual void bindSampler(uint32_t unit, uint32_t sampler) = 0;

    virtual void pixelStorei(uint32_t pname, int32_t param) = 0;

    // Viewport (glViewport, SPEC §10) and scissor box (glScissor). The scissor
    // *test* itself is a capability (GL_SCISSOR_TEST) pushed via enable/disable.
    virtual void setViewport(int32_t x, int32_t y, int32_t w, int32_t h) = 0;
    virtual void setScissor(int32_t x, int32_t y, int32_t w, int32_t h) = 0;

    // Indexed viewport/scissor (glViewportIndexedf/fv, glScissorIndexed/v,
    // SPEC §10.3.1). `index` selects the viewport/scissor slot.
    virtual void setViewportIndexed(uint32_t index, int32_t x, int32_t y,
                                    int32_t w, int32_t h) = 0;
    virtual void setScissorIndexed(uint32_t index, int32_t x, int32_t y,
                                   int32_t w, int32_t h) = 0;

    // Clear values (glClearColor / glClearDepth, SPEC §2.1). These are GL state
    // pushed to the backend before a clear command so the driver clears with the
    // correct color/depth. Pushed only when the value changed (SPEC §10).
    virtual void clearColor(float r, float g, float b, float a) = 0;
    virtual void clearDepth(double d) = 0;

    // Whole-framebuffer buffer selection (SPEC §15 / §16). `drawBuffers` selects
    // the draw buffers for the currently bound framebuffer; `readBuffer` selects
    // its read buffer. Pushed only when the selection changed (SPEC §10).
    virtual void drawBuffers(int32_t n, const uint32_t* bufs) = 0;
    virtual void readBuffer(uint32_t buf) = 0;

    // Color logic op (SPEC §17.3.4, glLogicOp). Pushed only when the mode changes
    // (SPEC §10); the driver applies it only while GL_COLOR_LOGIC_OP is enabled.
    virtual void logicOp(uint32_t mode) = 0;

    // Color clamping (SPEC §15.2.3, glClampColor). `target` is the clamp target
    // (GL_CLAMP_READ_COLOR in core 4.6); `mode` is GL_TRUE / GL_FALSE /
    // GL_FIXED_ONLY. GLES has no equivalent and records this without applying it.
    virtual void clampColor(uint32_t target, uint32_t mode) = 0;

    // Indexed buffer bindings (UBO / SSBO / transform feedback, SPEC §8).
    // `target` is the indexed buffer target, `index` the binding point.
    virtual void bindBufferBase(uint32_t target, uint32_t index,
                                uint32_t buffer) = 0;
    virtual void bindBufferRange(uint32_t target, uint32_t index,
                                 uint32_t buffer, intptr_t offset,
                                 intptr_t size) = 0;

    // Vertex array + attribute setup (SPEC §2.1). `vao` is the frontend VAO
    // name; `index` the attribute location.
    virtual void bindVertexArray(uint32_t vao) = 0;
    virtual void enableVertexAttribArray(uint32_t index) = 0;
    virtual void disableVertexAttribArray(uint32_t index) = 0;
    // Bind `buffer` (frontend name, resolved to the native id by the backend)
    // to `target` before an attribute pointer is issued. On GLES the attribute's
    // buffer binding is captured from the bound ARRAY_BUFFER at gl*VertexAttrib
    // Pointer time, so the flush must bind the attribute's ARRAY_BUFFER first
    // (SPEC §2.1).
    virtual void bindBuffer(uint32_t target, uint32_t buffer) = 0;
    virtual void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                     bool normalized, int32_t stride,
                                     intptr_t offset) = 0;
    // Vertex attribute divisor (SPEC §10, glVertexAttribDivisor). Pushed only when
    // the divisor is non-zero (the GL default is 0); instanced draws use it to
    // step the attribute once per `divisor` instances.
    virtual void vertexAttribDivisor(uint32_t index, uint32_t divisor) = 0;

    // Primitive restart index (SPEC §10.4, glPrimitiveRestartIndex). Pushed only
    // when the index changes (SPEC §10). The GL_PRIMITIVE_RESTART capability that
    // activates it is pushed via enable/disable.
    virtual void primitiveRestart(uint32_t index) = 0;
    // Quality hint (SPEC §21.1.1, glHint). Hints are non-binding; the backend may
    // ignore them, but the frontend records the requested target/mode and pushes
    // it on flush so a real driver receives the request.
    virtual void hint(uint32_t target, uint32_t mode) = 0;

    // Conditional rendering (SPEC §10.11, glBeginConditionalRender /
    // glEndConditionalRender). `id` is the frontend query-object name the region
    // is predicated on; `mode` is the GL_QUERY_* predicate mode. The backend
    // resolves the query name to its native id and installs the predicate so
    // subsequent draws are suppressed when the query fails. Backends without
    // conditional-render support record the region but cannot suppress draws
    // (reported honestly via the ConditionalRendering capability); the frontend
    // never calls these when the feature is Unsupported.
    virtual void beginConditionalRender(uint32_t id, uint32_t mode) = 0;
    virtual void endConditionalRender() = 0;
};

} // namespace glcompat
