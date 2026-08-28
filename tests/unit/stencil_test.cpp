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

TEST_CASE("stencil_separate_faces_pushed_only_when_changed") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Defaults produce no push on either the combined or separate counters.
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 0);
    EXPECT_EQ(backend.stencilFuncSeparateCalls, 0);

    // Setting both faces equal to the default is still a no-op.
    glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xFFFFFFFFu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncSeparateCalls, 0);

    // Changing only the back face pushes a single separate call for BACK.
    glStencilFuncSeparate(GL_BACK, GL_LESS, 3, 0x00FFu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncSeparateCalls, 1);
    EXPECT_EQ(backend.lastStencilFace, static_cast<uint32_t>(GL_BACK));
    EXPECT_EQ(backend.stencilFuncCalls, 0);

    // Re-flush with no further change: nothing new pushed.
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncSeparateCalls, 1);

    // Changing the front face pushes a separate call for FRONT only.
    glStencilFuncSeparate(GL_FRONT, GL_GREATER, 7, 0xFF00u);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncSeparateCalls, 2);
    EXPECT_EQ(backend.lastStencilFace, static_cast<uint32_t>(GL_FRONT));

    // Making both faces equal again collapses to a single combined push.
    glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xFFFFFFFFu);
    ctx.flushState();
    EXPECT_EQ(backend.stencilFuncCalls, 1);
    EXPECT_EQ(backend.stencilFuncSeparateCalls, 2);

    setCurrentContext(nullptr);
}

TEST_CASE("stencil_separate_invalid_face_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glStencilFuncSeparate(0xDEAD, GL_ALWAYS, 0, 0xFFFFFFFFu);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_ENUM));
    glStencilOpSeparate(0xDEAD, GL_KEEP, GL_KEEP, GL_KEEP);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_ENUM));
    glStencilMaskSeparate(0xDEAD, 0xFFFFFFFFu);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_ENUM));

    setCurrentContext(nullptr);
}
