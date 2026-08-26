#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glReadPixels_forwards_to_backend_after_state_flush") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[4] = {0};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);

    // The read flushed tracked state, then the backend received the read.
    EXPECT_EQ(backend.readPixelsCalls, 1);
    EXPECT_EQ(backend.lastReadX, 0);
    EXPECT_EQ(backend.lastReadY, 0);
    EXPECT_EQ(backend.lastReadW, 1);
    EXPECT_EQ(backend.lastReadH, 1);
    EXPECT_EQ(backend.lastReadFormat, static_cast<uint32_t>(GL_RGBA));
    EXPECT_EQ(backend.lastReadType, static_cast<uint32_t>(GL_UNSIGNED_BYTE));
    EXPECT_EQ(backend.lastReadPixels, static_cast<void*>(buf));

    setCurrentContext(nullptr);
}

TEST_CASE("glReadPixels_invalid_size_is_gl_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[4] = {0};
    glReadPixels(0, 0, 0, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    EXPECT_EQ(backend.readPixelsCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    glReadPixels(0, 0, 1, -1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    EXPECT_EQ(backend.readPixelsCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}
