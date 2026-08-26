#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("clear_color_records_and_pushes_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default (0,0,0,0) already matches the backend initial state: no push.
    ctx.flushState();
    EXPECT_EQ(backend.clearColorCalls, 0);

    glClearColor(0.1f, 0.2f, 0.3f, 0.4f);
    ctx.flushState();
    EXPECT_EQ(backend.clearColorCalls, 1);
    EXPECT_EQ(backend.lastClearR, 0.1f);
    EXPECT_EQ(backend.lastClearG, 0.2f);
    EXPECT_EQ(backend.lastClearB, 0.3f);
    EXPECT_EQ(backend.lastClearA, 0.4f);

    // Identical re-flush: no further push (SPEC §10).
    glClearColor(0.1f, 0.2f, 0.3f, 0.4f);
    ctx.flushState();
    EXPECT_EQ(backend.clearColorCalls, 1);

    // Changed color: pushed once more.
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    ctx.flushState();
    EXPECT_EQ(backend.clearColorCalls, 2);
    EXPECT_EQ(backend.lastClearR, 1.0f);

    setCurrentContext(nullptr);
}

TEST_CASE("clear_depth_records_and_pushes_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default depth 1.0 matches the backend initial state: no push.
    ctx.flushState();
    EXPECT_EQ(backend.clearDepthCalls, 0);

    glClearDepth(0.5);
    ctx.flushState();
    EXPECT_EQ(backend.clearDepthCalls, 1);
    EXPECT_EQ(backend.lastClearDepth, 0.5);

    // glClearDepthf mirrors glClearDepth (float promoted to double).
    glClearDepthf(0.25f);
    ctx.flushState();
    EXPECT_EQ(backend.clearDepthCalls, 2);
    EXPECT_EQ(backend.lastClearDepth, static_cast<double>(0.25f));

    setCurrentContext(nullptr);
}

TEST_CASE("glClear_issues_native_clear_after_state_flush") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // State flush pushed the clear color, then the backend received the clear.
    EXPECT_EQ(backend.clearColorCalls, 1);
    EXPECT_EQ(backend.clearCalls, 1);
    EXPECT_EQ(backend.lastClearMask,
              static_cast<uint32_t>(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    setCurrentContext(nullptr);
}

TEST_CASE("glClear_invalid_mask_is_gl_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // A bit outside color/depth/stencil is GL_INVALID_VALUE; no native clear.
    glClear(0xDEADBEEF);
    EXPECT_EQ(backend.clearCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // A valid stencil-only clear succeeds.
    glClear(GL_STENCIL_BUFFER_BIT);
    EXPECT_EQ(backend.clearCalls, 1);
    EXPECT_EQ(backend.lastClearMask, static_cast<uint32_t>(GL_STENCIL_BUFFER_BIT));

    setCurrentContext(nullptr);
}

TEST_CASE("glClear_pushes_color_then_clears_at_draw_time") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    glUseProgram(1);
    glDrawArrays(0x0004 /* GL_TRIANGLES */, 0, 3);

    // The draw flushed tracked state, which pushed the clear color.
    EXPECT_EQ(backend.clearColorCalls, 1);
    EXPECT_EQ(backend.lastClearG, 0.4f);

    setCurrentContext(nullptr);
}
