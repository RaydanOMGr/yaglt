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

    virtual void blendFunc(uint32_t sfactor, uint32_t dfactor) = 0;
    virtual void blendEquation(uint32_t mode) = 0;

    virtual void depthFunc(uint32_t func) = 0;
    virtual void depthMask(bool enabled) = 0;

    virtual void stencilFunc(uint32_t func, int32_t ref, uint32_t mask) = 0;
    virtual void stencilOp(uint32_t sfail, uint32_t dpfail, uint32_t dppass) = 0;
    virtual void stencilMask(uint32_t mask) = 0;

    virtual void cullFace(uint32_t mode) = 0;
    virtual void frontFace(uint32_t mode) = 0;

    virtual void pixelStorei(uint32_t pname, int32_t param) = 0;

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
