#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// glPolygonMode (SPEC §11.1): per-side render mode, forwarded to the backend.
TEST_CASE("rc_polygon_mode_forwards_to_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);

    ctx.polygonMode(GL_FRONT, GL_LINE);
    ctx.flushState();
    EXPECT_EQ(backend->polygonModeCalls, 1);
    EXPECT_EQ(backend->lastPolygonModeFront, static_cast<uint32_t>(GL_LINE));
    EXPECT_EQ(backend->lastPolygonModeBack, static_cast<uint32_t>(GL_FILL));

    ctx.polygonMode(GL_BACK, GL_POINT);
    ctx.flushState();
    EXPECT_EQ(backend->polygonModeCalls, 2);
    EXPECT_EQ(backend->lastPolygonModeFront, static_cast<uint32_t>(GL_LINE));
    EXPECT_EQ(backend->lastPolygonModeBack, static_cast<uint32_t>(GL_POINT));

    // FRONT_AND_BACK updates both sides from a single call.
    ctx.polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    ctx.flushState();
    EXPECT_EQ(backend->polygonModeCalls, 3);
    EXPECT_EQ(backend->lastPolygonModeFront, static_cast<uint32_t>(GL_FILL));
    EXPECT_EQ(backend->lastPolygonModeBack, static_cast<uint32_t>(GL_FILL));
}

TEST_CASE("rc_polygon_mode_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    // Default is FILL for both sides.
    GLint pm[2] = {0, 0};
    ctx.getIntegerv(GL_POLYGON_MODE, pm);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(pm[0], static_cast<GLint>(GL_FILL));
    EXPECT_EQ(pm[1], static_cast<GLint>(GL_FILL));

    // Unsupported face reports GL_INVALID_ENUM and leaves state untouched.
    ctx.polygonMode(0x9999, GL_FILL);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    ctx.getIntegerv(GL_POLYGON_MODE, pm);
    EXPECT_EQ(pm[0], static_cast<GLint>(GL_FILL));

    // Unsupported mode reports GL_INVALID_ENUM.
    ctx.polygonMode(GL_FRONT, 0x9999);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

// glSampleMaski (SPEC §11.5): one mask word, forwarded to the backend.
TEST_CASE("rc_sample_maski_forwards_to_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);

    ctx.sampleMaski(0, 0x1234u);
    ctx.flushState();
    EXPECT_EQ(backend->sampleMaskiCalls, 1);
    EXPECT_EQ(backend->lastSampleMaskNumber, 0u);
    EXPECT_EQ(backend->lastSampleMask, 0x1234u);

    ctx.sampleMaski(1, 0xABCDu);
    ctx.flushState();
    EXPECT_EQ(backend->sampleMaskiCalls, 2);
    EXPECT_EQ(backend->lastSampleMaskNumber, 1u);
    EXPECT_EQ(backend->lastSampleMask, 0xABCDu);

    // Out-of-range mask number reports GL_INVALID_VALUE.
    ctx.sampleMaski(99, 0x1u);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// glMinSampleShading (SPEC §11.5): minimum sample-shading fraction.
TEST_CASE("rc_min_sample_shading_forwards_to_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);

    ctx.minSampleShading(0.5f);
    ctx.flushState();
    EXPECT_EQ(backend->minSampleShadingCalls, 1);
    EXPECT_TRUE(backend->lastMinSampleShading == 0.5f);

    // Outside [0,1] reports GL_INVALID_VALUE.
    ctx.minSampleShading(2.0f);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.minSampleShading(-0.1f);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// State is queryable via glGet* (SPEC §22) without touching the backend.
TEST_CASE("rc_state_queryable_via_get") {
    auto backend = makeBackend();
    Context ctx(*backend);

    ctx.polygonMode(GL_FRONT_AND_BACK, GL_POINT);
    GLint pm[2] = {0, 0};
    ctx.getIntegerv(GL_POLYGON_MODE, pm);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(pm[0], static_cast<GLint>(GL_POINT));
    EXPECT_EQ(pm[1], static_cast<GLint>(GL_POINT));

    ctx.sampleMaski(0, 0xABCDu);
    GLint sm[2] = {0, 0};
    ctx.getIntegerv(GL_SAMPLE_MASK, sm);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(sm[0], static_cast<GLint>(0xABCDu));

    ctx.minSampleShading(0.25f);
    GLfloat ms = 0.0f;
    ctx.getFloatv(GL_MIN_SAMPLE_SHADING, &ms);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(ms == 0.25f);
}
