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

// --- Non-DSA immutable texture storage (SPEC §8.5) ---

TEST_CASE("texStorage1D_allocates_immutable_1d") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_1D, tex);
    ctx.texStorage1D(GL_TEXTURE_1D, 1, GL_RGBA8, 16);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_TRUE(t->immutableStorage);
    EXPECT_EQ(t->storageLevels, 1);
    EXPECT_EQ(t->storageBaseWidth, 16);
    EXPECT_EQ(t->storageInternalFormat, static_cast<uint32_t>(GL_RGBA8));

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->storage1DCalls, 1);
    EXPECT_EQ(mt->lastStorageTarget, static_cast<uint32_t>(GL_TEXTURE_1D));
    EXPECT_EQ(mt->lastStorageW, 16);
}

TEST_CASE("texStorage2D_allocates_immutable_2d") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 3, GL_RGBA8, 32, 16);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_TRUE(t->immutableStorage);
    EXPECT_EQ(t->storageLevels, 3);
    EXPECT_EQ(t->storageBaseWidth, 32);
    EXPECT_EQ(t->storageBaseHeight, 16);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->storage2DCalls, 1);
    EXPECT_EQ(mt->lastStorageW, 32);
    EXPECT_EQ(mt->lastStorageH, 16);
}

TEST_CASE("texStorage3D_allocates_immutable_3d") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_3D, tex);
    ctx.texStorage3D(GL_TEXTURE_3D, 2, GL_RGBA8, 8, 4, 2);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_TRUE(t->immutableStorage);
    EXPECT_EQ(t->storageBaseWidth, 8);
    EXPECT_EQ(t->storageBaseHeight, 4);
    EXPECT_EQ(t->storageBaseDepth, 2);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->storage3DCalls, 1);
    EXPECT_EQ(mt->lastStorageD, 2);
}

TEST_CASE("texStorage2D_without_bound_texture_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 4, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("texStorage2D_with_zero_levels_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("texStorage2D_with_zero_dimension_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// --- Non-DSA texture buffers (SPEC §8.9) ---

TEST_CASE("texBuffer_binds_buffer_to_texture") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_BUFFER, tex);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);

    ctx.texBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, buf);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->textureBufferCalls, 1);
    EXPECT_EQ(mt->lastStorageTarget, static_cast<uint32_t>(GL_TEXTURE_BUFFER));
    EXPECT_EQ(mt->lastBufferInternalFormat, static_cast<uint32_t>(GL_RGBA8));
}

TEST_CASE("texBufferRange_records_offset_and_size") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_BUFFER, tex);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 64, GL_STATIC_DRAW, nullptr);

    ctx.texBufferRange(GL_TEXTURE_BUFFER, GL_RGBA8, buf, 8, 32);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->textureBufferRangeCalls, 1);
    EXPECT_EQ(mt->lastBufferOffset, 8);
    EXPECT_EQ(mt->lastBufferSize, 32);
}

TEST_CASE("texBuffer_without_bound_texture_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);
    ctx.texBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, buf);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("texBuffer_with_ungenerated_buffer_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_BUFFER, tex);
    ctx.texBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, 999);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("texBuffer_with_wrong_target_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);
    ctx.texBuffer(GL_TEXTURE_2D, GL_RGBA8, buf);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("texBufferRange_with_negative_offset_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_BUFFER, tex);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 64, GL_STATIC_DRAW, nullptr);
    ctx.texBufferRange(GL_TEXTURE_BUFFER, GL_RGBA8, buf, -1, 32);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// --- Multisample texture storage (SPEC §8.19) ---

TEST_CASE("texStorage2DMultisample_allocates_immutable_msaa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
    ctx.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, 8, 8, true);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_TRUE(t->immutableStorage);
    EXPECT_EQ(t->storageBaseWidth, 8);
    EXPECT_EQ(t->storageBaseHeight, 8);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->storage2DMultisampleCalls, 1);
    EXPECT_EQ(mt->lastMTarget, static_cast<uint32_t>(GL_TEXTURE_2D_MULTISAMPLE));
    EXPECT_EQ(mt->lastMSamples, 4);
    EXPECT_EQ(mt->lastMWidth, 8);
    EXPECT_EQ(mt->lastMHeight, 8);
    EXPECT_TRUE(mt->lastMFixed);
}

TEST_CASE("texStorage2DMultisample_wrong_target_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texStorage2DMultisample(GL_TEXTURE_2D, 4, GL_RGBA8, 8, 8, true);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("texStorage2DMultisample_negative_samples_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
    ctx.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, -1, GL_RGBA8, 8, 8, true);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("texStorage3DMultisample_allocates_immutable_array_msaa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex);
    ctx.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 2, GL_RGBA8, 8, 4, 3,
                                false);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_TRUE(t->immutableStorage);
    EXPECT_EQ(t->storageBaseDepth, 3);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->storage3DMultisampleCalls, 1);
    EXPECT_EQ(mt->lastMTarget,
              static_cast<uint32_t>(GL_TEXTURE_2D_MULTISAMPLE_ARRAY));
    EXPECT_EQ(mt->lastMSamples, 2);
    EXPECT_EQ(mt->lastMDepth, 3);
    EXPECT_FALSE(mt->lastMFixed);
}

TEST_CASE("texImage2DMultisample_allocates_mutable_msaa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
    ctx.texImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, 8, 8, true);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_FALSE(t->immutableStorage);
    EXPECT_TRUE(t->storageSet);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texImage2DMultisampleCalls, 1);
    EXPECT_EQ(mt->lastMSamples, 4);
}

TEST_CASE("dsa_textureStorage2DMultisample_allocates_immutable_msaa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.textureStorage2DMultisample(tex, 4, GL_RGBA8, 8, 8, true);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_TRUE(t->immutableStorage);
    EXPECT_EQ(t->target, static_cast<uint32_t>(GL_TEXTURE_2D_MULTISAMPLE));

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->storage2DMultisampleCalls, 1);
    EXPECT_EQ(mt->lastMTarget, static_cast<uint32_t>(GL_TEXTURE_2D_MULTISAMPLE));
}

TEST_CASE("dsa_textureStorage2DMultisample_without_dsa_is_invalid_operation") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess,
                           FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.textureStorage2DMultisample(tex, 4, GL_RGBA8, 8, 8, true);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}
