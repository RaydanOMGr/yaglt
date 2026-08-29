#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("clear_stencil_records_and_pushes_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default 0 matches the backend initial state: no push.
    ctx.flushState();
    EXPECT_EQ(backend.clearStencilCalls, 0);

    glClearStencil(5);
    ctx.flushState();
    EXPECT_EQ(backend.clearStencilCalls, 1);
    EXPECT_EQ(backend.lastClearStencil, 5);

    // Identical re-flush: no further push (SPEC §10).
    glClearStencil(5);
    ctx.flushState();
    EXPECT_EQ(backend.clearStencilCalls, 1);

    // Changed value: pushed once more.
    glClearStencil(128);
    ctx.flushState();
    EXPECT_EQ(backend.clearStencilCalls, 2);
    EXPECT_EQ(backend.lastClearStencil, 128);

    // Resetting to the default 0 changes state, so it is pushed once more.
    glClearStencil(0);
    ctx.flushState();
    EXPECT_EQ(backend.clearStencilCalls, 3);
    EXPECT_EQ(backend.lastClearStencil, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("glClearStencil_via_public_dispatch_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glClearStencil(42);
    ctx.flushState();
    EXPECT_EQ(backend.clearStencilCalls, 1);
    EXPECT_EQ(backend.lastClearStencil, 42);

    setCurrentContext(nullptr);
}

TEST_CASE("glGetIntegerv_STENCIL_CLEAR_VALUE_returns_set_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default stencil clear value is 0.
    GLint v = -1;
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &v);
    EXPECT_EQ(v, 0);

    glClearStencil(77);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &v);
    EXPECT_EQ(v, 77);

    setCurrentContext(nullptr);
}

TEST_CASE("clearBufferiv_stencil_pushes_to_sink") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint s = 13;
    ctx.clearBufferiv(GL_STENCIL, 0, &s);
    EXPECT_EQ(backend.clearStencilCalls, 1);
    EXPECT_EQ(backend.lastClearStencil, 13);
    EXPECT_EQ(backend.clearCalls, 1);
    EXPECT_EQ(backend.lastClearMask,
              static_cast<uint32_t>(GL_STENCIL_BUFFER_BIT));

    setCurrentContext(nullptr);
}

TEST_CASE("clearBufferfi_pushes_stencil_to_sink") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    ctx.clearBufferfi(GL_DEPTH, 0, 0.5f, 9);
    EXPECT_EQ(backend.clearStencilCalls, 1);
    EXPECT_EQ(backend.lastClearStencil, 9);
    EXPECT_EQ(backend.clearDepthCalls, 1);
    EXPECT_EQ(backend.lastClearDepth, static_cast<double>(0.5));
    EXPECT_EQ(backend.lastClearMask,
              static_cast<uint32_t>(GL_DEPTH_BUFFER_BIT |
                                     GL_STENCIL_BUFFER_BIT));

    setCurrentContext(nullptr);
}
