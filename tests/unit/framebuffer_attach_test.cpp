#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// --- Classic (bound) framebuffer attachment entry points (SPEC §9.2) ---
// glFramebufferTexture / glFramebufferTextureLayer mirror the DSA surface but
// operate on the currently bound framebuffer instead of a named one.

TEST_CASE("framebuffer_texture_records_on_bound_fbo") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 64, 64);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTexture2DCalls, 1);
    EXPECT_EQ(mf->lastAttachment, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(mf->lastLevel, 0);
}

TEST_CASE("framebuffer_texture_layer_records_on_bound_fbo") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_2D_ARRAY, 1, &tex);
    ctx.textureStorage3D(tex, 1, GL_RGBA8, 64, 64, 4);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0, 2);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTextureLayerCalls, 1);
    EXPECT_EQ(mf->lastLayer, 2);
    EXPECT_EQ(mf->lastLayerLevel, 0);
}

TEST_CASE("framebuffer_texture_no_fbo_bound_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex;
    ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 64, 64);
    // Default framebuffer (name 0) bound -> classic attach rejects.
    ctx.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("framebuffer_texture_unknown_texture_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 777, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("framebuffer_texture_layer_unknown_texture_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 777, 0, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("framebuffer_texture_detach_when_texture_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 64, 64);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    // Detach: texture 0 is valid and must forwards a null native attachment.
    ctx.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTexture2DCalls, 2);
}

TEST_CASE("framebuffer_texture_1d_records_on_bound_fbo") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_1D, 1, &tex);
    ctx.textureStorage1D(tex, 1, GL_RGBA8, 64);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_1D,
                             tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTexture2DCalls, 1);
    EXPECT_EQ(mf->lastAttachment, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(mf->lastTexTarget, GL_TEXTURE_1D);
}

TEST_CASE("framebuffer_texture_1d_rejects_bad_textarget") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_1D, 1, &tex);
    ctx.textureStorage1D(tex, 1, GL_RGBA8, 64);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("framebuffer_texture_3d_records_layer_on_bound_fbo") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_3D, 1, &tex);
    ctx.textureStorage3D(tex, 1, GL_RGBA8, 64, 64, 4);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D,
                             tex, 0, 2);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTextureLayerCalls, 1);
    EXPECT_EQ(mf->lastLayer, 2);
    EXPECT_EQ(mf->lastLayerLevel, 0);
}

TEST_CASE("framebuffer_texture_3d_rejects_bad_textarget") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_3D, 1, &tex);
    ctx.textureStorage3D(tex, 1, GL_RGBA8, 64, 64, 4);
    ctx.bindFramebuffer(fb);
    ctx.framebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             tex, 0, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("gl_api_framebuffer_texture_1d_3d_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName fb, tex1, tex3;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_1D, 1, &tex1);
    ctx.textureStorage1D(tex1, 1, GL_RGBA8, 64);
    ctx.createTextures(GL_TEXTURE_3D, 1, &tex3);
    ctx.textureStorage3D(tex3, 1, GL_RGBA8, 64, 64, 4);
    ctx.bindFramebuffer(fb);

    glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_1D,
                           tex1, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D,
                           tex3, 0, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTexture2DCalls, 1);
    EXPECT_EQ(mf->framebufferTextureLayerCalls, 1);
    setCurrentContext(nullptr);
}
