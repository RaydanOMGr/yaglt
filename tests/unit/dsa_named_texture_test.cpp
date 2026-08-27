#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// DSA texture surface (SPEC §2.1 / §8.1): operate on explicit named objects via
// the frontend, forwarded to the named object's backend resource (YAGLT emulates
// DSA for every backend, so the mock must record each call). These exercise the
// Context API directly (no global current-context needed).
TEST_CASE("create_textures_records_target_and_generates_names") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName names[3] = {0, 0, 0};
    ctx.createTextures(GL_TEXTURE_2D, 3, names);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(names[0], 0u);
    EXPECT_NE(names[1], 0u);
    EXPECT_NE(names[2], 0u);
    EXPECT_NE(names[0], names[1]);
    // The created texture's target must be the requested one.
    EXPECT_EQ(ctx.getTexture(names[0])->target, GL_TEXTURE_2D);
}

TEST_CASE("create_textures_invalid_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName names[1] = {0};
    ctx.createTextures(0xDEAD, 1, names);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("texture_storage2d_records_and_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 4, GL_RGBA8, 64, 32);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mt = static_cast<MockTexture*>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->storage2DCalls, 1);
    EXPECT_EQ(mt->lastStorageLevels, 4);
    EXPECT_EQ(mt->lastStorageInternalFormat, GL_RGBA8);
    EXPECT_EQ(mt->lastStorageW, 64);
    EXPECT_EQ(mt->lastStorageH, 32);
    EXPECT_TRUE(ctx.getTexture(tex)->immutableStorage);
}

TEST_CASE("texture_storage_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 0, GL_RGBA8, 64, 32); // levels < 1
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 0, 32); // width < 1
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("texture_subimage2d_requires_storage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    // No storage allocated yet: sub-image must be refused.
    ctx.textureSubImage2D(tex, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    // After storage it is accepted.
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    ctx.textureSubImage2D(tex, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mt = static_cast<MockTexture*>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->texSubImage2DCalls, 1);
}

TEST_CASE("texture_subimage2d_out_of_bounds_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    ctx.textureSubImage2D(tex, 0, 4, 4, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("texture_parameter_roundtrips") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mt = static_cast<MockTexture*>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->texParameteriCalls, 1);
    EXPECT_EQ(mt->lastParamPname, GL_TEXTURE_MIN_FILTER);
    EXPECT_EQ(mt->lastParam, GL_LINEAR);
    GLint got = 0;
    ctx.getTextureParameteriv(tex, GL_TEXTURE_MIN_FILTER, &got);
    EXPECT_EQ(got, GL_LINEAR);
    GLfloat fgot = 0.0f;
    ctx.textureParameterf(tex, GL_TEXTURE_MAX_LOD, 2.0f);
    ctx.getTextureParameterfv(tex, GL_TEXTURE_MAX_LOD, &fgot);
    EXPECT_EQ(fgot, 2.0f);
}

TEST_CASE("generate_texture_mipmap_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.generateTextureMipmap(tex);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mt = static_cast<MockTexture*>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->generateMipmapCalls, 1);
}

TEST_CASE("get_texture_level_parameter_reads_frontend_storage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 3, GL_RGBA8, 64, 32);
    GLint w = 0, h = 0, internal = 0;
    ctx.getTextureLevelParameteriv(tex, 0, GL_TEXTURE_WIDTH, &w);
    ctx.getTextureLevelParameteriv(tex, 0, GL_TEXTURE_HEIGHT, &h);
    ctx.getTextureLevelParameteriv(tex, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal);
    EXPECT_EQ(w, 64);
    EXPECT_EQ(h, 32);
    EXPECT_EQ(internal, GL_RGBA8);
    // Level 1 dims are halved (frontend computes them).
    GLint w1 = 0;
    ctx.getTextureLevelParameteriv(tex, 1, GL_TEXTURE_WIDTH, &w1);
    EXPECT_EQ(w1, 32);
}

TEST_CASE("get_texture_level_parameter_bad_level_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 64, 32);
    GLint v = 0;
    ctx.getTextureLevelParameteriv(tex, 5, GL_TEXTURE_WIDTH, &v); // level >= levels
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("get_texture_image_forwards_to_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    unsigned char buf[64] = {0};
    ctx.getTextureImage(tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mt = static_cast<MockTexture*>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->getTexImageCalls, 1);
}

TEST_CASE("texture_buffer_binds_buffer_object") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    GLObjectName buf = ctx.genBuffer();
    ctx.textureBuffer(tex, GL_RGBA8, buf);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mt = static_cast<MockTexture*>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->textureBufferCalls, 1);
    // The backend receives the buffer's native id (the frontend name is resolved
    // through bindNativeObject; for the mock the native id is the mock's own id).
    GLuint bufNative =
        static_cast<MockBuffer*>(ctx.getBuffer(buf)->backend.get())->nativeId();
    EXPECT_EQ(mt->lastBufferNativeId, bufNative);
    EXPECT_EQ(mt->lastBufferInternalFormat, GL_RGBA8);
    EXPECT_EQ(ctx.getTexture(tex)->target, GL_TEXTURE_BUFFER);
}

TEST_CASE("dsa_unsupported_reports_invalid_operation") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName tex; ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.textureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.generateTextureMipmap(tex);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("dsa_on_ungenerated_name_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.textureStorage2D(99999, 1, GL_RGBA8, 8, 8);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.textureParameteri(99999, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}
