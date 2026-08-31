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

template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

constexpr GLenum GL_COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1;

// --- Compressed texture image upload (SPEC §8.6) ---

TEST_CASE("compressedTexImage2D_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, 8, 0,
                          32, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->compressedTexImage2DCalls, 1);
    EXPECT_EQ(mt->lastCompressedTarget, static_cast<uint32_t>(GL_TEXTURE_2D));
    EXPECT_EQ(mt->lastCompressedLevel, 0);
    EXPECT_EQ(mt->lastCompressedInternalFormat,
              static_cast<uint32_t>(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT));
    EXPECT_EQ(mt->lastCompressedWidth, 8);
    EXPECT_EQ(mt->lastCompressedHeight, 8);
    EXPECT_EQ(mt->lastCompressedBorder, 0);
    EXPECT_EQ(mt->lastCompressedImageSize, 32);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexImage3D_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_3D, tex);
    glCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 4, 4, 4, 0,
                          16, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->compressedTexImage3DCalls, 1);
    EXPECT_EQ(mt->lastCompressedWidth, 4);
    EXPECT_EQ(mt->lastCompressedHeight, 4);
    EXPECT_EQ(mt->lastCompressedDepth, 4);
    EXPECT_EQ(mt->lastCompressedImageSize, 16);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexSubImage2D_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    // Allocate the level first (mutable path requires a prior image).
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, 8, 0,
                          32, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 4, 4,
                             GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->compressedTexSubImage2DCalls, 1);
    EXPECT_EQ(mt->lastCompressedXoffset, 1);
    EXPECT_EQ(mt->lastCompressedYoffset, 1);
    EXPECT_EQ(mt->lastCompressedWidth, 4);
    EXPECT_EQ(mt->lastCompressedHeight, 4);
    EXPECT_EQ(mt->lastCompressedFormat,
              static_cast<uint32_t>(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT));
    EXPECT_EQ(mt->lastCompressedImageSize, 8);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTextureSubImage2D_dsa_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    // Immutable storage so the level is considered allocated for the sub-upload.
    glTextureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glCompressedTextureSubImage2D(tex, 0, 0, 0, 8, 8,
                                 GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 32, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->compressedTexSubImage2DCalls, 1);
    EXPECT_EQ(mt->lastCompressedWidth, 8);
    EXPECT_EQ(mt->lastCompressedHeight, 8);
    EXPECT_EQ(mt->lastCompressedImageSize, 32);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexImage_rejects_no_bound_texture") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glBindTexture(GL_TEXTURE_2D, 9999); // synthetic name, no texture object
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, 8, 0,
                          32, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexImage_rejects_rectangle_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_RECTANGLE, tex);
    glCompressedTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8,
                           8, 0, 32, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexImage_rejects_nonzero_border") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, 8, 1,
                          32, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexImage_rejects_negative_dimension") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, -8, 8, 0,
                          32, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    setCurrentContext(nullptr);
}

TEST_CASE("compressedTexSubImage_rejects_unallocated_level") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4,
                             GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    setCurrentContext(nullptr);
}
