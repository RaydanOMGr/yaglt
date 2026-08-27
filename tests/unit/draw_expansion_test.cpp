#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

TEST_CASE("vertex_attrib_divisor_pushed_only_when_nonzero") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    glUseProgram(prog);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // divisor 0 (the GL default) must not issue a native call.
    glVertexAttribDivisor(0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.vertexAttribDivisorCalls, 0);

    // A non-zero divisor is pushed at flush time.
    glVertexAttribDivisor(0, 2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.vertexAttribDivisorCalls, 1);
    EXPECT_EQ(backend.lastAttribDivisorIndex, 0u);
    EXPECT_EQ(backend.lastAttribDivisor, 2u);

    // No VAO bound -> INVALID_OPERATION.
    glBindVertexArray(0);
    glVertexAttribDivisor(1, 1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("multi_draw_arrays_records_drawcount") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    int32_t firsts[2] = {0, 3};
    int32_t counts[2] = {3, 3};
    glMultiDrawArrays(GL_TRIANGLES, firsts, counts, 2);
    EXPECT_EQ(backend.multiDrawArraysCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastMultiDrawCount, 2);

    // Negative drawcount -> INVALID_VALUE, no native call.
    glMultiDrawArrays(GL_TRIANGLES, firsts, counts, -1);
    EXPECT_EQ(backend.multiDrawArraysCalls, 1);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // No program -> INVALID_OPERATION.
    glUseProgram(0);
    glMultiDrawArrays(GL_TRIANGLES, firsts, counts, 2);
    EXPECT_EQ(backend.multiDrawArraysCalls, 1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("multi_draw_elements_records_drawcount") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    int32_t counts[2] = {6, 6};
    intptr_t indices[2] = {0, 12};
    glMultiDrawElements(GL_TRIANGLES, counts, GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid* const*>(indices), 2);
    EXPECT_EQ(backend.multiDrawElementsCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastMultiDrawCount, 2);

    setCurrentContext(nullptr);
}

TEST_CASE("draw_range_elements_validates_range_and_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    // end < start -> INVALID_VALUE, no native call.
    glDrawRangeElements(GL_TRIANGLES, 10, 4, 3, GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)));
    EXPECT_EQ(backend.drawRangeElementsCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Valid range is recorded.
    glDrawRangeElements(GL_TRIANGLES, 0, 8, 3, GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(4)));
    EXPECT_EQ(backend.drawRangeElementsCalls, 1);
    EXPECT_EQ(backend.lastDrawStart, 0u);
    EXPECT_EQ(backend.lastDrawEnd, 8u);
    EXPECT_EQ(backend.lastDrawCount, 3);
    EXPECT_EQ(backend.lastDrawIndices, 4);

    setCurrentContext(nullptr);
}

TEST_CASE("draw_elements_base_vertex_requires_program_and_capability") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Mock marks DrawElementsBaseVertex Native, so capability is satisfied.
    EXPECT_TRUE(backend.capabilities().isSupported(Feature::DrawElementsBaseVertex));

    // No program -> INVALID_OPERATION.
    glDrawElementsBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_INT,
                            reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)), 5);
    EXPECT_EQ(backend.drawElementsBaseVertexCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glUseProgram(1);
    glDrawElementsBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_INT,
                            reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(0)), 5);
    EXPECT_EQ(backend.drawElementsBaseVertexCalls, 1);
    EXPECT_EQ(backend.lastDrawCount, 3);
    EXPECT_EQ(backend.lastDrawBasevertex, 5);

    setCurrentContext(nullptr);
}
