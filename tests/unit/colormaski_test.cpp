#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "glcompat/state/gl_state.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

namespace {

TEST_CASE("colormaski_nonzero_buffer_uses_indexed_path") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default mask is (true,true,true,true); a different mask pushes buf 2.
    glColorMaski(2, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskiCalls, 1);
    EXPECT_EQ(backend.lastColorMaskiBuf, 2u);
    EXPECT_EQ(backend.lastColorMaskiR, false);
    EXPECT_EQ(backend.lastColorMaskiA, false);
    EXPECT_EQ(backend.colorMaskCalls, 0); // buffer 0 untouched
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("colormaski_buffer0_is_nonindexed_path") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskCalls, 1);
    EXPECT_EQ(backend.lastColorMaskR, false);
    EXPECT_EQ(backend.colorMaskiCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("colormask_nonindexed_sets_every_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // glColorMask writes the same mask to all draw buffers; buffer 0 is pushed
    // through the single-buffer sink and buffers 1..n through the indexed sink.
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskCalls, 1);
    EXPECT_EQ(backend.colorMaskiCalls,
              static_cast<int>(GLStateTracker::kMaxDrawBuffers - 1));
    EXPECT_EQ(backend.lastColorMaskiBuf,
              GLStateTracker::kMaxDrawBuffers - 1);
    EXPECT_EQ(backend.lastColorMaskiG, true);

    // Re-flush with identical mask: no redundant pushes.
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskCalls, 1);
    EXPECT_EQ(backend.colorMaskiCalls,
              static_cast<int>(GLStateTracker::kMaxDrawBuffers - 1));

    setCurrentContext(nullptr);
}

TEST_CASE("colormaski_validates_buffer_range") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glColorMaski(GLStateTracker::kMaxDrawBuffers, GL_FALSE, GL_FALSE, GL_FALSE,
                 GL_FALSE);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Last valid index is accepted.
    glColorMaski(GLStateTracker::kMaxDrawBuffers - 1, GL_FALSE, GL_FALSE,
                 GL_FALSE, GL_FALSE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskiCalls, 1);
    EXPECT_EQ(backend.lastColorMaskiBuf,
              GLStateTracker::kMaxDrawBuffers - 1);

    setCurrentContext(nullptr);
}

TEST_CASE("colormaski_skips_unchanged_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glColorMaski(3, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskiCalls, 1);

    // Same mask again: no push.
    glColorMaski(3, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskiCalls, 1);

    // Changed mask: re-push.
    glColorMaski(3, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    ctx.flushState();
    EXPECT_EQ(backend.colorMaskiCalls, 2);
    EXPECT_EQ(backend.lastColorMaskiR, true);

    setCurrentContext(nullptr);
}

TEST_CASE("colormask_reports_via_get") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    GLint iv[4] = {0};
    glGetIntegerv(GL_COLOR_WRITEMASK, iv);
    EXPECT_EQ(iv[0], 1);
    EXPECT_EQ(iv[1], 0);
    EXPECT_EQ(iv[2], 1);
    EXPECT_EQ(iv[3], 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

} // namespace
