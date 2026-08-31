#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

// glMultiDrawArraysIndirectCount (SPEC §10.4, GL 4.6) forwards the parameter-buffer
// offset, max draw count and stride, but only when a GL_PARAMETER_BUFFER is bound.
TEST_CASE("multi_draw_arrays_indirect_count_forwards_params") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint ibuf = 0, pbuf = 0;
    glGenBuffers(1, &ibuf);
    glGenBuffers(1, &pbuf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ibuf);
    glBindBuffer(GL_PARAMETER_BUFFER, pbuf);
    glUseProgram(3);

    glMultiDrawArraysIndirectCount(GL_TRIANGLES,
                                   reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(16)),
                                   static_cast<GLintptr>(32), 7, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.multiDrawArraysIndirectCountCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawIndirectCountParam, 32);
    EXPECT_EQ(backend.lastDrawIndirectMaxCount, 7);
    EXPECT_EQ(backend.lastDrawIndirectStride, 0);

    setCurrentContext(nullptr);
}

// glMultiDrawElementsIndirectCount also forwards the index type.
TEST_CASE("multi_draw_elements_indirect_count_forwards_type") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint ibuf = 0, pbuf = 0;
    glGenBuffers(1, &ibuf);
    glGenBuffers(1, &pbuf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ibuf);
    glBindBuffer(GL_PARAMETER_BUFFER, pbuf);
    glUseProgram(1);

    glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT,
                                     reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)),
                                     static_cast<GLintptr>(8), 16, 48);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.multiDrawElementsIndirectCountCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawIndirectCountParam, 8);
    EXPECT_EQ(backend.lastDrawIndirectMaxCount, 16);
    EXPECT_EQ(backend.lastDrawIndirectStride, 48);

    setCurrentContext(nullptr);
}

// Without a GL_PARAMETER_BUFFER bound the command is rejected (SPEC §10.4).
TEST_CASE("multi_draw_indirect_count_requires_parameter_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint ibuf = 0;
    glGenBuffers(1, &ibuf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ibuf);
    glUseProgram(2);

    glMultiDrawArraysIndirectCount(GL_TRIANGLES, nullptr, 4, 10, 0);
    EXPECT_EQ(backend.multiDrawArraysIndirectCountCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// The parameter-buffer offset must be a multiple of four (SPEC §10.4).
TEST_CASE("multi_draw_indirect_count_offset_must_be_aligned") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint ibuf = 0, pbuf = 0;
    glGenBuffers(1, &ibuf);
    glGenBuffers(1, &pbuf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ibuf);
    glBindBuffer(GL_PARAMETER_BUFFER, pbuf);
    glUseProgram(2);

    glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 6, 10, 0);
    EXPECT_EQ(backend.multiDrawElementsIndirectCountCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
