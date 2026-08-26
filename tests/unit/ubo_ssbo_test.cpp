#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_BOGUS_TARGET = 0xDEAD;
} // namespace

TEST_CASE("ubo_ssbo_bind_buffer_base_records_and_validates") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    EXPECT_NE(buf, 0u);

    // UBO is Native in the mock profile: binding forwards to the sink.
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.bindBufferBaseCalls, 1);
    EXPECT_EQ(backend.lastBindTarget, GL_UNIFORM_BUFFER);
    EXPECT_EQ(backend.lastBindIndex, 2u);
    EXPECT_EQ(backend.lastBindBuffer, buf);

    // SSBO is Native in the mock profile too.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.bindBufferBaseCalls, 2);
    EXPECT_EQ(backend.lastBindTarget, GL_SHADER_STORAGE_BUFFER);

    // Binding an ungenerated name is an error; no extra native call.
    GLuint fake = 9999;
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, fake);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(backend.bindBufferBaseCalls, 2);

    setCurrentContext(nullptr);
}

TEST_CASE("ubo_ssbo_bind_buffer_base_is_capability_guarded") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);

    // An unsupported / unknown indexed target must be reported honestly and
    // must NOT reach the driver (no native call).
    glBindBufferBase(GL_BOGUS_TARGET, 0, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(backend.bindBufferBaseCalls, 0);

    // bindBufferRange is guarded the same way and forwards otherwise.
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, buf, 0, 16);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.bindBufferRangeCalls, 1);

    glBindBufferRange(GL_BOGUS_TARGET, 0, buf, 0, 16);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(backend.bindBufferRangeCalls, 1);

    setCurrentContext(nullptr);
}
