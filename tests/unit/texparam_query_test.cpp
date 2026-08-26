#include "test_framework.hpp"

#include <cmath>

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

#define EXPECT_FLOAT_EQ(a, b) EXPECT_TRUE(std::abs((a) - (b)) < 1e-5f)

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

TEST_CASE("tex_parameterf_stores_float_scalar_and_pushes") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 1.5f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_FLOAT_EQ(t->paramsf[GL_TEXTURE_MIN_LOD], 1.5f);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texParameterfCalls, 1);
    EXPECT_FLOAT_EQ(mt->lastParamf, 1.5f);
}

TEST_CASE("tex_parameterfv_stores_vector_and_pushes") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    float bc[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    ctx.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->paramsfv[GL_TEXTURE_BORDER_COLOR].size(), 4u);
    EXPECT_FLOAT_EQ(t->paramsfv[GL_TEXTURE_BORDER_COLOR][2], 0.3f);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texParameterfvCalls, 1);
    EXPECT_EQ(mt->lastParamfv.size(), 4u);
    EXPECT_FLOAT_EQ(mt->lastParamfv[0], 0.1f);
}

TEST_CASE("tex_parameteriv_stores_int_vector") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    int sw[2] = {GL_REPEAT, GL_MIRRORED_REPEAT};
    ctx.texParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, sw, 2);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->paramsiv[GL_TEXTURE_SWIZZLE_RGBA].size(), 2u);
    EXPECT_EQ(t->paramsiv[GL_TEXTURE_SWIZZLE_RGBA][1],
              static_cast<int>(GL_MIRRORED_REPEAT));

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texParameterivCalls, 1);
}

TEST_CASE("tex_parameterfv_null_params_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    float bc[4] = {0, 0, 0, 0};
    ctx.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, nullptr, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("get_tex_parameterfv_reads_float_scalar") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 2.5f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    float v = -1.0f;
    ctx.getTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_FLOAT_EQ(v, 2.5f);
}

TEST_CASE("get_tex_parameterfv_unset_returns_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    float v = 99.0f;
    ctx.getTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_FLOAT_EQ(v, 0.0f);
}
