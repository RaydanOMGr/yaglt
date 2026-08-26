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

    virtual void useProgram(uint32_t prog) = 0;

    virtual void blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB,
                                   uint32_t srcAlpha, uint32_t dstAlpha) = 0;
    virtual void blendEquationSeparate(uint32_t modeRGB, uint32_t modeAlpha) = 0;
    virtual void blendColor(float r, float g, float b, float a) = 0;

    virtual void depthFunc(uint32_t func) = 0;
    virtual void depthMask(bool enabled) = 0;
    virtual void depthRange(double nearVal, double farVal) = 0;

    virtual void stencilFunc(uint32_t func, int32_t ref, uint32_t mask) = 0;
    virtual void stencilOp(uint32_t sfail, uint32_t dpfail, uint32_t dppass) = 0;
    virtual void stencilMask(uint32_t mask) = 0;

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
    virtual void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                     bool normalized, int32_t stride,
                                     intptr_t offset) = 0;

    // Primitive restart index (SPEC §10.4, glPrimitiveRestartIndex). Pushed only
    // when the index changes (SPEC §10). The GL_PRIMITIVE_RESTART capability that
    // activates it is pushed via enable/disable.
    virtual void primitiveRestart(uint32_t index) = 0;
};

} // namespace glcompat
