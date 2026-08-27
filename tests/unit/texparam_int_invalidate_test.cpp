#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("generate_mipmap_forwards_to_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.generateMipmap(GL_TEXTURE_2D);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->generateMipmapCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, static_cast<uint32_t>(GL_TEXTURE_2D));
}

TEST_CASE("generate_mipmap_no_texture_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.generateMipmap(GL_TEXTURE_2D);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("gl_api_generate_mipmap_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGenerateMipmap(GL_TEXTURE_2D);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    setCurrentContext(nullptr);
}

TEST_CASE("tex_parameter_iiv_stores_and_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    int32_t bc[4] = {1, 2, 3, 4};
    ctx.texParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->paramsIiv[GL_TEXTURE_BORDER_COLOR].size(), 4u);
    EXPECT_EQ(t->paramsIiv[GL_TEXTURE_BORDER_COLOR][2], 3);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texParameterIivCalls, 1);
    EXPECT_EQ(mt->lastParamIiv.size(), 4u);
    EXPECT_EQ(mt->lastParamIiv[0], 1);
}

TEST_CASE("tex_parameter_iuiv_stores_and_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    uint32_t bc[4] = {5u, 6u, 7u, 8u};
    ctx.texParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->paramsIuiv[GL_TEXTURE_BORDER_COLOR].size(), 4u);
    EXPECT_EQ(t->paramsIuiv[GL_TEXTURE_BORDER_COLOR][1], 6u);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texParameterIuivCalls, 1);
    EXPECT_EQ(mt->lastParamIuiv.size(), 4u);
    EXPECT_EQ(mt->lastParamIuiv[3], 8u);
}

TEST_CASE("tex_parameter_iiv_null_params_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("tex_parameter_iiv_no_texture_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t v = 0;
    ctx.texParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("get_tex_parameter_iiv_returns_stored") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    int32_t bc[4] = {9, 8, 7, 6};
    ctx.texParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t out[4] = {0, 0, 0, 0};
    ctx.getTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, out);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(out[0], 9);
    EXPECT_EQ(out[3], 6);
}

TEST_CASE("get_tex_parameter_iiv_unset_returns_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    int32_t v = -1;
    ctx.getTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(v, 0);
}

TEST_CASE("get_tex_parameter_iiv_null_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.getTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("get_tex_parameter_iuiv_returns_stored") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    uint32_t bc[4] = {1u, 0u, 0u, 0u};
    ctx.texParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    uint32_t out[4] = {0, 0, 0, 0};
    ctx.getTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, out);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(out[0], 1u);
}

TEST_CASE("dsa_texture_parameter_iiv_get") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    int32_t bc[4] = {2, 3, 4, 5};
    ctx.textureParameterIiv(tex, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t out[4] = {0, 0, 0, 0};
    ctx.getTextureParameterIiv(tex, GL_TEXTURE_BORDER_COLOR, out);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(out[1], 3);
}

TEST_CASE("dsa_get_texture_parameter_iiv_unsupported_invalid_operation") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    int32_t v = 0;
    ctx.getTextureParameterIiv(tex, GL_TEXTURE_BORDER_COLOR, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("invalidate_tex_image_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.invalidateTexImage(GL_TEXTURE_2D, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->invalidateTexImageCalls, 1);
    EXPECT_EQ(mt->lastInvLevel, 0);
}

TEST_CASE("invalidate_tex_image_negative_level_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.invalidateTexImage(GL_TEXTURE_2D, -1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("invalidate_tex_image_no_texture_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.invalidateTexImage(GL_TEXTURE_2D, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("invalidate_tex_sub_image_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.invalidateTexSubImage(GL_TEXTURE_2D, 1, 2, 3, 4, 16, 32, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->invalidateTexSubImageCalls, 1);
    EXPECT_EQ(mt->lastInvLevel, 1);
    EXPECT_EQ(mt->lastInvX, 2);
    EXPECT_EQ(mt->lastInvW, 16);
    EXPECT_EQ(mt->lastInvH, 32);
    EXPECT_EQ(mt->lastInvD, 1);
}

TEST_CASE("gl_api_int_invalidate_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    int32_t bc[4] = {1, 1, 1, 1};
    glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    int32_t out[4] = {0, 0, 0, 0};
    glGetTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(out[0], 1);

    glInvalidateTexImage(GL_TEXTURE_2D, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    setCurrentContext(nullptr);
}
