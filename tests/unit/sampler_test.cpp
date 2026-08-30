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

TEST_CASE("sampler_gen_creates_tracked_object") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    EXPECT_TRUE(s != 0);
    EXPECT_TRUE(ctx.getSampler(s) != nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
}

TEST_CASE("sampler_bind_pushes_only_when_changed") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();

    ctx.bindSampler(2, s);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 1);
    EXPECT_EQ(backend->lastBindSamplerUnit, 2u);
    EXPECT_EQ(backend->lastBindSampler, s);

    // Re-binding the same sampler to the same unit must not push again.
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 1);

    // Binding zero (unbind) pushes the change.
    ctx.bindSampler(2, 0);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 2);
    EXPECT_EQ(backend->lastBindSampler, 0u);
}

TEST_CASE("sampler_binding_query_reflects_active_unit") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName s = ctx.genSampler();

    ctx.bindSampler(3, s);
    GLint bound = 0;
    ctx.getIntegerv(GL_SAMPLER_BINDING, &bound);
    // SAMPLER_BINDING reports the sampler on the *active* texture unit.
    EXPECT_EQ(bound, 0); // active unit 0, not 3
    ctx.activeTexture(GL_TEXTURE0 + 3);
    ctx.getIntegerv(GL_SAMPLER_BINDING, &bound);
    EXPECT_EQ(bound, static_cast<GLint>(s));

    // glGetIntegerv surface.
    GLint viaApi = -1;
    glGetIntegerv(GL_SAMPLER_BINDING, &viaApi);
    EXPECT_EQ(viaApi, static_cast<GLint>(s));
    setCurrentContext(nullptr);
}

TEST_CASE("sampler_bind_out_of_range_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.bindSampler(ctx.state().maxCombinedTextureUnits(), s);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("sampler_bind_ungenerated_name_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindSampler(0, 9999);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("sampler_parameter_valid_and_invalid_pname") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();

    ctx.samplerParameteri(s, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    // Non-scalar / unknown pname -> INVALID_ENUM.
    ctx.samplerParameteri(s, GL_TEXTURE_BORDER_COLOR, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    int val = -1;
    ctx.getSamplerParameteriv(s, GL_TEXTURE_MIN_FILTER, &val);
    EXPECT_EQ(val, static_cast<int>(GL_NEAREST_MIPMAP_LINEAR));

    // Query of invalid pname also reports INVALID_ENUM.
    ctx.getSamplerParameteriv(s, 0xDEAD, &val);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("sampler_parameter_pushes_to_backend_resource") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.samplerParameteri(s, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    auto* ms = dynamic_cast<MockSampler*>(ctx.getSampler(s)->backend.get());
    EXPECT_TRUE(ms != nullptr);
    EXPECT_EQ(ms->samplerParameteriCalls, 1);
    EXPECT_EQ(ms->lastParamPname, static_cast<uint32_t>(GL_TEXTURE_WRAP_S));
    EXPECT_EQ(ms->lastParam, static_cast<int>(GL_CLAMP_TO_EDGE));
}

TEST_CASE("sampler_delete_reverts_binding") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.bindSampler(1, s);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 1);

    ctx.deleteSampler(s);
    // Deleting a bound sampler resets the unit binding to 0, pushed on flush.
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 2);
    EXPECT_EQ(backend->lastBindSampler, 0u);
    EXPECT_TRUE(ctx.getSampler(s) == nullptr);
    EXPECT_EQ(ctx.boundSampler(1), 0u);
}

TEST_CASE("sampler_is_sampler_distinguishes_objects") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    EXPECT_TRUE(ctx.isSampler(s));
    EXPECT_FALSE(ctx.isSampler(12345));
}

TEST_CASE("sampler_parameterf_scalar_float_query") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();

    ctx.samplerParameterf(s, GL_TEXTURE_MIN_LOD, -2.0f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* ms = dynamic_cast<MockSampler*>(ctx.getSampler(s)->backend.get());
    EXPECT_EQ(ms->samplerParameterfCalls, 1);
    EXPECT_EQ(ms->lastParamf, -2.0f);

    float val = 0.0f;
    ctx.getSamplerParameterfv(s, GL_TEXTURE_MIN_LOD, &val);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(val, -2.0f);
}

