#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("draw_buffers_pushes_to_sink_on_flush") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLenum buf = GL_COLOR_ATTACHMENT0;
    ctx.drawBuffers(1, &buf);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->drawBuffersCalls, 0); // deferred until flush

    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 1);
    EXPECT_EQ(backend->lastDrawBuffersN, 1);
    EXPECT_EQ(backend->lastDrawBuffers.size(), 1u);
    EXPECT_EQ(backend->lastDrawBuffers[0], GL_COLOR_ATTACHMENT0);
}

TEST_CASE("draw_buffers_change_only_pushes_once") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLenum b0 = GL_COLOR_ATTACHMENT0, b1 = GL_COLOR_ATTACHMENT1;
    ctx.drawBuffers(1, &b0);
    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 1);

    // Identical selection: no additional push on the next flush.
    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 1);

    // Different selection: pushes again.
    ctx.drawBuffers(1, &b1);
    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 2);
    EXPECT_EQ(backend->lastDrawBuffers[0], GL_COLOR_ATTACHMENT1);
}

TEST_CASE("draw_buffers_nonpositive_count_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLenum buf = GL_BACK;
    ctx.drawBuffers(0, &buf);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("draw_buffers_invalid_buffer_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLenum bad = 0xDEAD;
    ctx.drawBuffers(1, &bad);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("read_buffer_pushes_to_sink_on_flush") {
    auto backend = makeBackend();
    Context ctx(*backend);
    // Default read buffer is GL_BACK, so use a different value to observe a push.
    ctx.readBuffer(GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->readBufferCalls, 0);

    ctx.flushState();
    EXPECT_EQ(backend->readBufferCalls, 1);
    EXPECT_EQ(backend->lastReadBuffer, GL_COLOR_ATTACHMENT0);
}

TEST_CASE("read_buffer_invalid_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.readBuffer(0xBEEF);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("gl_api_draw_read_buffer_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLenum buf = GL_COLOR_ATTACHMENT0;
    ctx.drawBuffers(1, &buf);
    GLenum rb = GL_COLOR_ATTACHMENT0;
    glReadBuffer(rb);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 1);
    EXPECT_EQ(backend->readBufferCalls, 1);
    setCurrentContext(nullptr);
}

TEST_CASE("draw_buffer_pushes_single_to_sink_on_flush") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.drawBuffer(GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->drawBuffersCalls, 0); // deferred until flush

    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 1);
    EXPECT_EQ(backend->lastDrawBuffersN, 1);
    EXPECT_EQ(backend->lastDrawBuffers.size(), 1u);
    EXPECT_EQ(backend->lastDrawBuffers[0], GL_COLOR_ATTACHMENT0);
}

TEST_CASE("draw_buffer_invalid_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.drawBuffer(0xFEED);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("gl_api_draw_buffer_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ctx.flushState();
    EXPECT_EQ(backend->drawBuffersCalls, 1);
    EXPECT_EQ(backend->lastDrawBuffers[0], GL_COLOR_ATTACHMENT0);
    setCurrentContext(nullptr);
}
