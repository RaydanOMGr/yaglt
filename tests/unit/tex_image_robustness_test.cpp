#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

TEST_CASE("getnteximage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    glBindTexture(GL_TEXTURE_2D, tex);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetnTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getTexImageRobustCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, GL_TEXTURE_2D);
    EXPECT_EQ(mt->lastSubLevel, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("getnteximage_negative_level_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    glBindTexture(GL_TEXTURE_2D, tex);

    uint8_t buf[16] = {0};
    glGetnTexImage(GL_TEXTURE_2D, -1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("getnteximage_negative_bufsize_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    glBindTexture(GL_TEXTURE_2D, tex);

    uint8_t buf[16] = {0};
    glGetnTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, -1, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("getnteximage_no_bound_texture_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    uint8_t buf[16] = {0};
    glGetnTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("getntextureimage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetnTextureImage(tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getTexImageRobustCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("getncompressedteximage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    glBindTexture(GL_TEXTURE_2D, tex);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetnCompressedTexImage(GL_TEXTURE_2D, 0, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getCompressedTexImageRobustCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("getncompressedtextureimage_records_backend_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint8_t buf[16] = {0};
    glGetnCompressedTextureImage(tex, 0, sizeof(buf), buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TextureObject* to = ctx.getTexture(tex);
    auto* mt = static_cast<MockTexture*>(to->backend.get());
    EXPECT_EQ(mt->getCompressedTexImageRobustCalls, 1);

    setCurrentContext(nullptr);
}