TEST_CASE("sampler_parameterf_invalid_pname_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.samplerParameterf(s, GL_TEXTURE_WRAP_S, 1.0f); // int pname -> float setter
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    float val = -1.0f;
    ctx.getSamplerParameterfv(s, GL_TEXTURE_WRAP_S, &val);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("sampler_parameterfv_border_color_query") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();

    const float bc[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    ctx.samplerParameterfv(s, GL_TEXTURE_BORDER_COLOR, bc, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* ms = dynamic_cast<MockSampler*>(ctx.getSampler(s)->backend.get());
    EXPECT_EQ(ms->samplerParameterfvCalls, 1);
    EXPECT_EQ(ms->lastParamfv.size(), 4u);
    EXPECT_EQ(ms->lastParamfv[3], 0.4f);

    float val = 0.0f;
    ctx.getSamplerParameterfv(s, GL_TEXTURE_BORDER_COLOR, &val);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(val, 0.1f);
}

TEST_CASE("sampler_parameterfv_invalid_pname_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    const float bc[4] = {0, 0, 0, 0};
    ctx.samplerParameterfv(s, GL_TEXTURE_MIN_LOD, bc, 4); // float-scalar pname
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("sampler_parameterfv_null_params_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.samplerParameterfv(s, GL_TEXTURE_BORDER_COLOR, nullptr, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("sampler_parameterIiv_Iuiv_signed_unsigned_query") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();

    const int32_t iv[1] = {static_cast<int32_t>(GL_CLAMP_TO_EDGE)};
    ctx.samplerParameterIiv(s, GL_TEXTURE_WRAP_S, iv);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* ms = dynamic_cast<MockSampler*>(ctx.getSampler(s)->backend.get());
    EXPECT_EQ(ms->samplerParameterIivCalls, 1);

    const uint32_t uv[1] = {static_cast<uint32_t>(GL_CLAMP_TO_EDGE)};
    ctx.samplerParameterIuiv(s, GL_TEXTURE_WRAP_T, uv);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ms->samplerParameterIuivCalls, 1);

    int32_t ri = -1;
    ctx.getSamplerParameterIiv(s, GL_TEXTURE_WRAP_S, &ri);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ri, static_cast<int32_t>(GL_CLAMP_TO_EDGE));

    uint32_t ru = 0;
    ctx.getSamplerParameterIuiv(s, GL_TEXTURE_WRAP_T, &ru);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ru, static_cast<uint32_t>(GL_CLAMP_TO_EDGE));
}

TEST_CASE("sampler_parameterIiv_invalid_pname_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    const int32_t iv[1] = {0};
    ctx.samplerParameterIiv(s, GL_TEXTURE_MIN_LOD, iv); // float pname -> Iiv
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("sampler_parameter_null_sampler_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.samplerParameterf(9999, GL_TEXTURE_MIN_LOD, 0.0f);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("sampler_parameter_public_gl_api_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName s = ctx.genSampler();

    glSamplerParameterf(s, GL_TEXTURE_MAX_LOD, 8.0f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    const float bc[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glSamplerParameterfv(s, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    float maxLod = -1.0f;
    glGetSamplerParameterfv(s, GL_TEXTURE_MAX_LOD, &maxLod);
    EXPECT_EQ(maxLod, 8.0f);

    float b0 = -1.0f;
    glGetSamplerParameterfv(s, GL_TEXTURE_BORDER_COLOR, &b0);
    EXPECT_EQ(b0, 1.0f);
    setCurrentContext(nullptr);
}

TEST_CASE("sampler_parameteriv_border_color_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();

    const int32_t bc[4] = {11, 22, 33, 44};
    ctx.samplerParameteriv(s, GL_TEXTURE_BORDER_COLOR, bc, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* ms = dynamic_cast<MockSampler*>(ctx.getSampler(s)->backend.get());
    EXPECT_EQ(ms->samplerParameterivCalls, 1);
    EXPECT_EQ(ms->lastParamPname, static_cast<uint32_t>(GL_TEXTURE_BORDER_COLOR));
    EXPECT_EQ(ms->lastParamiv.size(), 4u);
    EXPECT_EQ(ms->lastParamiv[3], 44);

    int32_t val = -1;
    ctx.getSamplerParameteriv(s, GL_TEXTURE_BORDER_COLOR, &val);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(val, 11);
}

TEST_CASE("sampler_parameteriv_invalid_pname_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    const int32_t bc[4] = {0, 0, 0, 0};
    ctx.samplerParameteriv(s, GL_TEXTURE_MIN_LOD, bc, 4); // float-scalar pname
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("sampler_parameteriv_null_params_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.samplerParameteriv(s, GL_TEXTURE_BORDER_COLOR, nullptr, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("sampler_parameteriv_public_gl_api_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName s = ctx.genSampler();

    const int32_t bc[4] = {1, 2, 3, 4};
    glSamplerParameteriv(s, GL_TEXTURE_BORDER_COLOR, bc);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    int32_t v = -1;
    glGetSamplerParameteriv(s, GL_TEXTURE_BORDER_COLOR, &v);
    EXPECT_EQ(v, 1);
    setCurrentContext(nullptr);
}
