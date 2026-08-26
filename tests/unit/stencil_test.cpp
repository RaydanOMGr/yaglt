#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
// Stencil constants (GL_LESS, GL_ALWAYS, GL_KEEP, GL_REPLACE, GL_INCR, GL_DECR,
// …) now live in glcompat:: (gl_types.hpp) and are pulled in via the using
// directive above.
} // namespace

TEST_CASE("stencil_state_pushed_only_when_category_changes") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Defaults (func=ALWAYS, ref=0, mask=all-ones; ops=KEEP) produce no push.
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFFu);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFFFFFFFFu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 0);
    EXPECT_EQ(backend.stencilOpCalls, 0);
    EXPECT_EQ(backend.stencilMaskCalls, 0);

    // Changing the func pushes the whole stencil category (func+op+mask) once.
    glStencilFunc(GL_LESS, 1, 0x00FFu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 1);
    EXPECT_EQ(backend.stencilOpCalls, 1);
    EXPECT_EQ(backend.stencilMaskCalls, 1);

    // Identical re-flush is skipped.
    glStencilFunc(GL_LESS, 1, 0x00FFu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 1);
    EXPECT_EQ(backend.stencilOpCalls, 1);
    EXPECT_EQ(backend.stencilMaskCalls, 1);

    // Changing ops + mask pushes the whole category again (each counter +1).
    glStencilOp(GL_REPLACE, GL_INCR, GL_DECR);
    glStencilMask(0x0F0Fu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 2);
    EXPECT_EQ(backend.stencilOpCalls, 2);
    EXPECT_EQ(backend.stencilMaskCalls, 2);

    // Re-flush with same values: nothing new pushed.
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 2);
    EXPECT_EQ(backend.stencilOpCalls, 2);
    EXPECT_EQ(backend.stencilMaskCalls, 2);

    setCurrentContext(nullptr);
}
