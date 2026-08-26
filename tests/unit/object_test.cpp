#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("context_gen_buffer_unique_names_and_backend_resource") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName a = ctx.genBuffer();
    GLObjectName b = ctx.genBuffer();
    EXPECT_NE(a, b);
    EXPECT_EQ(a, 1u);
    EXPECT_EQ(b, 2u);

    BufferObject* ao = ctx.getBuffer(a);
    EXPECT_NE(ao, nullptr);
    EXPECT_NE(static_cast<MockBuffer*>(ao->backend.get()), nullptr);
    EXPECT_EQ(static_cast<MockBuffer*>(ao->backend.get())->id, 1);
}

TEST_CASE("context_bind_buffer_tracks_current_and_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();

    ctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, buf); // 0x8892 = GL_ARRAY_BUFFER
    EXPECT_EQ(ctx.boundBuffer(0x8892), buf);
    EXPECT_EQ(ctx.getBuffer(buf)->target, 0x8892u);

    // binding 0 unbinds
    ctx.bindBuffer(0x8892, 0);
    EXPECT_EQ(ctx.boundBuffer(0x8892), 0u);
}

TEST_CASE("context_bind_ungenerated_name_sets_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindBuffer(0x8892, 999);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("context_delete_buffer_resets_binding_and_lookup") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(0x8892, buf);
    EXPECT_EQ(ctx.boundBuffer(0x8892), buf);

    ctx.deleteBuffer(buf);
    EXPECT_EQ(ctx.getBuffer(buf), nullptr);
    EXPECT_EQ(ctx.boundBuffer(0x8892), 0u);

    // deleting unused name is a no-op, no error
    ctx.deleteBuffer(12345);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
}

TEST_CASE("context_texture_lifecycle") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName t = ctx.genTexture();
    EXPECT_EQ(t, 1u);
    ctx.bindTexture(t);
    EXPECT_EQ(ctx.boundTexture(), t);
    EXPECT_NE(ctx.getTexture(t)->backend.get(), nullptr);
    ctx.deleteTexture(t);
    EXPECT_EQ(ctx.getTexture(t), nullptr);
    EXPECT_EQ(ctx.boundTexture(), 0u);
}

TEST_CASE("context_renderbuffer_framebuffer_vertexarray_lifecycle") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName rb = ctx.genRenderbuffer();
    ctx.bindRenderbuffer(rb);
    EXPECT_EQ(ctx.boundRenderbuffer(), rb);
    ctx.deleteRenderbuffer(rb);
    EXPECT_EQ(ctx.boundRenderbuffer(), 0u);

    GLObjectName fb = ctx.genFramebuffer();
    ctx.bindFramebuffer(fb);
    EXPECT_EQ(ctx.boundFramebuffer(), fb);
    ctx.deleteFramebuffer(fb);
    EXPECT_EQ(ctx.boundFramebuffer(), 0u);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);
    EXPECT_EQ(ctx.boundVertexArray(), vao);
    ctx.deleteVertexArray(vao);
    EXPECT_EQ(ctx.boundVertexArray(), 0u);
}

TEST_CASE("context_get_error_clears_after_read") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindBuffer(0x8892, 7); // invalid
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(ctx.getError(), GLError::NoError); // cleared
}
