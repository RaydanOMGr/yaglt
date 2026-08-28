#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

TEST_CASE("getcompressedteximage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getCompressedTexImageCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, GL_TEXTURE_2D);
    EXPECT_EQ(mt->lastSubLevel, 0);

    // Negative level -> GL_INVALID_VALUE.
    glGetCompressedTexImage(GL_TEXTURE_2D, -1, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("getcompressedtextureimage_dsa_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetCompressedTextureImage(tex, 0, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getCompressedTexImageCalls, 1);

    // Negative level -> GL_INVALID_VALUE.
    glGetCompressedTextureImage(tex, -1, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Ungenerated texture -> GL_INVALID_OPERATION.
    glGetCompressedTextureImage(9999, 0, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
