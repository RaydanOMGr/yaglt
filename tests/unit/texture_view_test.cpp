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

// Texture views (SPEC §8.19 glTextureView): a view shares the immutable storage
// of a source texture, exposing a level/layer subrange under a (compatible)
// internal format. The frontend records the view and forwards it to the backend.
TEST_CASE("texture_view_creates_view_and_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName src; ctx.createTextures(GL_TEXTURE_2D, 1, &src);
    ctx.textureStorage2D(src, 4, GL_RGBA8, 64, 64);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    GLObjectName view; ctx.createTextures(GL_TEXTURE_2D, 1, &view);
    ctx.textureView(view, GL_TEXTURE_2D, src, GL_RGBA8, 1, 2, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* mt = static_cast<MockTexture*>(ctx.getTexture(view)->backend.get());
    EXPECT_EQ(mt->viewCalls, 1);
    EXPECT_EQ(mt->lastViewTarget, GL_TEXTURE_2D);
    EXPECT_EQ(mt->lastViewInternalFormat, GL_RGBA8);
    EXPECT_EQ(mt->lastViewMinLevel, 1u);
    EXPECT_EQ(mt->lastViewNumLevels, 2u);
    EXPECT_EQ(mt->lastViewMinLayer, 0u);
    EXPECT_EQ(mt->lastViewNumLayers, 1u);
    // The mock forwarded the source texture's native id to the driver.
    EXPECT_EQ(mt->lastViewOrigNativeId,
              static_cast<MockTexture*>(ctx.getTexture(src)->backend.get())->nativeId());

    // The view object records its view relationship and derived storage.
    TextureObject* v = ctx.getTexture(view);
    EXPECT_TRUE(v->isView);
    EXPECT_EQ(v->viewSource, src);
    EXPECT_TRUE(v->immutableStorage);
    EXPECT_EQ(v->storageLevels, 2);
    EXPECT_EQ(v->storageBaseWidth, 32); // 64 >> 1
    EXPECT_EQ(v->storageBaseHeight, 32);
}

TEST_CASE("texture_view_requires_immutable_source") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName src; ctx.createTextures(GL_TEXTURE_2D, 1, &src);
    // No storage allocated on the source: viewing must be refused.
    GLObjectName view; ctx.createTextures(GL_TEXTURE_2D, 1, &view);
    ctx.textureView(view, GL_TEXTURE_2D, src, GL_RGBA8, 0, 1, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_FALSE(ctx.getTexture(view)->isView);
}

TEST_CASE("texture_view_rejects_self_and_bad_range") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName src; ctx.createTextures(GL_TEXTURE_2D, 1, &src);
    ctx.textureStorage2D(src, 4, GL_RGBA8, 64, 64);

    // A texture may not be a view of itself.
    ctx.textureView(src, GL_TEXTURE_2D, src, GL_RGBA8, 0, 1, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Level range must fit within the source: minLevel + numLevels > 4.
    GLObjectName view; ctx.createTextures(GL_TEXTURE_2D, 1, &view);
    ctx.textureView(view, GL_TEXTURE_2D, src, GL_RGBA8, 3, 2, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // numLevels == 0 is also invalid.
    ctx.textureView(view, GL_TEXTURE_2D, src, GL_RGBA8, 0, 0, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // An invalid target is rejected with GL_INVALID_ENUM.
    ctx.textureView(view, 0xDEAD, src, GL_RGBA8, 0, 1, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("texture_view_unknown_object_rejected") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName src; ctx.createTextures(GL_TEXTURE_2D, 1, &src);
    ctx.textureStorage2D(src, 2, GL_RGBA8, 16, 16);
    // Viewing into / from an ungenerated name is an error.
    ctx.textureView(9999, GL_TEXTURE_2D, src, GL_RGBA8, 0, 1, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.textureView(0, GL_TEXTURE_2D, src, GL_RGBA8, 0, 1, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}
