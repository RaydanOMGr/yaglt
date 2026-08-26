#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/state/gl_state.hpp"
#include "mock_capabilities.hpp"
#include "mock_factory.hpp"
#include "mock_shader_compiler.hpp"

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

    IResourceFactory& resourceFactory() override { return factory_; }
    IShaderCompiler& shaderCompiler() override { return compiler_; }

    bool initialize() override { initialized_ = true; return true; }
    void shutdown() override { initialized_ = false; }

    GLStateSink* stateSink() override { return this; }

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

private:
    CapabilityTable capabilities_;
    MockPlatformCapabilities platform_;
    MockResourceFactory factory_;
    MockShaderCompiler compiler_;
    bool initialized_ = false;
};

} // namespace glcompat
