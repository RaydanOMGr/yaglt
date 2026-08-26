#include "test_framework.hpp"

#include "glcompat/core/capabilities.hpp"
#include "glcompat/core/capabilities_table.hpp"

using namespace glcompat;

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
