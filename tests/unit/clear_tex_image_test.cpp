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

TEST_CASE("clear_tex_image_records_native_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 3, GL_RGBA8, 32, 16);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glClearTexImage(tex, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->clearTexImageCalls, 1);
    EXPECT_EQ(backend->lastClearTexImageName, tex);
    EXPECT_EQ(backend->lastClearTexImageLevel, 1);
    EXPECT_EQ(backend->lastClearTexImageFormat, static_cast<uint32_t>(GL_RGBA));
    EXPECT_EQ(backend->lastClearTexImageType, static_cast<uint32_t>(GL_UNSIGNED_BYTE));

    setCurrentContext(nullptr);
}

TEST_CASE("clear_tex_sub_image_records_region") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 2, GL_RGBA8, 32, 16);

    glClearTexSubImage(tex, 0, 4, 5, 6, 7, 8, 9, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->clearTexSubImageCalls, 1);
    EXPECT_EQ(backend->lastClearTexSubImageName, tex);
    EXPECT_EQ(backend->lastClearTexSubImageLevel, 0);
    EXPECT_EQ(backend->lastClearTexSubImageX, 4);
    EXPECT_EQ(backend->lastClearTexSubImageY, 5);
    EXPECT_EQ(backend->lastClearTexSubImageZ, 6);
    EXPECT_EQ(backend->lastClearTexSubImageW, 7);
    EXPECT_EQ(backend->lastClearTexSubImageH, 8);
    EXPECT_EQ(backend->lastClearTexSubImageD, 9);

    setCurrentContext(nullptr);
}

TEST_CASE("clear_tex_image_rejects_ungenerated_or_no_storage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    glClearTexImage(9999u, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    GLuint tex = ctx.genTexture(); // generated but no storage yet
    glClearTexImage(tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    EXPECT_EQ(backend->clearTexImageCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("clear_tex_image_rejects_out_of_range_level") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 2, GL_RGBA8, 32, 16);

    glClearTexImage(tex, 5, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // only levels 0..1
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    glClearTexImage(tex, -1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("clear_tex_sub_image_rejects_negative_extent") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 16);

    glClearTexSubImage(tex, 0, 0, 0, 0, -1, 4, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}
