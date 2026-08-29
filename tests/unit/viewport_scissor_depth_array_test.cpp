#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "glcompat/state/gl_state.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {

TEST_CASE("viewport_arrayv_sets_contiguous_slots") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Two viewports starting at slot 2: (x, y, w, h) packed 4 floats each.
    GLfloat v[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    glViewportArrayv(2, 2, v);
    ctx.flushState();
    EXPECT_EQ(backend.viewportIndexedCalls, 2);
    EXPECT_EQ(backend.lastViewportIndexed, 3u);
    EXPECT_EQ(backend.lastViewportIndexedX, 50);
    EXPECT_EQ(backend.lastViewportIndexedY, 60);
    EXPECT_EQ(backend.lastViewportIndexedW, 70);
    EXPECT_EQ(backend.lastViewportIndexedH, 80);
    EXPECT_EQ(backend.viewportCalls, 0); // slot 0 untouched
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("scissor_arrayv_sets_contiguous_slots") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint v[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    glScissorArrayv(1, 2, v);
    ctx.flushState();
    EXPECT_EQ(backend.scissorIndexedCalls, 2);
    EXPECT_EQ(backend.lastScissorIndexed, 2u);
    EXPECT_EQ(backend.lastScissorIndexedX, 5);
    EXPECT_EQ(backend.lastScissorIndexedY, 6);
    EXPECT_EQ(backend.lastScissorIndexedW, 7);
    EXPECT_EQ(backend.lastScissorIndexedH, 8);
    EXPECT_EQ(backend.scissorCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("depth_range_indexed_nonzero_viewport") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glDepthRangeIndexed(2, 0.2, 0.8);
    ctx.flushState();
    EXPECT_EQ(backend.depthRangeIndexedCalls, 1);
    EXPECT_EQ(backend.lastDepthRangeIndexedIndex, 2u);
    EXPECT_EQ(backend.lastDepthNearIndexed, 0.2);
    EXPECT_EQ(backend.lastDepthFarIndexed, 0.8);
    EXPECT_EQ(backend.depthRangeCalls, 0); // viewport 0 untouched
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("depth_range_arrayv_sets_contiguous_viewports") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLdouble v[4] = {0.1, 0.9, 0.3, 0.7};
    glDepthRangeArrayv(1, 2, v);
    ctx.flushState();
    EXPECT_EQ(backend.depthRangeIndexedCalls, 2);
    EXPECT_EQ(backend.lastDepthRangeIndexedIndex, 2u);
    EXPECT_EQ(backend.lastDepthNearIndexed, 0.3);
    EXPECT_EQ(backend.lastDepthFarIndexed, 0.7);

    setCurrentContext(nullptr);
}

TEST_CASE("depth_range_indexed_reports_via_get_for_slot_zero") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Slot 0 maps to the non-indexed GL_DEPTH_RANGE query.
    glDepthRangeIndexed(0, 0.15, 0.85);
    double out[2] = {0, 0};
    glGetDoublev(GL_DEPTH_RANGE, out);
    EXPECT_EQ(out[0], 0.15);
    EXPECT_EQ(out[1], 0.85);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("viewport_arrayv_validates_range_and_negative_size") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLfloat v[8] = {0, 0, 1, 1, 0, 0, 1, 1};
    // first + count exceeds MAX_VIEWPORTS (16).
    glViewportArrayv(15, 2, v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // count must be > 0.
    glViewportArrayv(0, 0, v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // null array.
    glViewportArrayv(0, 1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // negative width.
    GLfloat bad[4] = {0, 0, -1, 1};
    glViewportArrayv(0, 1, bad);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    ctx.flushState();
    EXPECT_EQ(backend.viewportIndexedCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("depth_range_indexed_validates_viewport_range") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glDepthRangeIndexed(GLStateTracker::kMaxViewports, 0.0, 1.0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // Last valid index is accepted (use a non-default range so the push is
    // observed; (0.0, 1.0) is the default and would be a no-op).
    glDepthRangeIndexed(GLStateTracker::kMaxViewports - 1, 0.0, 0.5);
    ctx.flushState();
    EXPECT_EQ(backend.depthRangeIndexedCalls, 1);
    EXPECT_EQ(backend.lastDepthRangeIndexedIndex,
              GLStateTracker::kMaxViewports - 1);

    setCurrentContext(nullptr);
}

TEST_CASE("viewport_arrayv_skips_unchanged_slots") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLfloat v[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    glViewportArrayv(1, 2, v);
    ctx.flushState();
    EXPECT_EQ(backend.viewportIndexedCalls, 2);

    // Re-flush with identical values: no redundant pushes.
    glViewportArrayv(1, 2, v);
    ctx.flushState();
    EXPECT_EQ(backend.viewportIndexedCalls, 2);

    // Change only slot 2: only slot 2 is re-pushed.
    GLfloat v2[8] = {10, 20, 30, 40, 5, 6, 7, 8};
    glViewportArrayv(1, 2, v2);
    ctx.flushState();
    EXPECT_EQ(backend.viewportIndexedCalls, 3);
    EXPECT_EQ(backend.lastViewportIndexed, 2u);

    setCurrentContext(nullptr);
}

} // namespace
