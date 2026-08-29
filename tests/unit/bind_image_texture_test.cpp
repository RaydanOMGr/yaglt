#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("bind_image_texture_records_and_pushes") {
    MockBackend backend;
    backend.setCapability(Feature::ImageLoadStore, FeatureSupport::Native);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    EXPECT_NE(tex, 0u);

    // Default (no bindings) -> no push.
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 0);

    // Bind texture to unit 2.
    glBindImageTexture(2, tex, 1, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 1);
    EXPECT_EQ(backend.lastBindImageTextureUnit, 2u);
    EXPECT_EQ(backend.lastBindImageTexture, tex);
    EXPECT_EQ(backend.lastBindImageTextureLevel, 1);
    EXPECT_EQ(backend.lastBindImageTextureLayered, false);
    EXPECT_EQ(backend.lastBindImageTextureLayer, 0);
    EXPECT_EQ(backend.lastBindImageTextureAccess,
              static_cast<uint32_t>(GL_READ_WRITE));
    EXPECT_EQ(backend.lastBindImageTextureFormat,
              static_cast<uint32_t>(GL_RGBA32F));

    // Identical re-flush: no further push (SPEC §10).
    glBindImageTexture(2, tex, 1, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 1);

    // Binding unit 0 with a different texture pushes once more.
    GLuint tex2 = 0;
    glGenTextures(1, &tex2);
    glBindImageTexture(0, tex2, 3, GL_TRUE, 2, GL_READ_ONLY, GL_RGBA32F);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 2);
    EXPECT_EQ(backend.lastBindImageTextureUnit, 0u);
    EXPECT_EQ(backend.lastBindImageTextureLayered, true);
    EXPECT_EQ(backend.lastBindImageTextureLayer, 2);

    // Binding 0 unbinds the unit (other params ignored), pushing again.
    glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 3);
    EXPECT_EQ(backend.lastBindImageTextureUnit, 2u);
    EXPECT_EQ(backend.lastBindImageTexture, 0u);

    setCurrentContext(nullptr);
}

TEST_CASE("bind_image_texture_validation") {
    MockBackend backend;
    backend.setCapability(Feature::ImageLoadStore, FeatureSupport::Native);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tex = 0;
    glGenTextures(1, &tex);

    // Capability off -> GL_INVALID_OPERATION.
    backend.setCapability(Feature::ImageLoadStore, FeatureSupport::Unsupported);
    glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    backend.setCapability(Feature::ImageLoadStore, FeatureSupport::Native);

    // Out-of-range unit -> GL_INVALID_VALUE, no push.
    glBindImageTexture(8, tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 0);

    // Non-zero texture with negative level -> GL_INVALID_VALUE.
    glBindImageTexture(0, tex, -1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Negative layer -> GL_INVALID_VALUE.
    glBindImageTexture(0, tex, 0, GL_FALSE, -2, GL_READ_ONLY, GL_RGBA32F);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Invalid access -> GL_INVALID_ENUM.
    glBindImageTexture(0, tex, 0, GL_FALSE, 0, 0xDEAD, GL_RGBA32F);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    // Ungenerated texture name -> GL_INVALID_OPERATION.
    glBindImageTexture(0, 9999u, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Still no pushes from the rejected calls.
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 0);

    // Binding 0 ignores the other params (no error, no push since unit 0 is
    // already at the default unbound state).
    glBindImageTexture(0, 0, -5, GL_FALSE, -5, 0xDEAD, 0xDEAD);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("bind_image_textures_multi") {
    MockBackend backend;
    backend.setCapability(Feature::ImageLoadStore, FeatureSupport::Native);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint t[3] = {0, 0, 0};
    glGenTextures(3, t);

    // Multi-bind units 0..2 with defaults (level 0, READ_ONLY, RGBA32F).
    glBindImageTextures(0, 3, t);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 3);
    EXPECT_EQ(backend.lastBindImageTextureUnit, 2u);
    EXPECT_EQ(backend.lastBindImageTexture, t[2]);
    EXPECT_EQ(backend.lastBindImageTextureLevel, 0);
    EXPECT_EQ(backend.lastBindImageTextureAccess,
              static_cast<uint32_t>(GL_READ_ONLY));
    EXPECT_EQ(backend.lastBindImageTextureFormat,
              static_cast<uint32_t>(GL_RGBA32F));

    // count == 0 is a silent no-op (no error, no push).
    glBindImageTextures(0, 0, t);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 3);

    // first + count out of range -> GL_INVALID_VALUE, no push.
    glBindImageTextures(6, 4, t);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 3);

    // A null array unbinds every touched unit (still within range -> valid).
    glBindImageTextures(0, 2, nullptr);
    ctx.flushState();
    EXPECT_EQ(backend.bindImageTextureCalls, 5);
    EXPECT_EQ(backend.lastBindImageTextureUnit, 1u);
    EXPECT_EQ(backend.lastBindImageTexture, 0u);

    setCurrentContext(nullptr);
}
