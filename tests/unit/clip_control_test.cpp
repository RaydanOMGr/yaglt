#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

namespace {

std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

} // namespace

// glClipControl (SPEC §12.1) is frontend-owned clip-volume state, pushed to the
// backend only when it changes (SPEC §10).
TEST_CASE("clip_control_default_is_lower_left_negative_one_to_one") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t origin = 0, depth = 0;
    ctx.getIntegerv(GL_CLIP_ORIGIN, &origin);
    ctx.getIntegerv(GL_CLIP_DEPTH_MODE, &depth);
    EXPECT_EQ(origin, GL_LOWER_LEFT);
    EXPECT_EQ(depth, GL_NEGATIVE_ONE_TO_ONE);
}

TEST_CASE("clip_control_pushes_on_change") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.clipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
    ctx.flushState();
    EXPECT_TRUE(backend->clipControlCalls >= 1);
    EXPECT_EQ(backend->lastClipOrigin, GL_UPPER_LEFT);
    EXPECT_EQ(backend->lastClipDepthMode, GL_ZERO_TO_ONE);

    // A second flush with no change must not push again.
    int after = backend->clipControlCalls;
    ctx.flushState();
    EXPECT_EQ(backend->clipControlCalls, after);
}

TEST_CASE("clip_control_reports_via_get") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.clipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
    int32_t origin = 0, depth = 0;
    ctx.getIntegerv(GL_CLIP_ORIGIN, &origin);
    ctx.getIntegerv(GL_CLIP_DEPTH_MODE, &depth);
    EXPECT_EQ(origin, GL_UPPER_LEFT);
    EXPECT_EQ(depth, GL_ZERO_TO_ONE);
}

TEST_CASE("clip_control_bad_origin_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.clipControl(0xDEAD, GL_NEGATIVE_ONE_TO_ONE);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    // State unchanged.
    int32_t origin = 0;
    ctx.getIntegerv(GL_CLIP_ORIGIN, &origin);
    EXPECT_EQ(origin, GL_LOWER_LEFT);
}

TEST_CASE("clip_control_bad_depth_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.clipControl(GL_LOWER_LEFT, 0xDEAD);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    int32_t depth = 0;
    ctx.getIntegerv(GL_CLIP_DEPTH_MODE, &depth);
    EXPECT_EQ(depth, GL_NEGATIVE_ONE_TO_ONE);
}

TEST_CASE("clip_control_via_public_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
    ctx.flushState();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->lastClipOrigin, GL_UPPER_LEFT);
    EXPECT_EQ(backend->lastClipDepthMode, GL_ZERO_TO_ONE);
    setCurrentContext(nullptr);
}
