#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

TEST_CASE("draw_elements_instanced_base_vertex_requires_program_and_capability") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_TRUE(backend.capabilities().isSupported(Feature::DrawElementsBaseVertex));

    // No program -> INVALID_OPERATION, no native call.
    glDrawElementsInstancedBaseVertex(
        GL_TRIANGLES, 3, GL_UNSIGNED_INT,
        reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)), 2, 5);
    EXPECT_EQ(backend.drawElementsInstancedBaseVertexCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glUseProgram(1);
    glDrawElementsInstancedBaseVertex(
        GL_TRIANGLES, 3, GL_UNSIGNED_INT,
        reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)), 2, 5);
    EXPECT_EQ(backend.drawElementsInstancedBaseVertexCalls, 1);
    EXPECT_EQ(backend.lastDrawCount, 3);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawBasevertex, 5);

    setCurrentContext(nullptr);
}

TEST_CASE("draw_range_elements_base_vertex_validates_range_and_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);

    // end < start -> INVALID_VALUE, no native call.
    glDrawRangeElementsBaseVertex(
        GL_TRIANGLES, 10, 4, 3, GL_UNSIGNED_INT,
        reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)), 5);
    EXPECT_EQ(backend.drawRangeElementsBaseVertexCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Valid range is recorded with the base vertex.
    glDrawRangeElementsBaseVertex(
        GL_TRIANGLES, 0, 8, 3, GL_UNSIGNED_INT,
        reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(4)), 7);
    EXPECT_EQ(backend.drawRangeElementsBaseVertexCalls, 1);
    EXPECT_EQ(backend.lastDrawStart, 0u);
    EXPECT_EQ(backend.lastDrawEnd, 8u);
    EXPECT_EQ(backend.lastDrawCount, 3);
    EXPECT_EQ(backend.lastDrawIndices, 4);
    EXPECT_EQ(backend.lastDrawBasevertex, 7);

    setCurrentContext(nullptr);
}

TEST_CASE("multi_draw_elements_base_vertex_records_drawcount_and_basevertex") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    GLsizei counts[2] = {6, 6};
    intptr_t indices[2] = {0, 12};
    glMultiDrawElementsBaseVertex(
        GL_TRIANGLES, counts, GL_UNSIGNED_INT,
        reinterpret_cast<const GLvoid* const*>(indices), 2, 3);
    EXPECT_EQ(backend.multiDrawElementsBaseVertexCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawCount, 2);
    EXPECT_EQ(backend.lastDrawBasevertex, 3);

    setCurrentContext(nullptr);
}
