#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glNamedFramebufferDrawBuffer_forwards_single_buffer") {
    MockBackend backend;
    backend.initialize();
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName fb = 0;
    glCreateFramebuffers(1, &fb);
    EXPECT_NE(fb, 0u);

    glNamedFramebufferDrawBuffer(fb, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawBuffersCalls, 1);
    EXPECT_EQ(backend.lastDrawBuffersN, 1);
    EXPECT_EQ(backend.lastDrawBuffers.size(), 1u);
    EXPECT_EQ(backend.lastDrawBuffers[0], static_cast<uint32_t>(GL_COLOR_ATTACHMENT0));
    EXPECT_EQ(backend.readBufferCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("glNamedFramebufferDrawBuffers_forwards_multi_buffer") {
    MockBackend backend;
    backend.initialize();
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName fb = 0;
    glCreateFramebuffers(1, &fb);

    GLenum bufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glNamedFramebufferDrawBuffers(fb, 2, bufs);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawBuffersCalls, 1);
    EXPECT_EQ(backend.lastDrawBuffersN, 2);
    EXPECT_EQ(backend.lastDrawBuffers.size(), 2u);
    EXPECT_EQ(backend.lastDrawBuffers[0], static_cast<uint32_t>(GL_COLOR_ATTACHMENT0));
    EXPECT_EQ(backend.lastDrawBuffers[1], static_cast<uint32_t>(GL_COLOR_ATTACHMENT1));

    setCurrentContext(nullptr);
}

TEST_CASE("glNamedFramebufferDrawBuffers_validates_args") {
    MockBackend backend;
    backend.initialize();
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName fb = 0;
    glCreateFramebuffers(1, &fb);
    int callsBefore = backend.drawBuffersCalls;

    // Invalid draw-buffer token.
    GLenum bad[] = {GL_FRONT};
    glNamedFramebufferDrawBuffers(fb, 1, bad);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    EXPECT_EQ(backend.drawBuffersCalls, callsBefore);

    // Negative count.
    glNamedFramebufferDrawBuffers(fb, -1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Null bufs with n > 0.
    glNamedFramebufferDrawBuffers(fb, 2, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("glNamedFramebufferReadBuffer_forwards_valid_token") {
    MockBackend backend;
    backend.initialize();
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName fb = 0;
    glCreateFramebuffers(1, &fb);

    glNamedFramebufferReadBuffer(fb, GL_BACK);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.readBufferCalls, 1);
    EXPECT_EQ(backend.lastReadBuffer, static_cast<uint32_t>(GL_BACK));

    glNamedFramebufferReadBuffer(fb, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.readBufferCalls, 2);
    EXPECT_EQ(backend.lastReadBuffer, static_cast<uint32_t>(GL_COLOR_ATTACHMENT0));

    setCurrentContext(nullptr);
}

TEST_CASE("glNamedFramebufferReadBuffer_rejects_invalid_token") {
    MockBackend backend;
    backend.initialize();
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName fb = 0;
    glCreateFramebuffers(1, &fb);
    int callsBefore = backend.readBufferCalls;

    // Out of range color attachment.
    glNamedFramebufferReadBuffer(fb, GL_COLOR_ATTACHMENT0 + 0x10);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    EXPECT_EQ(backend.readBufferCalls, callsBefore);

    setCurrentContext(nullptr);
}

TEST_CASE("glNamedFramebufferDrawBuffer_rejects_ungenerated_name") {
    MockBackend backend;
    backend.initialize();
    Context ctx(backend);
    setCurrentContext(&ctx);

    glNamedFramebufferDrawBuffer(9999, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend.drawBuffersCalls, 0);

    glNamedFramebufferReadBuffer(9999, GL_BACK);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend.readBufferCalls, 0);

    setCurrentContext(nullptr);
}
