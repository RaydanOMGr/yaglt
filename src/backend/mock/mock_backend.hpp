#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/state/gl_state.hpp"
#include "mock_capabilities.hpp"
#include "mock_factory.hpp"
#include "mock_shader_compiler.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace glcompat {

// In-memory backend for tests and headless development. No native GL context
// is created; resources are tracking objects. Exercises the frontend<->backend
// abstraction without a real driver.
//
// It also implements GLStateSink so the frontend can push tracked pipeline
// state through it; every push is recorded so tests can assert that redundant
// native calls are skipped (SPEC §10).
class MockBackend : public IGraphicsBackend, public GLStateSink {
public:
    explicit MockBackend(PlatformOs os = PlatformOs::Linux, int sdk = 0)
        : platform_(os, sdk) {
        populateMockCapabilities(capabilities_);
    }

    BackendApi api() const override { return BackendApi::Mock; }
    const ICapabilities& capabilities() const override { return capabilities_; }
    const IPlatformCapabilities& platform() const override { return platform_; }

    // Test helper: override a capability after construction.
    void setCapability(Feature f, FeatureSupport s) { capabilities_.set(f, s); }

    IResourceFactory& resourceFactory() override { return factory_; }
    IShaderCompiler& shaderCompiler() override { return compiler_; }

    bool initialize() override { initialized_ = true; return true; }
    void shutdown() override { initialized_ = false; }

    GLStateSink* stateSink() override { return this; }

    // Record frontend name -> native id registrations (observable in tests).
    std::unordered_map<uint32_t, uint32_t> nativeMap_;
    void bindNativeObject(uint32_t name, uint32_t nativeId) override {
        nativeMap_[name] = nativeId;
    }

    std::string describe() const override {
        return "MockBackend(" + platform_.describe() + ")";
    }

    // --- GLStateSink recording (observable in tests) ---
    int enableCalls = 0;
    int disableCalls = 0;
    GLenum lastEnableCap = 0;
    GLenum lastDisableCap = 0;
    std::vector<std::pair<GLenum, bool>> capCalls;

    int useProgramCalls = 0;
    GLObjectName lastProgram = 0;

    int blendFuncCalls = 0;
    int blendEquationCalls = 0;
    int depthFuncCalls = 0;
    int depthMaskCalls = 0;
    int stencilFuncCalls = 0;
    int stencilOpCalls = 0;
    int stencilMaskCalls = 0;
    int cullFaceCalls = 0;
    int frontFaceCalls = 0;
    int pixelStoreiCalls = 0;

    void enable(GLenum cap) override {
        ++enableCalls;
        lastEnableCap = cap;
        capCalls.emplace_back(cap, true);
    }
    void disable(GLenum cap) override {
        ++disableCalls;
        lastDisableCap = cap;
        capCalls.emplace_back(cap, false);
    }
    void useProgram(GLObjectName prog) override {
        ++useProgramCalls;
        lastProgram = prog;
    }
    void blendFunc(GLenum, GLenum) override { ++blendFuncCalls; }
    void blendEquation(GLenum) override { ++blendEquationCalls; }
    void depthFunc(GLenum) override { ++depthFuncCalls; }
    void depthMask(bool) override { ++depthMaskCalls; }
    void stencilFunc(GLenum, GLint, GLuint) override { ++stencilFuncCalls; }
    void stencilOp(GLenum, GLenum, GLenum) override { ++stencilOpCalls; }
    void stencilMask(GLuint) override { ++stencilMaskCalls; }
    void cullFace(GLenum) override { ++cullFaceCalls; }
    void frontFace(GLenum) override { ++frontFaceCalls; }
    void pixelStorei(GLenum, GLint) override { ++pixelStoreiCalls; }

    int bindBufferBaseCalls = 0;
    int bindBufferRangeCalls = 0;
    uint32_t lastBindTarget = 0;
    uint32_t lastBindIndex = 0;
    GLObjectName lastBindBuffer = 0;
    void bindBufferBase(uint32_t target, uint32_t index, uint32_t buffer) override {
        ++bindBufferBaseCalls;
        lastBindTarget = target;
        lastBindIndex = index;
        lastBindBuffer = buffer;
    }
    void bindBufferRange(uint32_t target, uint32_t index, uint32_t buffer,
                          intptr_t, intptr_t) override {
        ++bindBufferRangeCalls;
        lastBindTarget = target;
        lastBindIndex = index;
        lastBindBuffer = buffer;
    }

    int bindVertexArrayCalls = 0;
    uint32_t lastBindVertexArray = 0;
    int enableVertexAttribArrayCalls = 0;
    int disableVertexAttribArrayCalls = 0;
    int vertexAttribPointerCalls = 0;
    uint32_t lastAttribIndex = 0;
    int32_t lastAttribSize = 0;
    uint32_t lastAttribType = 0;
    bool lastAttribNormalized = false;
    int32_t lastAttribStride = 0;
    intptr_t lastAttribOffset = 0;
    void bindVertexArray(uint32_t vao) override {
        ++bindVertexArrayCalls;
        lastBindVertexArray = vao;
    }
    void enableVertexAttribArray(uint32_t index) override {
        ++enableVertexAttribArrayCalls;
        lastAttribIndex = index;
    }
    void disableVertexAttribArray(uint32_t index) override {
        ++disableVertexAttribArrayCalls;
        lastAttribIndex = index;
    }
    void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                             bool normalized, int32_t stride,
                             intptr_t offset) override {
        ++vertexAttribPointerCalls;
        lastAttribIndex = index;
        lastAttribSize = size;
        lastAttribType = type;
        lastAttribNormalized = normalized;
        lastAttribStride = stride;
        lastAttribOffset = offset;
    }

    // --- Draw command recording (observable in tests) ---
    int drawArraysCalls = 0;
    int drawElementsCalls = 0;
    int drawArraysInstancedCalls = 0;
    int drawElementsInstancedCalls = 0;
    uint32_t lastDrawMode = 0;
    int32_t lastDrawFirst = 0;
    int32_t lastDrawCount = 0;
    uint32_t lastDrawType = 0;
    intptr_t lastDrawIndices = 0;
    int32_t lastDrawPrimcount = 0;
    void drawArrays(uint32_t mode, int32_t first, int32_t count) override {
        ++drawArraysCalls;
        lastDrawMode = mode;
        lastDrawFirst = first;
        lastDrawCount = count;
    }
    void drawElements(uint32_t mode, int32_t count, uint32_t type,
                      intptr_t indices) override {
        ++drawElementsCalls;
        lastDrawMode = mode;
        lastDrawCount = count;
        lastDrawType = type;
        lastDrawIndices = indices;
    }
    void drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                             int32_t primcount) override {
        ++drawArraysInstancedCalls;
        lastDrawMode = mode;
        lastDrawFirst = first;
        lastDrawCount = count;
        lastDrawPrimcount = primcount;
    }
    void drawElementsInstanced(uint32_t mode, int32_t count, uint32_t type,
                               intptr_t indices, int32_t primcount) override {
        ++drawElementsInstancedCalls;
        lastDrawMode = mode;
        lastDrawCount = count;
        lastDrawType = type;
        lastDrawIndices = indices;
        lastDrawPrimcount = primcount;
    }

private:
    CapabilityTable capabilities_;
    MockPlatformCapabilities platform_;
    MockResourceFactory factory_;
    MockShaderCompiler compiler_;
    bool initialized_ = false;
};

} // namespace glcompat
