#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
} // namespace

TEST_CASE("state_flush_pushes_enable_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // No apply yet: setCapability only records; no native call.
    glEnable(GL_BLEND);
    EXPECT_EQ(backend.enableCalls, 0);
    EXPECT_EQ(backend.disableCalls, 0);

    // First flush pushes the enable.
    ctx.flushState();
    EXPECT_EQ(backend.enableCalls, 1);
    EXPECT_EQ(backend.lastEnableCap, GL_BLEND);
    EXPECT_EQ(backend.disableCalls, 0);

    // Redundant enable: tracker sees no change, flush is a no-op.
    glEnable(GL_BLEND);
    ctx.flushState();
    EXPECT_EQ(backend.enableCalls, 1);
    EXPECT_EQ(backend.disableCalls, 0);

    // Disable flips it: flush pushes a single disable.
    glDisable(GL_BLEND);
    ctx.flushState();
    EXPECT_EQ(backend.disableCalls, 1);
    EXPECT_EQ(backend.lastDisableCap, GL_BLEND);

    // Redundant disable: no further native call.
    glDisable(GL_BLEND);
    ctx.flushState();
    EXPECT_EQ(backend.disableCalls, 1);
    EXPECT_EQ(backend.enableCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("state_flush_pushes_other_state_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFunc(0x0302 /*GL_SRC_ALPHA*/, 0x0303 /*GL_ONE_MINUS_SRC_ALPHA*/);
    glUseProgram(5);
    glDepthFunc(0x0203 /*GL_LEQUAL*/);
    glCullFace(0x0404 /*GL_FRONT*/);
    glFrontFace(0x0900 /*GL_CW*/);

    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 1);
    EXPECT_EQ(backend.blendEquationCalls, 1);
    EXPECT_EQ(backend.useProgramCalls, 1);
    EXPECT_EQ(backend.lastProgram, 5u);
    EXPECT_EQ(backend.depthFuncCalls, 1);
    EXPECT_EQ(backend.cullFaceCalls, 1);
    EXPECT_EQ(backend.frontFaceCalls, 1);

    // Re-flush with identical state: nothing re-pushed.
    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 1);
    EXPECT_EQ(backend.useProgramCalls, 1);
    EXPECT_EQ(backend.depthFuncCalls, 1);
    EXPECT_EQ(backend.cullFaceCalls, 1);
    EXPECT_EQ(backend.frontFaceCalls, 1);

    // Change one value: only that category is pushed again.
    glBlendFunc(1 /*GL_ONE*/, 0 /*GL_ZERO*/);
    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 2);
    EXPECT_EQ(backend.useProgramCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("state_flush_is_noop_without_sink") {
    // The mock always provides a sink; verify the public API path is safe when
    // no context is current (guards against null deref).
    setCurrentContext(nullptr);
    glFlushState(); // must not crash
}
