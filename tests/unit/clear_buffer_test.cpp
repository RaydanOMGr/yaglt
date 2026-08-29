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

// Classic (bound-framebuffer) clears push the per-type clear value to the
// backend state sink and then issue a native clear on the bound draw FBO
// (SPEC §9.3.1 / §15.2.3).
TEST_CASE("clear_buffer_fv_color_pushes_clear_color_and_clears") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const float c[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    ctx.clearBufferfv(GL_COLOR, 0, c);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(backend->clearColorCalls >= 1);
    EXPECT_TRUE(backend->lastClearR == 0.1f);
    EXPECT_TRUE(backend->lastClearG == 0.2f);
    EXPECT_TRUE(backend->lastClearB == 0.3f);
    EXPECT_TRUE(backend->lastClearA == 0.4f);
    EXPECT_EQ(backend->lastClearMask, GL_COLOR_BUFFER_BIT);
}

TEST_CASE("clear_buffer_fv_depth_pushes_clear_depth") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const float d[1] = {0.5f};
    ctx.clearBufferfv(GL_DEPTH, 0, d);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(backend->clearDepthCalls >= 1);
    EXPECT_TRUE(backend->lastClearDepth == 0.5);
    EXPECT_EQ(backend->lastClearMask, GL_DEPTH_BUFFER_BIT);
}

TEST_CASE("clear_buffer_iv_color_pushes_clear_color_int") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const int32_t c[4] = {128, 64, 32, 255};
    ctx.clearBufferiv(GL_COLOR, 0, c);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(backend->clearColorCalls >= 1);
    EXPECT_TRUE(backend->lastClearR == 128.0f);
    EXPECT_TRUE(backend->lastClearA == 255.0f);
    EXPECT_EQ(backend->lastClearMask, GL_COLOR_BUFFER_BIT);
}

TEST_CASE("clear_buffer_iv_stencil_clears_stencil_only") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const int32_t s[1] = {7};
    int beforeColor = backend->clearColorCalls;
    ctx.clearBufferiv(GL_STENCIL, 0, s);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->lastClearMask, GL_STENCIL_BUFFER_BIT);
    EXPECT_EQ(backend->clearColorCalls, beforeColor); // stencil has no color value
}

TEST_CASE("clear_buffer_uiv_color_pushes_unsigned_clear_color") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const uint32_t c[4] = {10, 20, 30, 40};
    ctx.clearBufferuiv(GL_COLOR, 0, c);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(backend->clearColorCalls >= 1);
    EXPECT_TRUE(backend->lastClearR == 10.0f);
    EXPECT_EQ(backend->lastClearMask, GL_COLOR_BUFFER_BIT);
}

TEST_CASE("clear_buffer_fi_clears_depth_and_stencil") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.clearBufferfi(GL_DEPTH, 0, 0.25f, 3);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(backend->clearDepthCalls >= 1);
    EXPECT_TRUE(backend->lastClearDepth == 0.25);
    EXPECT_EQ(backend->lastClearMask,
              GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

TEST_CASE("clear_buffer_null_value_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.clearBufferfv(GL_COLOR, 0, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.clearBufferiv(GL_COLOR, 0, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.clearBufferuiv(GL_COLOR, 0, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("clear_buffer_negative_drawbuffer_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const float c[4] = {0, 0, 0, 0};
    ctx.clearBufferfv(GL_COLOR, -1, c);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("clear_buffer_bad_buffer_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const float c[4] = {0, 0, 0, 0};
    // DEPTH is not valid for clearBufferfv-with-color variants; use a clearly
    // wrong buffer token (GL_FRONT) to force GL_INVALID_ENUM.
    const int32_t i[4] = {0, 0, 0, 0};
    ctx.clearBufferfv(GL_FRONT, 0, c);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    // clearBufferiv accepts only COLOR/STENCIL; DEPTH is invalid.
    ctx.clearBufferiv(GL_DEPTH, 0, i);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    // clearBufferuiv accepts only COLOR; DEPTH is invalid.
    const uint32_t u[4] = {0, 0, 0, 0};
    ctx.clearBufferuiv(GL_DEPTH, 0, u);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    // clearBufferfi accepts only DEPTH; COLOR is invalid.
    ctx.clearBufferfi(GL_COLOR, 0, 0.0f, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("clear_buffer_via_public_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    const float c[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    // The public gl* entry point forwards to the context method (SPEC §9.3.1).
    glClearBufferfv(GL_COLOR, 0, c);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(backend->clearColorCalls >= 1);
    EXPECT_TRUE(backend->lastClearR == 1.0f);
    EXPECT_EQ(backend->lastClearMask, GL_COLOR_BUFFER_BIT);
    setCurrentContext(nullptr);
}
