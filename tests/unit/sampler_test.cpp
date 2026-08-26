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
    ctx.getSamplerParameteriv(s, GL_TEXTURE_BORDER_COLOR, &val);
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
