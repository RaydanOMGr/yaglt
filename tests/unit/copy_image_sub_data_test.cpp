#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

static GLuint makeTex2D(Context& ctx, GLuint internalFormat, int w, int h,
                        int levels = 1) {
    GLuint tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, levels, internalFormat, w, h);
    ctx.bindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static GLuint makeRB(Context& ctx, GLuint internalFormat, int w, int h) {
    GLuint rb = 0;
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, internalFormat, w, h);
    return rb;
}

TEST_CASE("copy_image_sub_data_records_native_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA8, 32, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA8, 32, 16);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glCopyImageSubData(src, GL_TEXTURE_2D, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0,
                       0, 0, 16, 8, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->copyImageSubDataCalls, 1);
    EXPECT_EQ(backend->lastCopySrcName, src);
    EXPECT_EQ(backend->lastCopySrcTarget, static_cast<uint32_t>(GL_TEXTURE_2D));
    EXPECT_EQ(backend->lastCopyDstName, dst);
    EXPECT_EQ(backend->lastCopyDstTarget, static_cast<uint32_t>(GL_TEXTURE_2D));
    EXPECT_EQ(backend->lastCopyWidth, 16);
    EXPECT_EQ(backend->lastCopyHeight, 8);
    EXPECT_EQ(backend->lastCopyDepth, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_class_compatible_formats") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA32F, 16, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA32I, 16, 16);

    glCopyImageSubData(src, GL_TEXTURE_2D, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0,
                       0, 0, 16, 16, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->copyImageSubDataCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_incompatible_formats") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA8, 16, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA32F, 16, 16);

    glCopyImageSubData(src, GL_TEXTURE_2D, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0,
                       0, 0, 16, 16, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend->copyImageSubDataCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_invalid_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA8, 16, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA8, 16, 16);

    glCopyImageSubData(src, GL_TEXTURE_BUFFER, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0,
                       0, 0, 0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_unknown_name") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint dst = makeTex2D(ctx, GL_RGBA8, 16, 16);

    glCopyImageSubData(7777u, GL_TEXTURE_2D, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0,
                       0, 0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_target_object_type_mismatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint tex = makeTex2D(ctx, GL_RGBA8, 16, 16);
    GLuint rb = makeRB(ctx, GL_RGBA8, 16, 16);

    // RENDERBUFFER target but name is a texture -> INVALID_ENUM
    glCopyImageSubData(tex, GL_RENDERBUFFER, 0, 0, 0, 0, rb, GL_RENDERBUFFER, 0, 0,
                       0, 0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_out_of_range_level") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA8, 16, 16, 2);
    GLuint dst = makeTex2D(ctx, GL_RGBA8, 16, 16, 2);

    glCopyImageSubData(src, GL_TEXTURE_2D, 5, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0, 0,
                       0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    glCopyImageSubData(src, GL_TEXTURE_2D, -1, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0,
                       0, 0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_negative_extent") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA8, 16, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA8, 16, 16);

    glCopyImageSubData(src, GL_TEXTURE_2D, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0, 0,
                       0, -4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_rejects_out_of_bounds_region") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeTex2D(ctx, GL_RGBA8, 16, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA8, 16, 16);

    glCopyImageSubData(src, GL_TEXTURE_2D, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0, 0,
                       0, 32, 4, 1); // width 32 exceeds 16
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_renderbuffer_level_must_be_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeRB(ctx, GL_RGBA8, 16, 16);
    GLuint dst = makeRB(ctx, GL_RGBA8, 16, 16);

    glCopyImageSubData(src, GL_RENDERBUFFER, 1, 0, 0, 0, dst, GL_RENDERBUFFER, 0, 0,
                       0, 0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    EXPECT_EQ(backend->copyImageSubDataCalls, 0);

    glCopyImageSubData(src, GL_RENDERBUFFER, 0, 0, 0, 0, dst, GL_RENDERBUFFER, 0, 0,
                       0, 0, 4, 4, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->copyImageSubDataCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("copy_image_sub_data_renderbuffer_to_texture") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint src = makeRB(ctx, GL_RGBA8, 16, 16);
    GLuint dst = makeTex2D(ctx, GL_RGBA8, 16, 16);

    glCopyImageSubData(src, GL_RENDERBUFFER, 0, 0, 0, 0, dst, GL_TEXTURE_2D, 0, 0,
                       0, 0, 16, 16, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->copyImageSubDataCalls, 1);
    EXPECT_EQ(backend->lastCopySrcTarget,
              static_cast<uint32_t>(GL_RENDERBUFFER));
    EXPECT_EQ(backend->lastCopyDstTarget,
              static_cast<uint32_t>(GL_TEXTURE_2D));

    setCurrentContext(nullptr);
}
