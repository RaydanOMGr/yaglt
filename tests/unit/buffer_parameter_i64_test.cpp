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

// glGetBufferParameteri64v (SPEC §6.1.1): reads frontend-owned buffer state as
// 64-bit. GL_BUFFER_SIZE is genuinely 64-bit; the rest widen the 32-bit form.
TEST_CASE("buffer_param_i64v_reads_size_and_usage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 1024, GL_DYNAMIC_COPY, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int64_t size = -1;
    ctx.getBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    EXPECT_EQ(size, 1024);

    int64_t usage = -1;
    ctx.getBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);
    EXPECT_EQ(usage, static_cast<int64_t>(GL_DYNAMIC_COPY));

    int64_t mapped = -1;
    ctx.getBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
    EXPECT_EQ(mapped, GL_FALSE);
}

TEST_CASE("buffer_param_i64v_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);

    // Unbound target -> INVALID_OPERATION.
    int64_t v = 0;
    ctx.getBufferParameteri64v(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Null params -> INVALID_VALUE.
    ctx.getBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Unknown pname -> INVALID_ENUM.
    ctx.getBufferParameteri64v(GL_ARRAY_BUFFER, 0xDEAD, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

// glGetNamedBufferParameteri64v (SPEC §6.1.1, DSA): operates on a named buffer
// via DirectStateAccess, no binding required.
TEST_CASE("buffer_param_named_i64v_via_dsa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 2048, GL_STREAM_DRAW, nullptr);

    int64_t size = -1;
    ctx.getNamedBufferParameteri64v(buf, GL_BUFFER_SIZE, &size);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(size, 2048);

    // Ungenerated name -> INVALID_OPERATION.
    int64_t v = 0;
    ctx.getNamedBufferParameteri64v(999, GL_BUFFER_SIZE, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// When DirectStateAccess is unsupported, the named variant is rejected.
TEST_CASE("buffer_param_named_i64v_gated_by_dsa_capability") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 64, GL_STATIC_DRAW, nullptr);

    int64_t v = 0;
    ctx.getNamedBufferParameteri64v(buf, GL_BUFFER_SIZE, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// Public dispatch surface reaches the frontend.
TEST_CASE("buffer_param_i64v_gl_api_entry_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, 512, nullptr, GL_STATIC_DRAW);

    GLint64 size = 0;
    glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    EXPECT_EQ(size, 512);

    GLint64 named = 0;
    glGetNamedBufferParameteri64v(buf, GL_BUFFER_SIZE, &named);
    EXPECT_EQ(named, 512);

    setCurrentContext(nullptr);
}

// glGetNamedBufferParameteriv (SPEC §6.1.1, DSA): 32-bit counterpart of the
// named i64v query. Reads the same frontend-owned buffer state.
TEST_CASE("buffer_param_named_iv_via_dsa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 2048, GL_STREAM_DRAW, nullptr);

    int32_t size = -1;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_SIZE, &size);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(size, 2048);

    int32_t usage = -1;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_USAGE, &usage);
    EXPECT_EQ(usage, static_cast<int32_t>(GL_STREAM_DRAW));

    int32_t mapped = -1;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_MAPPED, &mapped);
    EXPECT_EQ(mapped, GL_FALSE);

    int32_t imm = -1;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_IMMUTABLE_STORAGE, &imm);
    EXPECT_EQ(imm, GL_FALSE);
}

// glGetNamedBufferParameteriv validation mirrors the i64v variant.
TEST_CASE("buffer_param_named_iv_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);

    // Ungenerated name -> INVALID_OPERATION.
    int32_t v = 0;
    ctx.getNamedBufferParameteriv(999, GL_BUFFER_SIZE, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Null params -> INVALID_VALUE.
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_SIZE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Unknown pname -> INVALID_ENUM.
    ctx.getNamedBufferParameteriv(buf, 0xDEAD, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("buffer_param_named_iv_gated_by_dsa_capability") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 64, GL_STATIC_DRAW, nullptr);

    int32_t v = 0;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_SIZE, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("buffer_param_named_iv_gl_api_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, 512, nullptr, GL_STATIC_DRAW);

    GLint size = 0;
    glGetNamedBufferParameteriv(buf, GL_BUFFER_SIZE, &size);
    EXPECT_EQ(size, 512);

    setCurrentContext(nullptr);
}
