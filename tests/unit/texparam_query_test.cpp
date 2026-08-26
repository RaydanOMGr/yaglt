#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("get_tex_parameter_iv_returns_stored_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int v = -1;
    ctx.getTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(v, static_cast<int>(GL_LINEAR));
}

TEST_CASE("get_tex_parameter_iv_no_texture_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int v = 0;
    ctx.getTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("get_tex_parameter_iv_null_params_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.getTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("get_texture_parameter_iv_dsa_explicit_texture") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int v = -1;
    ctx.getTextureParameteriv(tex, GL_TEXTURE_WRAP_S, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(v, static_cast<int>(GL_CLAMP_TO_EDGE));
}

TEST_CASE("get_texture_parameter_iv_dsa_ungenerated_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int v = 0;
    ctx.getTextureParameteriv(99999, GL_TEXTURE_WRAP_S, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("get_texture_parameter_iv_dsa_unsupported_invalid_operation") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    int v = 0;
    ctx.getTextureParameteriv(tex, GL_TEXTURE_WRAP_S, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("gl_api_tex_parameter_query_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLint v = -1;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &v);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(v, static_cast<GLint>(GL_NEAREST));
    setCurrentContext(nullptr);
}
