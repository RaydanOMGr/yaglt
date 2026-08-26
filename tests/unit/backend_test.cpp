#include "test_framework.hpp"

#include "glcompat/core/backend.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("mock_backend_initialize_reports_mock_api") {
    MockBackend backend;
    EXPECT_EQ(backend.api(), BackendApi::Mock);
    EXPECT_TRUE(backend.initialize());
    EXPECT_NE(backend.describe().find("MockBackend"), std::string::npos);
    backend.shutdown();
}

TEST_CASE("mock_backend_core_features_native") {
    MockBackend backend;
    const ICapabilities& caps = backend.capabilities();
    EXPECT_EQ(caps.getFeatureSupport(Feature::BufferObjects), FeatureSupport::Native);
    EXPECT_EQ(caps.getFeatureSupport(Feature::TextureObjects), FeatureSupport::Native);
    EXPECT_EQ(caps.getFeatureSupport(Feature::VertexArrayObjects), FeatureSupport::Native);
    EXPECT_TRUE(caps.isSupported(Feature::DirectStateAccess)); // emulated
    EXPECT_FALSE(caps.isSupported(Feature::ComputeShaders));   // unsupported
}

TEST_CASE("mock_backend_resource_factory_unique_ids") {
    MockBackend backend;
    auto a = backend.resourceFactory().createBuffer();
    auto b = backend.resourceFactory().createBuffer();
    auto tex = backend.resourceFactory().createTexture();
    EXPECT_NE(static_cast<MockBuffer*>(a.get())->id, static_cast<MockBuffer*>(b.get())->id);
    EXPECT_NE(static_cast<MockBuffer*>(a.get())->id, static_cast<MockTexture*>(tex.get())->id);
}

TEST_CASE("mock_backend_shader_compiler_passthrough") {
    MockBackend backend;
    std::string out, err;
    EXPECT_TRUE(backend.shaderCompiler().compile("void main(){}", out, err));
    EXPECT_EQ(out, std::string("void main(){}"));
    EXPECT_TRUE(err.empty());

    EXPECT_FALSE(backend.shaderCompiler().compile("", out, err));
    EXPECT_FALSE(err.empty());
}

TEST_CASE("mock_backend_platform_linux_headless") {
    MockBackend backend;
    EXPECT_EQ(backend.platform().os(), PlatformOs::Linux);
    EXPECT_EQ(backend.platform().androidSdkVersion(), 0);
    EXPECT_TRUE(backend.platform().supportsSharedMemory());
}
