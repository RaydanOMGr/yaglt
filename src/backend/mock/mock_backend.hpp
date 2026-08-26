#pragma once

#include "glcompat/core/backend.hpp"
#include "mock_capabilities.hpp"
#include "mock_factory.hpp"
#include "mock_shader_compiler.hpp"

namespace glcompat {

// In-memory backend for tests and headless development. No native GL context
// is created; resources are tracking objects. Exercises the frontend<->backend
// abstraction without a real driver.
class MockBackend : public IGraphicsBackend {
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

    std::string describe() const override {
        return "MockBackend(" + platform_.describe() + ")";
    }

private:
    CapabilityTable capabilities_;
    MockPlatformCapabilities platform_;
    MockResourceFactory factory_;
    MockShaderCompiler compiler_;
    bool initialized_ = false;
};

} // namespace glcompat
