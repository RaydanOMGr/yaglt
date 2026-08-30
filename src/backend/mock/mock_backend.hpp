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

    void bindFramebuffer(uint32_t target, uint32_t framebuffer) override {
        lastBindFramebufferTarget = target;
        lastBindFramebuffer = framebuffer;
        ++bindFramebufferCalls;
    }

    std::string describe() const override {
        return "MockBackend(" + platform_.describe() + ")";
    }

    // Driver-level string query. The mock has no native driver, so it returns
    // fixed placeholder strings keyed by the standard GL query names.
    const char* getBackingGlString(uint32_t name) override {
        switch (name) {
        case GL_VENDOR: return "MockVendor";
        case GL_RENDERER: return "MockRenderer";
        case GL_VERSION: return "Mock 1.0";
        case GL_SHADING_LANGUAGE_VERSION: return "Mock GLSL 1.0";
        default: return nullptr;
        }
    }

    // --- GLStateSink recording (observable in tests) ---
    int enableCalls = 0;
    int disableCalls = 0;
    GLenum lastEnableCap = 0;
    GLenum lastDisableCap = 0;
    std::vector<std::pair<GLenum, bool>> capCalls;

    // glEnablei/glDisablei recording (SPEC §10.3.1).
    int enableIndexedCalls = 0;
    int disableIndexedCalls = 0;
    GLenum lastEnableIndexedCap = 0;
    uint32_t lastEnableIndexedIndex = 0;
    GLenum lastDisableIndexedCap = 0;
    uint32_t lastDisableIndexedIndex = 0;
    std::vector<std::tuple<GLenum, uint32_t, bool>> indexedCapCalls;

    // glHint recording (SPEC §21.1.1).
    int hintCalls = 0;
    GLenum lastHintTarget = 0;
    GLenum lastHintMode = 0;
    std::vector<std::pair<GLenum, GLenum>> hintCallsList;

    // Recording helpers for vertex-array flush assertions (DSA tests need the
    // full per-attribute sequence, not just the last call).
    struct VertexAttribPtrRecord {
        uint32_t index = 0;
        int32_t size = 0;
        uint32_t type = 0;
        bool normalized = false;
        int32_t stride = 0;
        intptr_t offset = 0;
    };
    struct BufferBindRecord {
        uint32_t target = 0;
        uint32_t buffer = 0;
    };
    struct VertexAttribDivisorRecord {
        uint32_t index = 0;
        uint32_t divisor = 0;
    };
    std::vector<uint32_t> enableVertexAttribOrder;
    std::vector<uint32_t> disableVertexAttribOrder;
    std::vector<VertexAttribPtrRecord> vertexAttribPtrs;
    std::vector<BufferBindRecord> bufferBinds;
    std::vector<VertexAttribDivisorRecord> attribDivisors;

    int useProgramCalls = 0;
    GLObjectName lastProgram = 0;

    int bindProgramPipelineCalls = 0;
    GLObjectName lastProgramPipeline = 0;

    int blendFuncCalls = 0;
    int blendEquationCalls = 0;
    int blendColorCalls = 0;
    GLenum lastSrcRGB = 0, lastDstRGB = 0, lastSrcAlpha = 0, lastDstAlpha = 0;
    GLenum lastEqRGB = 0, lastEqAlpha = 0;
    int blendFunciCalls = 0;
    int blendEquationiCalls = 0;
    uint32_t lastBlendFunciBuf = 0;
    uint32_t lastBlendEquationiBuf = 0;
    GLenum lastSrcRGBi = 0, lastDstRGBi = 0, lastSrcAlphai = 0,
           lastDstAlphai = 0;
    GLenum lastEqRGBi = 0, lastEqAlphai = 0;
    float lastBlendR = 0.0f, lastBlendG = 0.0f, lastBlendB = 0.0f,
          lastBlendA = 0.0f;
    int depthFuncCalls = 0;
    int depthMaskCalls = 0;
    int depthRangeCalls = 0;
    double lastDepthNear = 0.0, lastDepthFar = 1.0;
    int depthRangeIndexedCalls = 0;
    uint32_t lastDepthRangeIndexedIndex = 0;
    double lastDepthNearIndexed = 0.0, lastDepthFarIndexed = 1.0;
    int stencilFuncCalls = 0;
    int stencilOpCalls = 0;
    int stencilMaskCalls = 0;
    int stencilFuncSeparateCalls = 0;
    int stencilOpSeparateCalls = 0;
    int stencilMaskSeparateCalls = 0;
    uint32_t lastStencilFace = 0;
    int colorMaskCalls = 0;
    bool lastColorMaskR = true, lastColorMaskG = true, lastColorMaskB = true,
         lastColorMaskA = true;
    int colorMaskiCalls = 0;
    uint32_t lastColorMaskiBuf = 0;
    bool lastColorMaskiR = true, lastColorMaskiG = true, lastColorMaskiB = true,
         lastColorMaskiA = true;
    int sampleCoverageCalls = 0;
    float lastSampleCoverageValue = 1.0f;
    bool lastSampleCoverageInvert = false;
    int primitiveRestartCalls = 0;
    uint32_t lastPrimitiveRestartIndex = 0;
    int cullFaceCalls = 0;
    int frontFaceCalls = 0;
    int pointSizeCalls = 0;
    int lineWidthCalls = 0;
    int polygonOffsetCalls = 0;
    int pointParametersCalls = 0;
    float lastPointSize = 1.0f;
    float lastLineWidth = 1.0f;
    float lastPolygonOffsetFactor = 0.0f;
    float lastPolygonOffsetUnits = 0.0f;
    float lastPolygonOffsetClamp = 0.0f;
    float lastPointSizeMin = 0.0f;
    float lastPointSizeMax = 1.0f;
    float lastPointFadeThreshold = 0.0f;
    GLenum lastSpriteCoordOrigin = 0x8CA2; // GL_UPPER_LEFT
    int patchParameterCalls = 0;
    uint32_t lastPatchPname = 0;
    uint32_t lastPatchVertices = 3;
    std::array<float, 4> lastPatchOuterLevel = {1.0f, 1.0f, 1.0f, 1.0f};
    std::array<float, 2> lastPatchInnerLevel = {1.0f, 1.0f};
    int clipControlCalls = 0;
    GLenum lastClipOrigin = 0x8CA1; // GL_LOWER_LEFT
    GLenum lastClipDepthMode = 0x8E27; // GL_NEGATIVE_ONE_TO_ONE
    int polygonModeCalls = 0;
    uint32_t lastPolygonModeFront = 0x1B02; // GL_FILL
    uint32_t lastPolygonModeBack = 0x1B02;  // GL_FILL
    int sampleMaskiCalls = 0;
    uint32_t lastSampleMaskNumber = 0;
    uint32_t lastSampleMask = 0;
    int minSampleShadingCalls = 0;
    float lastMinSampleShading = 0.0f;
    int provokingVertexCalls = 0;
    uint32_t lastProvokingVertexMode = 0x8E4E; // GL_LAST_VERTEX_CONVENTION
    int clampColorCalls = 0;
    uint32_t lastClampColorTarget = 0x891C; // GL_CLAMP_READ_COLOR
    uint32_t lastClampColorMode = 0x891D;    // GL_FIXED_ONLY
    int pixelStoreiCalls = 0;

    int viewportCalls = 0;
    int32_t lastViewportX = 0, lastViewportY = 0;
    int32_t lastViewportW = 0, lastViewportH = 0;
    int scissorCalls = 0;
    int32_t lastScissorX = 0, lastScissorY = 0;
    int32_t lastScissorW = 0, lastScissorH = 0;
    int viewportIndexedCalls = 0;
    uint32_t lastViewportIndexed = 0;
    int32_t lastViewportIndexedX = 0, lastViewportIndexedY = 0;
    int32_t lastViewportIndexedW = 0, lastViewportIndexedH = 0;
    int scissorIndexedCalls = 0;
    uint32_t lastScissorIndexed = 0;
    int32_t lastScissorIndexedX = 0, lastScissorIndexedY = 0;
    int32_t lastScissorIndexedW = 0, lastScissorIndexedH = 0;

    int clearColorCalls = 0;
    float lastClearR = 0.0f, lastClearG = 0.0f, lastClearB = 0.0f,
          lastClearA = 0.0f;
    int clearDepthCalls = 0;
    double lastClearDepth = 1.0;
    int clearStencilCalls = 0;
    int lastClearStencil = 0;
    int clearCalls = 0;
    uint32_t lastClearMask = 0;
    int clearTexImageCalls = 0;
    uint32_t lastClearTexImageName = 0;
    int lastClearTexImageLevel = 0;
    uint32_t lastClearTexImageFormat = 0, lastClearTexImageType = 0;
    int clearTexSubImageCalls = 0;
    uint32_t lastClearTexSubImageName = 0;
    int lastClearTexSubImageLevel = 0;
    int lastClearTexSubImageX = 0, lastClearTexSubImageY = 0, lastClearTexSubImageZ = 0;
    int lastClearTexSubImageW = 0, lastClearTexSubImageH = 0, lastClearTexSubImageD = 0;
    uint32_t lastClearTexSubImageFormat = 0, lastClearTexSubImageType = 0;
    int copyImageSubDataCalls = 0;
    uint32_t lastCopySrcName = 0, lastCopySrcTarget = 0, lastCopyDstName = 0,
             lastCopyDstTarget = 0;
    int lastCopySrcLevel = 0, lastCopySrcX = 0, lastCopySrcY = 0, lastCopySrcZ = 0;
    int lastCopyDstLevel = 0, lastCopyDstX = 0, lastCopyDstY = 0, lastCopyDstZ = 0;
    int lastCopyWidth = 0, lastCopyHeight = 0, lastCopyDepth = 0;
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
    void enableIndexed(GLenum cap, uint32_t index) override {
        ++enableIndexedCalls;
        lastEnableIndexedCap = cap;
        lastEnableIndexedIndex = index;
        indexedCapCalls.emplace_back(cap, index, true);
    }
    void disableIndexed(GLenum cap, uint32_t index) override {
        ++disableIndexedCalls;
        lastDisableIndexedCap = cap;
        lastDisableIndexedIndex = index;
        indexedCapCalls.emplace_back(cap, index, false);
    }
    void useProgram(GLObjectName prog) override {
        ++useProgramCalls;
        lastProgram = prog;
    }
    void bindProgramPipeline(uint32_t pipeline) override {
        ++bindProgramPipelineCalls;
        lastProgramPipeline = pipeline;
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
    void blendFuncSeparatei(uint32_t buf, uint32_t srcRGB, uint32_t dstRGB,
                            uint32_t srcAlpha, uint32_t dstAlpha) override {
        ++blendFunciCalls;
        lastBlendFunciBuf = buf;
        lastSrcRGBi = srcRGB;
        lastDstRGBi = dstRGB;
        lastSrcAlphai = srcAlpha;
        lastDstAlphai = dstAlpha;
    }
    void blendEquationSeparatei(uint32_t buf, uint32_t modeRGB,
                                uint32_t modeAlpha) override {
        ++blendEquationiCalls;
        lastBlendEquationiBuf = buf;
        lastEqRGBi = modeRGB;
        lastEqAlphai = modeAlpha;
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
    void depthRangeIndexed(uint32_t index, double n, double f) override {
        ++depthRangeIndexedCalls;
        lastDepthRangeIndexedIndex = index;
        lastDepthNearIndexed = n;
        lastDepthFarIndexed = f;
    }
    void stencilFunc(GLenum, GLint, GLuint) override { ++stencilFuncCalls; }
    void stencilOp(GLenum, GLenum, GLenum) override { ++stencilOpCalls; }
    void stencilMask(GLuint) override { ++stencilMaskCalls; }
    void stencilFuncSeparate(GLenum face, GLenum, GLint, GLuint) override {
        ++stencilFuncSeparateCalls;
        lastStencilFace = face;
    }
    void stencilOpSeparate(GLenum face, GLenum, GLenum, GLenum) override {
        ++stencilOpSeparateCalls;
        lastStencilFace = face;
    }
    void stencilMaskSeparate(GLenum face, GLuint) override {
        ++stencilMaskSeparateCalls;
        lastStencilFace = face;
    }
    void colorMask(bool r, bool g, bool b, bool a) override {
        ++colorMaskCalls;
        lastColorMaskR = r;
        lastColorMaskG = g;
        lastColorMaskB = b;
        lastColorMaskA = a;
    }
    void colorMaski(uint32_t buf, bool r, bool g, bool b, bool a) override {
        ++colorMaskiCalls;
        lastColorMaskiBuf = buf;
        lastColorMaskiR = r;
        lastColorMaskiG = g;
        lastColorMaskiB = b;
        lastColorMaskiA = a;
    }
    void sampleCoverage(float value, bool invert) override {
        ++sampleCoverageCalls;
        lastSampleCoverageValue = value;
        lastSampleCoverageInvert = invert;
    }
    void primitiveRestart(uint32_t index) override {
        ++primitiveRestartCalls;
        lastPrimitiveRestartIndex = index;
    }
    void hint(uint32_t target, uint32_t mode) override {
        ++hintCalls;
        lastHintTarget = target;
        lastHintMode = mode;
        hintCallsList.emplace_back(target, mode);
    }
    // Conditional rendering (SPEC §10.11). Recorded so tests can assert the
    // region is opened/closed with the expected query id and predicate mode.
    int beginConditionalRenderCalls = 0;
    uint32_t lastConditionalRenderId = 0;
    uint32_t lastConditionalRenderMode = 0;
    int endConditionalRenderCalls = 0;
    void beginConditionalRender(uint32_t id, uint32_t mode) override {
        ++beginConditionalRenderCalls;
        lastConditionalRenderId = id;
        lastConditionalRenderMode = mode;
    }
    void endConditionalRender() override { ++endConditionalRenderCalls; }
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
    void polygonOffset(float factor, float units, float clamp) override {
        ++polygonOffsetCalls;
        lastPolygonOffsetFactor = factor;
        lastPolygonOffsetUnits = units;
        lastPolygonOffsetClamp = clamp;
    }
    void polygonMode(uint32_t front, uint32_t back) override {
        ++polygonModeCalls;
        lastPolygonModeFront = front;
        lastPolygonModeBack = back;
    }
    void sampleMaski(uint32_t maskNumber, uint32_t mask) override {
        ++sampleMaskiCalls;
        lastSampleMaskNumber = maskNumber;
        lastSampleMask = mask;
    }
    void minSampleShading(float value) override {
        ++minSampleShadingCalls;
        lastMinSampleShading = value;
    }
    void provokingVertex(uint32_t mode) override {
        ++provokingVertexCalls;
        lastProvokingVertexMode = mode;
    }
    void pointParameters(float sizeMin, float sizeMax, float fadeThreshold,
                          GLenum spriteCoordOrigin) override {
        ++pointParametersCalls;
        lastPointSizeMin = sizeMin;
        lastPointSizeMax = sizeMax;
        lastPointFadeThreshold = fadeThreshold;
        lastSpriteCoordOrigin = spriteCoordOrigin;
    }
    void patchParameteri(uint32_t pname, int value) override {
        ++patchParameterCalls;
        lastPatchPname = pname;
        lastPatchVertices = static_cast<uint32_t>(value);
    }
    void patchParameterfv(uint32_t pname, const float* values) override {
        ++patchParameterCalls;
        lastPatchPname = pname;
        if (pname == GL_PATCH_DEFAULT_OUTER_LEVEL) {
            for (int i = 0; i < 4; ++i) lastPatchOuterLevel[i] = values[i];
        } else if (pname == GL_PATCH_DEFAULT_INNER_LEVEL) {
            for (int i = 0; i < 2; ++i) lastPatchInnerLevel[i] = values[i];
        }
    }
    void clipControl(GLenum origin, GLenum depth) override {
        ++clipControlCalls;
        lastClipOrigin = origin;
        lastClipDepthMode = depth;
    }
    void clampColor(uint32_t target, uint32_t mode) override {
        ++clampColorCalls;
        lastClampColorTarget = target;
        lastClampColorMode = mode;
    }
    void pixelStorei(GLenum, GLint) override { ++pixelStoreiCalls; }
    void setViewport(int32_t x, int32_t y, int32_t w, int32_t h) override {
        ++viewportCalls;
        lastViewportX = x;
        lastViewportY = y;
        lastViewportW = w;
        lastViewportH = h;
    }
    void setViewportIndexed(uint32_t index, int32_t x, int32_t y, int32_t w,
                            int32_t h) override {
        ++viewportIndexedCalls;
        lastViewportIndexed = index;
        lastViewportIndexedX = x;
        lastViewportIndexedY = y;
        lastViewportIndexedW = w;
        lastViewportIndexedH = h;
    }
    void setScissor(int32_t x, int32_t y, int32_t w, int32_t h) override {
        ++scissorCalls;
        lastScissorX = x;
        lastScissorY = y;
        lastScissorW = w;
        lastScissorH = h;
    }
    void setScissorIndexed(uint32_t index, int32_t x, int32_t y, int32_t w,
                           int32_t h) override {
        ++scissorIndexedCalls;
        lastScissorIndexed = index;
        lastScissorIndexedX = x;
        lastScissorIndexedY = y;
        lastScissorIndexedW = w;
        lastScissorIndexedH = h;
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
    void clearStencil(int s) override {
        ++clearStencilCalls;
        lastClearStencil = s;
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
    int bindFramebufferCalls = 0;
    uint32_t lastBindFramebufferTarget = 0;
    uint32_t lastBindFramebuffer = 0;
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
        enableVertexAttribOrder.push_back(index);
    }
    void disableVertexAttribArray(uint32_t index) override {
        ++disableVertexAttribArrayCalls;
        lastAttribIndex = index;
        disableVertexAttribOrder.push_back(index);
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
        vertexAttribPtrs.push_back(
            {index, size, type, normalized, stride, offset});
    }
    int vertexAttribDivisorCalls = 0;
    uint32_t lastAttribDivisorIndex = 0;
    uint32_t lastAttribDivisor = 0;
    void vertexAttribDivisor(uint32_t index, uint32_t divisor) override {
        ++vertexAttribDivisorCalls;
        lastAttribDivisorIndex = index;
        lastAttribDivisor = divisor;
        attribDivisors.push_back({index, divisor});
    }
    int bindBufferCalls = 0;
    uint32_t lastBindBufferTarget = 0;
    GLObjectName lastBindBufferName = 0;
    void bindBuffer(uint32_t target, uint32_t buffer) override {
        ++bindBufferCalls;
        lastBindBufferTarget = target;
        lastBindBufferName = buffer;
        bufferBinds.push_back({target, buffer});
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

    // Records the last glBindImageTexture push (whole binding).
    int bindImageTextureCalls = 0;
    uint32_t lastBindImageTextureUnit = 0;
    GLObjectName lastBindImageTexture = 0;
    int lastBindImageTextureLevel = 0;
    bool lastBindImageTextureLayered = false;
    int lastBindImageTextureLayer = 0;
    uint32_t lastBindImageTextureAccess = 0;
    uint32_t lastBindImageTextureFormat = 0;
    void bindImageTexture(uint32_t unit, uint32_t texture, int level,
                          bool layered, int layer, uint32_t access,
                          uint32_t format) override {
        ++bindImageTextureCalls;
        lastBindImageTextureUnit = unit;
        lastBindImageTexture = texture;
        lastBindImageTextureLevel = level;
        lastBindImageTextureLayered = layered;
        lastBindImageTextureLayer = layer;
        lastBindImageTextureAccess = access;
        lastBindImageTextureFormat = format;
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

    // --- Transform-feedback draw recording (SPEC §13.3.3) ---
    int drawTransformFeedbackCalls = 0;
    int drawTransformFeedbackInstancedCalls = 0;
    int drawTransformFeedbackStreamCalls = 0;
    int drawTransformFeedbackStreamInstancedCalls = 0;
    uint32_t lastDrawTfId = 0;
    uint32_t lastDrawStream = 0;
    void drawTransformFeedback(uint32_t mode, uint32_t tfId, int32_t count) override {
        ++drawTransformFeedbackCalls;
        lastDrawMode = mode;
        lastDrawTfId = tfId;
        lastDrawCount = count;
    }
    void drawTransformFeedbackInstanced(uint32_t mode, uint32_t tfId, int32_t count,
                                       int32_t primcount) override {
        ++drawTransformFeedbackInstancedCalls;
        lastDrawMode = mode;
        lastDrawTfId = tfId;
        lastDrawCount = count;
        lastDrawPrimcount = primcount;
    }
    void drawTransformFeedbackStream(uint32_t mode, uint32_t tfId, uint32_t stream,
                                    int32_t count) override {
        ++drawTransformFeedbackStreamCalls;
        lastDrawMode = mode;
        lastDrawTfId = tfId;
        lastDrawStream = stream;
        lastDrawCount = count;
    }
    void drawTransformFeedbackStreamInstanced(uint32_t mode, uint32_t tfId,
                                             uint32_t stream, int32_t count,
                                             int32_t primcount) override {
        ++drawTransformFeedbackStreamInstancedCalls;
        lastDrawMode = mode;
        lastDrawTfId = tfId;
        lastDrawStream = stream;
        lastDrawCount = count;
        lastDrawPrimcount = primcount;
    }

    // --- Draw expansion (SPEC §10) ---
    int multiDrawArraysCalls = 0;
    int multiDrawElementsCalls = 0;
    int drawRangeElementsCalls = 0;
    int drawElementsBaseVertexCalls = 0;
    int32_t lastMultiDrawCount = 0;
    uint32_t lastDrawStart = 0;
    uint32_t lastDrawEnd = 0;
    int32_t lastDrawBasevertex = 0;
    void multiDrawArrays(uint32_t mode, const int32_t* firsts,
                         const int32_t* counts, int32_t drawcount) override {
        ++multiDrawArraysCalls;
        lastDrawMode = mode;
        lastMultiDrawCount = drawcount;
        (void)firsts;
        (void)counts;
    }
    void multiDrawElements(uint32_t mode, const int32_t* counts, uint32_t type,
                           const intptr_t* indices, int32_t drawcount) override {
        ++multiDrawElementsCalls;
        lastDrawMode = mode;
        lastDrawType = type;
        lastMultiDrawCount = drawcount;
        (void)counts;
        (void)indices;
    }
    void drawRangeElements(uint32_t mode, uint32_t start, uint32_t end,
                           int32_t count, uint32_t type,
                           intptr_t indices) override {
        ++drawRangeElementsCalls;
        lastDrawMode = mode;
        lastDrawStart = start;
        lastDrawEnd = end;
        lastDrawCount = count;
        lastDrawType = type;
        lastDrawIndices = indices;
    }
    void drawElementsBaseVertex(uint32_t mode, int32_t count, uint32_t type,
                                intptr_t indices, int32_t basevertex) override {
        ++drawElementsBaseVertexCalls;
        lastDrawMode = mode;
        lastDrawCount = count;
        lastDrawType = type;
        lastDrawIndices = indices;
        lastDrawBasevertex = basevertex;
    }

    // Indirect draw (SPEC §10). Recorded so tests can assert the call is issued
    // (the mock has no driver; the frontend validates the indirect buffer / program).
    int drawArraysIndirectCalls = 0;
    int drawElementsIndirectCalls = 0;
    const void* lastIndirect = nullptr;
    void drawArraysIndirect(uint32_t mode, const void* indirect) override {
        ++drawArraysIndirectCalls;
        lastDrawMode = mode;
        lastIndirect = indirect;
    }
    void drawElementsIndirect(uint32_t mode, uint32_t type,
                             const void* indirect) override {
        ++drawElementsIndirectCalls;
        lastDrawMode = mode;
        lastDrawType = type;
        lastIndirect = indirect;
    }

    // Compute dispatch (SPEC §7.4). Recorded so tests can assert the call is
    // issued (the mock has no driver; the frontend validates the feature / program).
    int dispatchComputeCalls = 0;
    int dispatchComputeIndirectCalls = 0;
    uint32_t lastDispatchX = 0, lastDispatchY = 0, lastDispatchZ = 0;
    uintptr_t lastDispatchIndirect = 0;
    void dispatchCompute(uint32_t x, uint32_t y, uint32_t z) override {
        ++dispatchComputeCalls;
        lastDispatchX = x;
        lastDispatchY = y;
        lastDispatchZ = z;
    }
    void dispatchComputeIndirect(uintptr_t offset) override {
        ++dispatchComputeIndirectCalls;
        lastDispatchIndirect = offset;
    }

    void clear(uint32_t mask) override {
        ++clearCalls;
        lastClearMask = mask;
    }

    void clearTexImage(uint32_t texture, int level, uint32_t format, uint32_t type,
                      const void* data) override {
        ++clearTexImageCalls;
        lastClearTexImageName = texture;
        lastClearTexImageLevel = level;
        lastClearTexImageFormat = format;
        lastClearTexImageType = type;
    }

    void clearTexSubImage(uint32_t texture, int level, int x, int y, int z, int w,
                         int h, int d, uint32_t format, uint32_t type,
                         const void* data) override {
        ++clearTexSubImageCalls;
        lastClearTexSubImageName = texture;
        lastClearTexSubImageLevel = level;
        lastClearTexSubImageX = x;
        lastClearTexSubImageY = y;
        lastClearTexSubImageZ = z;
        lastClearTexSubImageW = w;
        lastClearTexSubImageH = h;
        lastClearTexSubImageD = d;
        lastClearTexSubImageFormat = format;
        lastClearTexSubImageType = type;
    }

    void copyImageSubData(uint32_t srcName, uint32_t srcTarget, int srcLevel,
                          int srcX, int srcY, int srcZ, uint32_t dstName,
                          uint32_t dstTarget, int dstLevel, int dstX, int dstY,
                          int dstZ, int srcWidth, int srcHeight,
                          int srcDepth) override {
        ++copyImageSubDataCalls;
        lastCopySrcName = srcName;
        lastCopySrcTarget = srcTarget;
        lastCopySrcLevel = srcLevel;
        lastCopySrcX = srcX;
        lastCopySrcY = srcY;
        lastCopySrcZ = srcZ;
        lastCopyDstName = dstName;
        lastCopyDstTarget = dstTarget;
        lastCopyDstLevel = dstLevel;
        lastCopyDstX = dstX;
        lastCopyDstY = dstY;
        lastCopyDstZ = dstZ;
        lastCopyWidth = srcWidth;
        lastCopyHeight = srcHeight;
        lastCopyDepth = srcDepth;
    }

    // Color logic op (SPEC §17.3.4). Recorded so tests can assert it is pushed
    // only when the mode changes.
    int logicOpCalls = 0;
    uint32_t lastLogicOp = 0;
    void logicOp(uint32_t mode) override {
        ++logicOpCalls;
        lastLogicOp = mode;
    }

    // Whole-framebuffer ops (SPEC §15 / §16). Recorded for assertions.
    int blitFramebufferCalls = 0;
    int32_t lastBlitSrcX0 = 0, lastBlitSrcY0 = 0, lastBlitSrcX1 = 0,
            lastBlitSrcY1 = 0;
    int32_t lastBlitDstX0 = 0, lastBlitDstY0 = 0, lastBlitDstX1 = 0,
            lastBlitDstY1 = 0;
    uint32_t lastBlitMask = 0, lastBlitFilter = 0;
    void blitFramebuffer(int32_t srcX0, int32_t srcY0, int32_t srcX1, int32_t srcY1,
                        int32_t dstX0, int32_t dstY0, int32_t dstX1, int32_t dstY1,
                        uint32_t mask, uint32_t filter) override {
        ++blitFramebufferCalls;
        lastBlitSrcX0 = srcX0; lastBlitSrcY0 = srcY0; lastBlitSrcX1 = srcX1;
        lastBlitSrcY1 = srcY1; lastBlitDstX0 = dstX0; lastBlitDstY0 = dstY0;
        lastBlitDstX1 = dstX1; lastBlitDstY1 = dstY1;
        lastBlitMask = mask; lastBlitFilter = filter;
    }

    int invalidateFramebufferCalls = 0;
    bool lastInvalidateSub = false;
    uint32_t lastInvalidateTarget = 0;
    int32_t lastInvalidateNum = 0;
    std::vector<uint32_t> lastInvalidateAttachments;
    int32_t lastInvalidateX = 0, lastInvalidateY = 0, lastInvalidateW = 0,
            lastInvalidateH = 0;
    void invalidateFramebuffer(uint32_t target, int32_t numAttachments,
                               const uint32_t* attachments, int32_t x, int32_t y,
                               int32_t width, int32_t height) override {
        ++invalidateFramebufferCalls;
        lastInvalidateTarget = target;
        lastInvalidateNum = numAttachments;
        lastInvalidateSub = (width != 0 || height != 0);
        lastInvalidateX = x; lastInvalidateY = y;
        lastInvalidateW = width; lastInvalidateH = height;
        lastInvalidateAttachments.assign(attachments, attachments + numAttachments);
    }
    void flush() override { ++flushCalls; }
    void finish() override { ++finishCalls; }
    void memoryBarrier(uint32_t barriers) override {
        ++memoryBarrierCalls;
        lastBarriers = barriers;
    }
    void memoryBarrierByRegion(uint32_t barriers) override {
        ++memoryBarrierByRegionCalls;
        lastBarriersByRegion = barriers;
    }
    int memoryBarrierCalls = 0;
    uint32_t lastBarriers = 0;
    int memoryBarrierByRegionCalls = 0;
    uint32_t lastBarriersByRegion = 0;
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

    // Internal format queries (SPEC §22.3). The mock does not model real driver
    // format support, so it returns a conservative documented default: no sample
    // counts, supported=true for a curated set of common core formats, and 0 for
    // every other pname. Unit tests assert this contract.
    void getInternalformativ(uint32_t target, uint32_t internalformat,
                            uint32_t pname, int32_t bufSize,
                            int32_t* params) override {
        (void)target;
        if (params == nullptr || bufSize <= 0) return;
        auto supported = [](uint32_t f) -> bool {
            switch (f) {
            case GL_RGBA8: case GL_RGB8: case GL_RGBA16F: case GL_RGB16F:
            case GL_R8: case GL_RG8: case GL_R16F: case GL_RG16F:
            case GL_DEPTH24_STENCIL8: case GL_DEPTH_COMPONENT24:
            case GL_DEPTH_COMPONENT32F: case GL_R11F_G11F_B10F:
            case GL_SRGB8_ALPHA8: case GL_RGB10_A2:
                return true;
            default:
                return false;
            }
        };
        switch (pname) {
        case GL_NUM_SAMPLE_COUNTS: params[0] = 0; break;
        case GL_SAMPLES: /* 0 sample counts -> nothing to write */ break;
        case GL_INTERNALFORMAT_SUPPORTED:
            params[0] = supported(internalformat)
                            ? static_cast<int32_t>(GL_TRUE)
                            : static_cast<int32_t>(GL_FALSE);
            break;
        default: params[0] = 0; break;
        }
    }
    void getInternalformati64v(uint32_t target, uint32_t internalformat,
                              uint32_t pname, int32_t bufSize,
                              int64_t* params) override {
        (void)target;
        if (params == nullptr || bufSize <= 0) return;
        auto supported = [](uint32_t f) -> bool {
            switch (f) {
            case GL_RGBA8: case GL_RGB8: case GL_RGBA16F: case GL_RGB16F:
            case GL_R8: case GL_RG8: case GL_R16F: case GL_RG16F:
            case GL_DEPTH24_STENCIL8: case GL_DEPTH_COMPONENT24:
            case GL_DEPTH_COMPONENT32F: case GL_R11F_G11F_B10F:
            case GL_SRGB8_ALPHA8: case GL_RGB10_A2:
                return true;
            default:
                return false;
            }
        };
        switch (pname) {
        case GL_NUM_SAMPLE_COUNTS: params[0] = 0; break;
        case GL_SAMPLES: break;
        case GL_INTERNALFORMAT_SUPPORTED:
            params[0] = supported(internalformat)
                            ? static_cast<int64_t>(GL_TRUE)
                            : static_cast<int64_t>(GL_FALSE);
            break;
        default: params[0] = 0; break;
        }
    }
    // Multisample sample-position queries (SPEC §14.3.1). Sample positions are
    // implementation-defined; the mock reports a fixed placeholder sample count
    // and a fixed sub-pixel grid so the frontend's index validation and the
    // (x, y) result contract are deterministic and testable. Unit tests assert
    // this contract.
    uint32_t getMultisampleSampleCount() override { return kMockSampleCount; }
    void getMultisamplefv(uint32_t pname, uint32_t index, float* val) override {
        (void)pname;
        if (val == nullptr || index >= kMockSampleCount) return;
        // Fixed deterministic grid in [0,1]^2; not meant to model any real GPU.
        val[0] = (index % 2u) ? 0.25f : 0.75f;
        val[1] = ((index / 2u) % 2u) ? 0.25f : 0.75f;
    }

private:
    // Number of samples the mock pretends its framebuffers have.
    static constexpr uint32_t kMockSampleCount = 4;

    CapabilityTable capabilities_;
    MockPlatformCapabilities platform_;
    MockResourceFactory factory_;
    MockShaderCompiler compiler_;
    bool initialized_ = false;
};

} // namespace glcompat
