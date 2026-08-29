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

TEST_CASE("bind_samplers_binds_consecutive_units") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s[3] = {ctx.genSampler(), ctx.genSampler(), ctx.genSampler()};

    ctx.bindSamplers(1, 3, s);
    ctx.flushState();
    // One native bindSampler push per touched unit (SPEC §10: only changed).
    EXPECT_EQ(backend->bindSamplerCalls, 3);
    EXPECT_EQ(backend->lastBindSamplerUnit, 3u);
    EXPECT_EQ(backend->lastBindSampler, s[2]);
    EXPECT_EQ(ctx.boundSampler(1), s[0]);
    EXPECT_EQ(ctx.boundSampler(2), s[1]);
    EXPECT_EQ(ctx.boundSampler(3), s[2]);
}

TEST_CASE("bind_samplers_null_unbinds_range") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s[2] = {ctx.genSampler(), ctx.genSampler()};

    ctx.bindSamplers(0, 2, s);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 2);

    // A null array unbinds every touched unit (SPEC: equivalent to per-unit 0).
    ctx.bindSamplers(0, 2, nullptr);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 4);
    EXPECT_EQ(backend->lastBindSampler, 0u);
    EXPECT_EQ(ctx.boundSampler(0), 0u);
    EXPECT_EQ(ctx.boundSampler(1), 0u);
}

TEST_CASE("bind_samplers_rebind_skips_unchanged_units") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s[2] = {ctx.genSampler(), ctx.genSampler()};

    ctx.bindSamplers(0, 2, s);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 2);

    // Re-bind identical array: no new native pushes.
    ctx.bindSamplers(0, 2, s);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 2);
}

TEST_CASE("bind_samplers_negative_count_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    ctx.bindSamplers(0, -1, &s);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("bind_samplers_range_out_of_bounds_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName s = ctx.genSampler();
    // SPEC §8.2: first + count > MAX_COMBINED_TEXTURE_IMAGE_UNITS is
    // GL_INVALID_OPERATION (negative count is the GL_INVALID_VALUE case).
    ctx.bindSamplers(ctx.state().maxCombinedTextureUnits(), 1, &s);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("bind_samplers_zero_count_at_limit_is_valid") {
    auto backend = makeBackend();
    Context ctx(*backend);
    // first + count == the unit count is not "greater than", so no error.
    ctx.bindSamplers(ctx.state().maxCombinedTextureUnits(), 0, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
}

TEST_CASE("bind_samplers_ungenerated_name_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName bad = 9999;
    ctx.bindSamplers(0, 1, &bad);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(ctx.boundSampler(0), 0u);
}

TEST_CASE("bind_samplers_invalid_entry_still_binds_valid_entries") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName good0 = ctx.genSampler();
    GLObjectName good2 = ctx.genSampler();
    GLObjectName prev = ctx.genSampler();
    ctx.bindSampler(1, prev); // unit 1 keeps this binding
    ctx.flushState();
    const int before = backend->bindSamplerCalls;

    GLObjectName arr[3] = {good0, 9999, good2};
    ctx.bindSamplers(0, 3, arr);
    // SPEC §8.2: the invalid entry is reported per binding...
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    // ...but the valid entries are still bound and unit 1 is untouched.
    EXPECT_EQ(ctx.boundSampler(0), good0);
    EXPECT_EQ(ctx.boundSampler(1), prev);
    EXPECT_EQ(ctx.boundSampler(2), good2);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, before + 2);
}

TEST_CASE("bind_samplers_public_dispatch_path") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName s[2] = {ctx.genSampler(), ctx.genSampler()};

    glBindSamplers(2, 2, s);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    ctx.flushState();
    EXPECT_EQ(backend->bindSamplerCalls, 2);
    EXPECT_EQ(ctx.boundSampler(2), s[0]);
    EXPECT_EQ(ctx.boundSampler(3), s[1]);
    setCurrentContext(nullptr);
}
