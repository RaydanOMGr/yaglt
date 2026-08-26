#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/core/log.hpp"
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

    bool initialize() override {
        initialized_ = true;
        log(LogCategory::Backend, LogLevel::Info)
            << "selected backend: Mock (headless test backend)";
        capabilities_.report();
        return true;
    }
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
    int blendColorCalls = 0;
    GLenum lastSrcRGB = 0, lastDstRGB = 0, lastSrcAlpha = 0, lastDstAlpha = 0;
    GLenum lastEqRGB = 0, lastEqAlpha = 0;
    float lastBlendR = 0.0f, lastBlendG = 0.0f, lastBlendB = 0.0f,
          lastBlendA = 0.0f;
    int depthFuncCalls = 0;
    int depthMaskCalls = 0;
    int depthRangeCalls = 0;
    double lastDepthNear = 0.0, lastDepthFar = 1.0;
    int stencilFuncCalls = 0;
    int stencilOpCalls = 0;
    int stencilMaskCalls = 0;
    int cullFaceCalls = 0;
    int frontFaceCalls = 0;
    int pointSizeCalls = 0;
    int lineWidthCalls = 0;
    int polygonOffsetCalls = 0;
    float lastPointSize = 1.0f;
    float lastLineWidth = 1.0f;
    float lastPolygonOffsetFactor = 0.0f;
    float lastPolygonOffsetUnits = 0.0f;
    int pixelStoreiCalls = 0;

    int viewportCalls = 0;
    int32_t lastViewportX = 0, lastViewportY = 0;
    int32_t lastViewportW = 0, lastViewportH = 0;
    int scissorCalls = 0;
    int32_t lastScissorX = 0, lastScissorY = 0;
    int32_t lastScissorW = 0, lastScissorH = 0;

    int clearColorCalls = 0;
    float lastClearR = 0.0f, lastClearG = 0.0f, lastClearB = 0.0f,
          lastClearA = 0.0f;
    int clearDepthCalls = 0;
    double lastClearDepth = 1.0;
    int clearCalls = 0;
    uint32_t lastClearMask = 0;
    int flushCalls = 0;
    int finishCalls = 0;
    int readPixelsCalls = 0;
    int32_t lastReadX = 0, lastReadY = 0, lastReadW = 0, lastReadH = 0;
    uint32_t lastReadFormat = 0, lastReadType = 0;
    void* lastReadPixels = nullptr;

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
    void blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB, uint32_t srcAlpha,
                           uint32_t dstAlpha) override {
        ++blendFuncCalls;
        lastSrcRGB = srcRGB;
        lastDstRGB = dstRGB;
        lastSrcAlpha = srcAlpha;
        lastDstAlpha = dstAlpha;
    }
    void blendEquationSeparate(uint32_t modeRGB, uint32_t modeAlpha) override {
        ++blendEquationCalls;
        lastEqRGB = modeRGB;
        lastEqAlpha = modeAlpha;
    }
    void blendColor(float r, float g, float b, float a) override {
        ++blendColorCalls;
        lastBlendR = r;
        lastBlendG = g;
        lastBlendB = b;
        lastBlendA = a;
    }
    void depthFunc(GLenum) override { ++depthFuncCalls; }
    void depthMask(bool) override { ++depthMaskCalls; }
    void depthRange(double n, double f) override {
        ++depthRangeCalls;
        lastDepthNear = n;
        lastDepthFar = f;
    }
    void stencilFunc(GLenum, GLint, GLuint) override { ++stencilFuncCalls; }
    void stencilOp(GLenum, GLenum, GLenum) override { ++stencilOpCalls; }
    void stencilMask(GLuint) override { ++stencilMaskCalls; }
    void cullFace(GLenum) override { ++cullFaceCalls; }
    void frontFace(GLenum) override { ++frontFaceCalls; }
    void pointSize(float size) override {
        ++pointSizeCalls;
        lastPointSize = size;
    }
    void lineWidth(float width) override {
        ++lineWidthCalls;
        lastLineWidth = width;
    }
    void polygonOffset(float factor, float units) override {
        ++polygonOffsetCalls;
        lastPolygonOffsetFactor = factor;
        lastPolygonOffsetUnits = units;
    }
    void pixelStorei(GLenum, GLint) override { ++pixelStoreiCalls; }
    void setViewport(int32_t x, int32_t y, int32_t w, int32_t h) override {
        ++viewportCalls;
        lastViewportX = x;
        lastViewportY = y;
        lastViewportW = w;
        lastViewportH = h;
    }
    void setScissor(int32_t x, int32_t y, int32_t w, int32_t h) override {
        ++scissorCalls;
        lastScissorX = x;
        lastScissorY = y;
        lastScissorW = w;
        lastScissorH = h;
    }
    void clearColor(float r, float g, float b, float a) override {
        ++clearColorCalls;
        lastClearR = r;
        lastClearG = g;
        lastClearB = b;
        lastClearA = a;
    }
    void clearDepth(double d) override {
        ++clearDepthCalls;
        lastClearDepth = d;
    }

    int drawBuffersCalls = 0;
    int32_t lastDrawBuffersN = 0;
    std::vector<uint32_t> lastDrawBuffers;
    int readBufferCalls = 0;
    uint32_t lastReadBuffer = 0;
    void drawBuffers(int32_t n, const uint32_t* bufs) override {
        ++drawBuffersCalls;
        lastDrawBuffersN = n;
        lastDrawBuffers.assign(bufs, bufs + n);
    }
    void readBuffer(uint32_t buf) override {
        ++readBufferCalls;
        lastReadBuffer = buf;
    }

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

    // Texture units (SPEC §2.1). Recorded so tests can assert the frontend
    // pushes the active unit and per-unit bindings only when they change.
    int activeTextureCalls = 0;
    uint32_t lastActiveTexture = 0;
    int bindTextureCalls = 0;
    uint32_t lastTexBindTarget = 0;
    GLObjectName lastBindTexture = 0;
    void activeTexture(uint32_t unit) override {
        ++activeTextureCalls;
        lastActiveTexture = unit;
    }
    void bindTexture(uint32_t target, uint32_t texture) override {
        ++bindTextureCalls;
        lastTexBindTarget = target;
        lastBindTexture = texture;
    }

    // Sampler objects (SPEC §8.2). Recorded so tests can assert the frontend
    // pushes a sampler binding only when it changes.
    int bindSamplerCalls = 0;
    uint32_t lastBindSamplerUnit = 0;
    GLObjectName lastBindSampler = 0;
    void bindSampler(uint32_t unit, uint32_t sampler) override {
        ++bindSamplerCalls;
        lastBindSamplerUnit = unit;
        lastBindSampler = sampler;
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

    void clear(uint32_t mask) override {
        ++clearCalls;
        lastClearMask = mask;
    }
    void flush() override { ++flushCalls; }
    void finish() override { ++finishCalls; }
    void readPixels(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t format,
                    uint32_t type, void* pixels) override {
        ++readPixelsCalls;
        lastReadX = x;
        lastReadY = y;
        lastReadW = w;
        lastReadH = h;
        lastReadFormat = format;
        lastReadType = type;
        lastReadPixels = pixels;
    }

private:
    CapabilityTable capabilities_;
    MockPlatformCapabilities platform_;
    MockResourceFactory factory_;
    MockShaderCompiler compiler_;
    bool initialized_ = false;
};

} // namespace glcompat
