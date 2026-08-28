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

// glGetInternalformativ (SPEC §22.3): forwards to the backend. The mock returns a
// documented conservative default (no sample counts, supported=true for a curated
// set of common core formats, 0 otherwise).
TEST_CASE("internalformat_iv_num_sample_counts_is_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t n = -1;
    ctx.getInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &n);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(n, 0);
}

TEST_CASE("internalformat_iv_supported_true_for_known_format") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t supported = -1;
    ctx.getInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_INTERNALFORMAT_SUPPORTED,
                           1, &supported);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(supported, static_cast<int32_t>(GL_TRUE));
}

TEST_CASE("internalformat_iv_supported_false_for_unknown_format") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t supported = -1;
    ctx.getInternalformativ(GL_TEXTURE_2D, 0xDEAD, GL_INTERNALFORMAT_SUPPORTED,
                           1, &supported);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(supported, static_cast<int32_t>(GL_FALSE));
}

TEST_CASE("internalformat_iv_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    // Unknown pname -> INVALID_ENUM.
    int32_t v = 0;
    ctx.getInternalformativ(GL_TEXTURE_2D, GL_RGBA8, 0xDEAD, 1, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    // Null params -> INVALID_VALUE.
    ctx.getInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Negative bufSize -> INVALID_VALUE.
    ctx.getInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, -1, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("internalformat_i64v_num_sample_counts_is_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int64_t n = -1;
    ctx.getInternalformati64v(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &n);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(n, 0);
}

TEST_CASE("internalformat_i64v_supported_true_for_known_format") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int64_t supported = -1;
    ctx.getInternalformati64v(GL_TEXTURE_2D, GL_RGBA16F, GL_INTERNALFORMAT_SUPPORTED,
                             1, &supported);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(supported, static_cast<int64_t>(GL_TRUE));
}

TEST_CASE("internalformat_gl_api_entry_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    int32_t n32 = -1;
    glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &n32);
    EXPECT_EQ(n32, 0);

    int64_t n64 = -1;
    glGetInternalformati64v(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &n64);
    EXPECT_EQ(n64, 0);

    setCurrentContext(nullptr);
}
