#include "test_framework.hpp"

#include "glcompat/backend/gles/gles_capabilities.hpp"
#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/capabilities.hpp"
#include "glcompat/core/capabilities_table.hpp"

using namespace glcompat;

namespace {
// Build a GLESLib with only the version fields set, so we can exercise
// populateGLESCapabilities' version-driven classification without a driver.
GLESLib fakeLib(int major, int minor) {
    GLESLib lib;
    lib.glesMajor = major;
    lib.glesMinor = minor;
    return lib;
}
} // namespace

TEST_CASE("capability_table_default_unsupported") {
    CapabilityTable t;
    EXPECT_EQ(t.getFeatureSupport(Feature::BufferObjects), FeatureSupport::Unsupported);
    EXPECT_FALSE(t.isSupported(Feature::BufferObjects));
}

TEST_CASE("capability_table_set_and_query") {
    CapabilityTable t;
    t.set(Feature::BufferObjects, FeatureSupport::Native);
    t.set(Feature::ProgramPipelines, FeatureSupport::Emulated);
    t.set(Feature::GeometryShaders, FeatureSupport::Unsupported);

    EXPECT_EQ(t.getFeatureSupport(Feature::BufferObjects), FeatureSupport::Native);
    EXPECT_TRUE(t.isSupported(Feature::BufferObjects));

    EXPECT_EQ(t.getFeatureSupport(Feature::ProgramPipelines), FeatureSupport::Emulated);
    EXPECT_TRUE(t.isSupported(Feature::ProgramPipelines)); // emulated counts as supported

    EXPECT_EQ(t.getFeatureSupport(Feature::GeometryShaders), FeatureSupport::Unsupported);
    EXPECT_FALSE(t.isSupported(Feature::GeometryShaders));
}

TEST_CASE("capability_feature_name_known") {
    CapabilityTable t;
    EXPECT_EQ(t.featureName(Feature::DirectStateAccess), std::string("DirectStateAccess"));
    EXPECT_EQ(t.featureName(Feature::FeatureCount), std::string("FeatureCount"));
}

TEST_CASE("gles_capabilities_ubo_ssbo_by_version") {
    // ES 3.0: UBO native, SSBO unsupported.
    {
        CapabilityTable t;
        populateGLESCapabilities(t, fakeLib(3, 0));
        EXPECT_EQ(t.getFeatureSupport(Feature::UniformBufferObjects), FeatureSupport::Native);
        EXPECT_EQ(t.getFeatureSupport(Feature::ShaderStorageBufferObjects), FeatureSupport::Unsupported);
    }
    // ES 3.1: both native.
    {
        CapabilityTable t;
        populateGLESCapabilities(t, fakeLib(3, 1));
        EXPECT_EQ(t.getFeatureSupport(Feature::UniformBufferObjects), FeatureSupport::Native);
        EXPECT_EQ(t.getFeatureSupport(Feature::ShaderStorageBufferObjects), FeatureSupport::Native);
    }
    // Geometry/tessellation have no GLES equivalent regardless of version.
    {
        CapabilityTable t;
        populateGLESCapabilities(t, fakeLib(3, 1));
        EXPECT_EQ(t.getFeatureSupport(Feature::GeometryShaders), FeatureSupport::Unsupported);
        EXPECT_EQ(t.getFeatureSupport(Feature::TessellationShaders), FeatureSupport::Unsupported);
    }
}
