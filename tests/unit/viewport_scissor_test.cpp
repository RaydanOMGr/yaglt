#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_SCISSOR_TEST = 0x0C11;
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

TEST_CASE("viewport_records_and_pushes_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // No flush yet: nothing pushed.
    glViewport(0, 0, 320, 240);
    EXPECT_EQ(backend.viewportCalls, 0);

    // First flush pushes the viewport.
    ctx.flushState();
    EXPECT_EQ(backend.viewportCalls, 1);
    EXPECT_EQ(backend.lastViewportX, 0);
    EXPECT_EQ(backend.lastViewportY, 0);
    EXPECT_EQ(backend.lastViewportW, 320);
    EXPECT_EQ(backend.lastViewportH, 240);

    // Redundant identical viewport: no further push.
    glViewport(0, 0, 320, 240);
    ctx.flushState();
    EXPECT_EQ(backend.viewportCalls, 1);

    // Changed viewport: pushed once more.
    glViewport(10, 20, 640, 480);
    ctx.flushState();
    EXPECT_EQ(backend.viewportCalls, 2);
    EXPECT_EQ(backend.lastViewportX, 10);
    EXPECT_EQ(backend.lastViewportW, 640);

    setCurrentContext(nullptr);
}

TEST_CASE("scissor_box_records_and_pushes_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glScissor(5, 6, 100, 100);
    EXPECT_EQ(backend.scissorCalls, 0);

    ctx.flushState();
    EXPECT_EQ(backend.scissorCalls, 1);
    EXPECT_EQ(backend.lastScissorX, 5);
    EXPECT_EQ(backend.lastScissorY, 6);
    EXPECT_EQ(backend.lastScissorW, 100);
    EXPECT_EQ(backend.lastScissorH, 100);

    glScissor(5, 6, 100, 100);
    ctx.flushState();
    EXPECT_EQ(backend.scissorCalls, 1);

    glScissor(0, 0, 50, 50);
    ctx.flushState();
    EXPECT_EQ(backend.scissorCalls, 2);
    EXPECT_EQ(backend.lastScissorW, 50);

    setCurrentContext(nullptr);
}

TEST_CASE("scissor_test_is_capability_not_box") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Enabling GL_SCISSOR_TEST routes to the capability system (enable/disable),
    // not to the scissor-box push.
    glEnable(GL_SCISSOR_TEST);
    ctx.flushState();
    EXPECT_EQ(backend.enableCalls, 1);
    EXPECT_EQ(backend.lastEnableCap, GL_SCISSOR_TEST);
    EXPECT_EQ(backend.scissorCalls, 0);

    // A scissor box set while the test is disabled still records the box only.
    glScissor(1, 2, 3, 4);
    ctx.flushState();
    EXPECT_EQ(backend.scissorCalls, 1);
    EXPECT_EQ(backend.enableCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("viewport_and_scissor_pushed_together_at_draw") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glViewport(0, 0, 800, 600);
    glScissor(0, 0, 100, 100);
    // Nothing flushed yet.
    EXPECT_EQ(backend.viewportCalls, 0);
    EXPECT_EQ(backend.scissorCalls, 0);

    // A draw flushes tracked state to the backend. An active program is required
    // for the draw to proceed, which exercises the flush path.
    glUseProgram(1);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    EXPECT_EQ(backend.viewportCalls, 1);
    EXPECT_EQ(backend.scissorCalls, 1);
    EXPECT_EQ(backend.drawArraysCalls, 1);

    setCurrentContext(nullptr);
}
