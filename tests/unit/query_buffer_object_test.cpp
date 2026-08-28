#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

namespace {

// Build a buffer object of `size` bytes bound to ARRAY_BUFFER and return its
// name. The frontend keeps an authoritative CPU mirror of the store.
GLuint makeBuffer(Context& ctx, GLsizeiptr size) {
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_COPY);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    return buf;
}

// Inject a known query result so getQueryObject* can be read deterministically.
MockQuery* makeQuery(Context& ctx, GLuint q, int64_t value) {
    glBeginQuery(GL_SAMPLES_PASSED, q);
    glEndQuery(GL_SAMPLES_PASSED);
    MockQuery* mq = static_cast<MockQuery*>(ctx.getQuery(q)->backend.get());
    mq->resultValue = value;
    mq->hasResult = true;
    return mq;
}

} // namespace

TEST_CASE("query_buffer_object_writes_result_into_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    GLuint buf = makeBuffer(ctx, 16);
    makeQuery(ctx, q, 42);

    // 32-bit signed QUERY_RESULT at offset 0.
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    int32_t read = 0;
    ctx.getNamedBufferSubData(buf, 0, 4, &read);
    EXPECT_EQ(read, 42);

    // QUERY_RESULT_AVAILABLE at offset 4 reports GL_TRUE.
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT_AVAILABLE, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ctx.getNamedBufferSubData(buf, 4, 4, &read);
    EXPECT_EQ(read, static_cast<int32_t>(GL_TRUE));

    setCurrentContext(nullptr);
}

TEST_CASE("query_buffer_object_64bit_variant_writes_8_bytes") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    GLuint buf = makeBuffer(ctx, 32);
    makeQuery(ctx, q, 100000000000LL);

    glGetQueryBufferObjectui64v(q, buf, GL_QUERY_RESULT, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    uint64_t read = 0;
    ctx.getNamedBufferSubData(buf, 0, 8, &read);
    EXPECT_EQ(read, 100000000000ULL);

    // Misaligned 64-bit offset is rejected.
    glGetQueryBufferObjectui64v(q, buf, GL_QUERY_RESULT, 4);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("query_buffer_object_validates_inputs") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    GLuint buf = makeBuffer(ctx, 16);
    makeQuery(ctx, q, 7);

    // Ungenerated query id -> INVALID_OPERATION.
    glGetQueryBufferObjectiv(9999, buf, GL_QUERY_RESULT, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Ungenerated buffer -> INVALID_OPERATION.
    glGetQueryBufferObjectiv(q, 9999, GL_QUERY_RESULT, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Unknown pname -> INVALID_ENUM.
    glGetQueryBufferObjectiv(q, buf, 0xDEAD, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Misaligned 32-bit offset -> INVALID_VALUE.
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT, 1);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Write extends past the buffer -> INVALID_VALUE.
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT, 13);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Negative offset -> INVALID_VALUE.
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT, -4);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
