#pragma once

#include "glcompat/backend/gles/gles_capabilities.hpp"
#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/backend.hpp"
#include "glcompat/core/capabilities_table.hpp"
#include "glcompat/core/factory.hpp"
#include "glcompat/core/platform.hpp"
#include "glcompat/state/gl_state_sink.hpp"
#include "src/backend/gles/gles_factory.hpp"
#include "src/backend/gles/gles_shader_compiler.hpp"
#include "src/platform/linux/linux_capabilities.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace glcompat {

// Real OpenGL ES backend. Uses a headless, surfaceless EGL context so it works
// without a window system (Linux/Mesa, Android). Symbols are resolved at
// runtime via GLESLib, so this compiles even where libGLESv2 dev libs are
// absent. initialize() returns false honestly when no driver is available.
class GLESBackend : public IGraphicsBackend, public GLStateSink {
public:
    GLESBackend();
    ~GLESBackend() override;

    BackendApi api() const override { return BackendApi::GLES; }
    const ICapabilities& capabilities() const override { return caps_; }
    const IPlatformCapabilities& platform() const override { return platform_; }

    IResourceFactory& resourceFactory() override { return factory_; }
    IShaderCompiler& shaderCompiler() override { return *compiler_; }

    bool initialize() override;
    void shutdown() override;

    GLStateSink* stateSink() override { return this; }

    std::string describe() const override;

    // GLStateSink: push tracked state to the native driver. The frontend calls
    // these via GLStateTracker::apply() at draw / flush time (SPEC §10).
    void enable(uint32_t cap) override;
    void disable(uint32_t cap) override;
    void useProgram(uint32_t prog) override;
    void blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB, uint32_t srcAlpha,
                           uint32_t dstAlpha) override;
    void blendEquationSeparate(uint32_t modeRGB, uint32_t modeAlpha) override;
    void blendColor(float r, float g, float b, float a) override;
    void depthFunc(uint32_t func) override;
    void depthMask(bool enabled) override;
    void depthRange(double nearVal, double farVal) override;
    void stencilFunc(uint32_t func, int32_t ref, uint32_t mask) override;
    void stencilOp(uint32_t sfail, uint32_t dpfail, uint32_t dppass) override;
    void stencilMask(uint32_t mask) override;
    void cullFace(uint32_t mode) override;
    void frontFace(uint32_t mode) override;
    void pointSize(float size) override;
    void lineWidth(float width) override;
    void polygonOffset(float factor, float units) override;
    void pixelStorei(uint32_t pname, int32_t param) override;
    void setViewport(int32_t x, int32_t y, int32_t w, int32_t h) override;
    void setScissor(int32_t x, int32_t y, int32_t w, int32_t h) override;
    void clearColor(float r, float g, float b, float a) override;
    void clearDepth(double d) override;
    void bindBufferBase(uint32_t target, uint32_t index, uint32_t buffer) override;
    void bindBufferRange(uint32_t target, uint32_t index, uint32_t buffer,
                          intptr_t offset, intptr_t size) override;

    // Texture units (SPEC §2.1). The frontend name is resolved to the native
    // driver id via the registered name map, mirroring useProgram/bindVertexArray.
    void activeTexture(uint32_t unit) override;
    void bindTexture(uint32_t target, uint32_t texture) override;

    // Sampler objects (SPEC §8.2). `sampler` is the frontend name resolved to the
    // native driver sampler id via the registered name map; `unit` is the
    // zero-based texture unit index.
    void bindSampler(uint32_t unit, uint32_t sampler) override;

    // Vertex array + attribute setup (SPEC §2.1).
    void bindVertexArray(uint32_t vao) override;
    void enableVertexAttribArray(uint32_t index) override;
    void disableVertexAttribArray(uint32_t index) override;
    void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                             bool normalized, int32_t stride,
                             intptr_t offset) override;

    // Frontend name -> native id mapping so useProgram/bindVertexArray can bind
    // the real driver objects (SPEC §3/§11).
    void bindNativeObject(uint32_t name, uint32_t nativeId) override;

    // Test/debug access to the registered name -> native id map.
    const std::unordered_map<uint32_t, uint32_t>& nativeMap() const {
        return nativeMap_;
    }

    // Draw commands (SPEC §2.1). The frontend flushes tracked pipeline state
    // before calling these, so the driver already sees current GL state.
    void drawArrays(uint32_t mode, int32_t first, int32_t count) override;
    void drawElements(uint32_t mode, int32_t count, uint32_t type,
                      intptr_t indices) override;
    void drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                             int32_t primcount) override;
    void drawElementsInstanced(uint32_t mode, int32_t count, uint32_t type,
                                intptr_t indices, int32_t primcount) override;

    // Clear the bound framebuffer (SPEC §2.1). The frontend pushes the tracked
    // clear color/depth through GLStateSink first, so this issues the native
    // clear with the current values.
    void clear(uint32_t mask) override;

    // Command stream flush / finish (SPEC §2.1).
    void flush() override;
    void finish() override;

    // Read back pixels from the bound framebuffer (SPEC §2.1).
    void readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                    uint32_t format, uint32_t type, void* pixels) override;

private:
    GLESLibPtr lib_;
    LinuxCapabilities platform_;
    CapabilityTable caps_;
    GLESResourceFactory factory_;
    std::unique_ptr<IShaderCompiler> compiler_;

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    bool initialized_ = false;

    // Frontend object name -> backend-native id (only entries that the frontend
    // has registered via bindNativeObject).
    std::unordered_map<uint32_t, uint32_t> nativeMap_;

    bool createContext();
    void queryVersion();
};

} // namespace glcompat
