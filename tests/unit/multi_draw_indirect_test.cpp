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

// glMultiDrawArraysIndirect (SPEC §10, ARB_multi_draw_indirect) records the
// draw count, stride, and byte offset forwarded to the backend.
TEST_CASE("multi_draw_arrays_indirect_forwards_count_stride_offset") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    glUseProgram(3);

    glMultiDrawArraysIndirect(GL_TRIANGLES,
                              reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(32)),
                              4, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.multiDrawArraysIndirectCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawIndirectCount, 4);
    EXPECT_EQ(backend.lastDrawIndirectStride, 0);
    EXPECT_EQ(backend.lastIndirect,
              reinterpret_cast<const void*>(static_cast<intptr_t>(32)));

    setCurrentContext(nullptr);
}

// glMultiDrawElementsIndirect forwards mode/type/count/stride/offset.
TEST_CASE("multi_draw_elements_indirect_forwards_all_params") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    glUseProgram(1);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT,
                                reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)),
                                3, 48);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.multiDrawElementsIndirectCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawIndirectCount, 3);
    EXPECT_EQ(backend.lastDrawIndirectStride, 48);

    setCurrentContext(nullptr);
}

// Like the single indirect draws, an unbound indirect buffer is rejected.
TEST_CASE("multi_draw_indirect_requires_indirect_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(2);
    glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, 2, 0);
    EXPECT_EQ(backend.multiDrawArraysIndirectCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// An active program is required.
TEST_CASE("multi_draw_indirect_requires_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    glUseProgram(0);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);
    EXPECT_EQ(backend.multiDrawElementsIndirectCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// stride == 0 (tightly packed) is a legal specialization and must still record.
TEST_CASE("multi_draw_indirect_zero_stride_is_valid") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    glUseProgram(4);
    glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, 5, 0);
    EXPECT_EQ(backend.multiDrawArraysIndirectCalls, 1);
    EXPECT_EQ(backend.lastDrawIndirectCount, 5);
    EXPECT_EQ(backend.lastDrawIndirectStride, 0);

    setCurrentContext(nullptr);
}
