#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("gl_api_buffer_gen_bind_data_error") {
    auto backend = std::make_unique<MockBackend>();
    backend->initialize();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint b = 0;
    glGenBuffers(1, &b);
    EXPECT_NE(b, 0u);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glBindBuffer(GL_ARRAY_BUFFER, b);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glBufferData(GL_ARRAY_BUFFER, 1024, nullptr, GL_STATIC_DRAW);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ctx.getBuffer(b)->size, 1024);
    EXPECT_EQ(ctx.getBuffer(b)->usage, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 4242);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glDeleteBuffers(1, &b);
    EXPECT_EQ(ctx.getBuffer(b), nullptr);
    glcompat::setCurrentContext(nullptr);
}

TEST_CASE("gl_api_buffer_bulk_gen_unique") {
    auto backend = std::make_unique<MockBackend>();
    backend->initialize();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint bufs[3] = {0, 0, 0};
    glGenBuffers(3, bufs);
    EXPECT_NE(bufs[0], bufs[1]);
    EXPECT_NE(bufs[1], bufs[2]);
    EXPECT_NE(bufs[0], bufs[2]);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glcompat::setCurrentContext(nullptr);
}

TEST_CASE("gl_api_texture_renderbuffer_framebuffer_vertexarray") {
    auto backend = std::make_unique<MockBackend>();
    backend->initialize();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint t = 0, rb = 0, fb = 0, vao = 0;
    glGenTextures(1, &t);
    glGenRenderbuffers(1, &rb);
    glGenFramebuffers(1, &fb);
    glGenVertexArrays(1, &vao);

    glBindTexture(GL_TEXTURE_2D, t);
    EXPECT_EQ(ctx.boundTextureForTarget(GL_TEXTURE_2D), t);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    EXPECT_EQ(ctx.boundRenderbuffer(), rb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    EXPECT_EQ(ctx.boundFramebuffer(), fb);
    glBindVertexArray(vao);
    EXPECT_EQ(ctx.boundVertexArray(), vao);

    glDeleteTextures(1, &t);
    glDeleteRenderbuffers(1, &rb);
    glDeleteFramebuffers(1, &fb);
    glDeleteVertexArrays(1, &vao);

    EXPECT_EQ(ctx.getTexture(t), nullptr);
    EXPECT_EQ(ctx.getVertexArray(vao), nullptr);
    glcompat::setCurrentContext(nullptr);
}

TEST_CASE("gl_api_no_current_context_returns_error") {
    glcompat::setCurrentContext(nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    GLuint b = 0;
    glGenBuffers(1, &b); // no-op, no crash
    EXPECT_EQ(b, 0u);
}
