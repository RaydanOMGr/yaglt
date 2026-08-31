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

// glGetnBufferParameteriv (SPEC §6.1.2 / ARB_robustness): bounds-checked 32-bit
// buffer parameter read. Writes the single int element that the non-robust
// query would, and rejects a bufSize too small to hold it.
TEST_CASE("getn_buffer_parameter_iv_reads_size_and_usage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 1024, GL_DYNAMIC_COPY, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t size = -1;
    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, &size);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(size, 1024);

    int32_t usage = -1;
    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, 1, &usage);
    EXPECT_EQ(usage, static_cast<int32_t>(GL_DYNAMIC_COPY));

    int32_t mapped = -1;
    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, 1, &mapped);
    EXPECT_EQ(mapped, GL_FALSE);
}

// glGetnBufferParameteri64v (SPEC §6.1.2 / ARB_robustness): 64-bit counterpart.
TEST_CASE("getn_buffer_parameter_i64v_reads_size") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 2048, GL_STREAM_DRAW, nullptr);

    int64_t size = -1;
    ctx.getnBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, &size);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(size, 2048);

    int64_t usage = -1;
    ctx.getnBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, 1, &usage);
    EXPECT_EQ(usage, static_cast<int64_t>(GL_STREAM_DRAW));
}

// A bufSize smaller than the one element written is an INVALID_OPERATION, and a
// negative bufSize is an INVALID_VALUE (ARB_robustness).
TEST_CASE("getn_buffer_parameter_bufsize_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);

    int32_t v = 0;
    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 0, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, -1, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    int64_t w = 0;
    ctx.getnBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 0, &w);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// The robust variants inherit the same frontend validation as the non-robust
// queries: unbound target -> INVALID_OPERATION, null params -> INVALID_VALUE,
// unknown pname -> INVALID_ENUM.
TEST_CASE("getn_buffer_parameter_inherits_base_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);

    int32_t v = 0;
    ctx.getnBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    ctx.getnBufferParameteriv(GL_ARRAY_BUFFER, 0xDEAD, 1, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    int64_t w = 0;
    ctx.getnBufferParameteri64v(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, &w);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    ctx.getnBufferParameteri64v(GL_ARRAY_BUFFER, 0xDEAD, 1, &w);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

// Public dispatch surface reaches the frontend.
TEST_CASE("getn_buffer_parameter_gl_api_entry_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, 512, nullptr, GL_STATIC_DRAW);

    GLint size = 0;
    glGetnBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, &size);
    EXPECT_EQ(size, 512);

    GLint64 size64 = 0;
    glGetnBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, 1, &size64);
    EXPECT_EQ(size64, 512);

    setCurrentContext(nullptr);
}
