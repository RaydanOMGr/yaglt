#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_factory.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";

GLuint makeLinkedProgram(MockBackend& backend) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}
} // namespace

// Unknown pname is rejected with GL_INVALID_ENUM (no backend reflection needed).
TEST_CASE("get_active_atomic_counter_buffer_rejects_unknown_pname") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);

    GLint v = 0;
    glGetActiveAtomicCounterBufferiv(prog, 0, GL_BLEND, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    glDeleteProgram(prog);
    setCurrentContext(nullptr);
}

// Scalar reflection: BINDING / DATA_SIZE / ACTIVE_ATOMIC_COUNTERS / referenced-by.
TEST_CASE("get_active_atomic_counter_buffer_reads_scalar_props") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    auto* p = factory.lastCreatedProgram;
    p->atomicCounterBuffers.resize(1);
    p->atomicCounterBuffers[0].binding = 3;
    p->atomicCounterBuffers[0].dataSize = 16;
    p->atomicCounterBuffers[0].referencedByVertex = 1;
    p->atomicCounterBuffers[0].referencedByFragment = 0;

    GLint v = -1;
    glGetActiveAtomicCounterBufferiv(prog, 0, GL_ATOMIC_COUNTER_BUFFER_BINDING, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(v, 3);

    glGetActiveAtomicCounterBufferiv(prog, 0, GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE, &v);
    EXPECT_EQ(v, 16);

    glGetActiveAtomicCounterBufferiv(
        prog, 0, GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS, &v);
    EXPECT_EQ(v, 0);

    glGetActiveAtomicCounterBufferiv(
        prog, 0, GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_VERTEX_SHADER, &v);
    EXPECT_EQ(v, 1);

    glDeleteProgram(prog);
    setCurrentContext(nullptr);
}

// The ACTIVE_ATOMIC_COUNTER_INDICES pname writes the full variable-length array.
TEST_CASE("get_active_atomic_counter_buffer_writes_index_array") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    auto* p = factory.lastCreatedProgram;
    p->atomicCounterBuffers.resize(1);
    p->atomicCounterBuffers[0].indices = {4, 5, 6};

    GLint out[4] = {-1, -1, -1, -1};
    glGetActiveAtomicCounterBufferiv(
        prog, 0, GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES, out);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(out[0], 4);
    EXPECT_EQ(out[1], 5);
    EXPECT_EQ(out[2], 6);
    EXPECT_EQ(out[3], -1); // untouched

    glDeleteProgram(prog);
    setCurrentContext(nullptr);
}

// A null params pointer is rejected with GL_INVALID_VALUE.
TEST_CASE("get_active_atomic_counter_buffer_rejects_null_params") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);

    glGetActiveAtomicCounterBufferiv(
        prog, 0, GL_ATOMIC_COUNTER_BUFFER_BINDING, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glDeleteProgram(prog);
    setCurrentContext(nullptr);
}

// An out-of-range buffer index is rejected with GL_INVALID_VALUE.
TEST_CASE("get_active_atomic_counter_buffer_rejects_bad_index") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);

    GLint v = 0;
    glGetActiveAtomicCounterBufferiv(
        prog, 5, GL_ATOMIC_COUNTER_BUFFER_BINDING, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glDeleteProgram(prog);
    setCurrentContext(nullptr);
}
