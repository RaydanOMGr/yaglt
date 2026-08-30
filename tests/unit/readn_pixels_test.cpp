#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glReadnPixels_forwards_to_backend_after_state_flush") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[4] = {0};
    glReadnPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);

    // The read flushed tracked state, then the backend received the robust read.
    EXPECT_EQ(backend.readnPixelsCalls, 1);
    EXPECT_EQ(backend.lastReadnX, 0);
    EXPECT_EQ(backend.lastReadnY, 0);
    EXPECT_EQ(backend.lastReadnW, 1);
    EXPECT_EQ(backend.lastReadnH, 1);
    EXPECT_EQ(backend.lastReadnFormat, static_cast<uint32_t>(GL_RGBA));
    EXPECT_EQ(backend.lastReadnType, static_cast<uint32_t>(GL_UNSIGNED_BYTE));
    EXPECT_EQ(backend.lastReadnBufSize, static_cast<int32_t>(sizeof(buf)));
    EXPECT_EQ(backend.lastReadnPixels, static_cast<void*>(buf));

    setCurrentContext(nullptr);
}

TEST_CASE("glReadnPixels_invalid_size_is_gl_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[4] = {0};
    glReadnPixels(0, 0, 0, 1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);
    EXPECT_EQ(backend.readnPixelsCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    glReadnPixels(0, 0, 1, -1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);
    EXPECT_EQ(backend.readnPixelsCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("glReadnPixels_negative_bufsize_is_gl_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[4] = {0};
    glReadnPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, -1, buf);
    EXPECT_EQ(backend.readnPixelsCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}
