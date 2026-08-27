#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <string>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
constexpr GLenum kBadStage = 0xDEAD;
} // namespace

static GLuint makeLinkedProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}

TEST_CASE("sub_index_not_found_is_honest_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetSubroutineIndex(prog, GL_VERTEX_SHADER, "nonexistent"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_index_invalid_stage_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetSubroutineIndex(prog, kBadStage, "x"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_uniform_loc_not_found_is_honest_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetSubroutineUniformLocation(prog, GL_FRAGMENT_SHADER, "nonexistent"),
              -1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_uniform_loc_invalid_stage_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetSubroutineUniformLocation(prog, kBadStage, "x"), -1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_active_uniform_iv_requires_values") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    // Null values buffer is GL_INVALID_VALUE.
    glGetActiveSubroutineUniformiv(prog, GL_VERTEX_SHADER, 0,
                                  GL_NUM_COMPATIBLE_SUBROUTINES, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // A valid call writes nothing (Mock has no introspection) and stays clean.
    GLint out = -1;
    glGetActiveSubroutineUniformiv(prog, GL_VERTEX_SHADER, 0,
                                  GL_NUM_COMPATIBLE_SUBROUTINES, &out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_active_uniform_name_buffer_edge_cases") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char buf[32] = {0};
    // Negative buffer size is GL_INVALID_VALUE.
    glGetActiveSubroutineUniformName(prog, GL_VERTEX_SHADER, 0, -1, nullptr, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    // bufSize == 0 writes nothing (no error).
    glGetActiveSubroutineUniformName(prog, GL_VERTEX_SHADER, 0, 0, nullptr, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_active_name_buffer_edge_cases") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char buf[32] = {0};
    glGetActiveSubroutineName(prog, GL_VERTEX_SHADER, 0, -1, nullptr, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetActiveSubroutineName(prog, GL_VERTEX_SHADER, 0, 0, nullptr, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_uniform_subroutines_requires_active_program_and_valid_stage") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint idx = 0;
    // No active program yet -> GL_INVALID_OPERATION.
    glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &idx);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glUseProgram(prog);
    // Bind a program, then an invalid stage is still an error.
    glUniformSubroutinesuiv(kBadStage, 1, &idx);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    // Negative count is GL_INVALID_VALUE.
    glUniformSubroutinesuiv(GL_VERTEX_SHADER, -1, &idx);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    // Valid call forwards to the backend (no-op on Mock) without error.
    glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &idx);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sub_get_uniform_subroutine_requires_active_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint out = 0;
    // No active program -> GL_INVALID_OPERATION.
    glGetUniformSubroutineuiv(GL_VERTEX_SHADER, 0, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glUseProgram(prog);
    // Null params is GL_INVALID_VALUE.
    glGetUniformSubroutineuiv(GL_VERTEX_SHADER, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    // Valid call reads (0 on Mock) without error.
    glGetUniformSubroutineuiv(GL_VERTEX_SHADER, 0, &out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}
