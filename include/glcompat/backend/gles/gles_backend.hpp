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
    void blendFunc(uint32_t sfactor, uint32_t dfactor) override;
    void blendEquation(uint32_t mode) override;
    void depthFunc(uint32_t func) override;
    void depthMask(bool enabled) override;
    void stencilFunc(uint32_t func, int32_t ref, uint32_t mask) override;
    void stencilOp(uint32_t sfail, uint32_t dpfail, uint32_t dppass) override;
    void stencilMask(uint32_t mask) override;
    void cullFace(uint32_t mode) override;
    void frontFace(uint32_t mode) override;
    void pixelStorei(uint32_t pname, int32_t param) override;
    void bindBufferBase(uint32_t target, uint32_t index, uint32_t buffer) override;
    void bindBufferRange(uint32_t target, uint32_t index, uint32_t buffer,
                         intptr_t offset, intptr_t size) override;

private:
    GLESLibPtr lib_;
    LinuxCapabilities platform_;
    CapabilityTable caps_;
    GLESResourceFactory factory_;
    std::unique_ptr<IShaderCompiler> compiler_;

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    bool initialized_ = false;

    bool createContext();
    void queryVersion();
};

} // namespace glcompat
