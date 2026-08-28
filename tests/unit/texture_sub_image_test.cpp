#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

TEST_CASE("gettexturesubimage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetTextureSubImage(tex, 0, 0, 0, 0, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                         sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getTextureSubImageCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, GL_TEXTURE_2D);
    EXPECT_EQ(mt->lastSubLevel, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("gettexturesubimage_negative_level_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);

    uint8_t buf[16] = {0};
    glGetTextureSubImage(tex, -1, 0, 0, 0, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                         sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("gettexturesubimage_negative_extent_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);

    uint8_t buf[16] = {0};
    glGetTextureSubImage(tex, 0, 0, 0, 0, -2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                         sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glGetTextureSubImage(tex, 0, -1, 0, 0, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                         sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glGetTextureSubImage(tex, 0, 0, 0, 0, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, -1,
                         buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("gettexturesubimage_ungenerated_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[16] = {0};
    glGetTextureSubImage(9999, 0, 0, 0, 0, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                         sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("getcompressedtexturesubimage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetCompressedTextureSubImage(tex, 0, 0, 0, 0, 2, 2, 1, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getCompressedTextureSubImageCalls, 1);

    glGetCompressedTextureSubImage(tex, -1, 0, 0, 0, 2, 2, 1, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glGetCompressedTextureSubImage(9999, 0, 0, 0, 0, 2, 2, 1, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
