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

    virtual void cullFace(uint32_t mode) = 0;
    virtual void frontFace(uint32_t mode) = 0;

    // Texture units (SPEC §2.1). `activeTexture` selects the unit (unit =
    // GL_TEXTURE0 + i); `bindTexture` binds `texture` (frontend object name) to
    // `target` on the currently selected unit. The backend converts the frontend
    // name to its native id, mirroring useProgram/bindVertexArray.
    virtual void activeTexture(uint32_t unit) = 0;
    virtual void bindTexture(uint32_t target, uint32_t texture) = 0;

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
};

} // namespace glcompat
