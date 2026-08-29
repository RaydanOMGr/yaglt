#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
constexpr GLenum kBadStage = 0xDEAD;
constexpr GLenum kBadPname = 0xDEAD;
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

TEST_CASE("program_stage_valid_query_writes_zero_and_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint value = -1;
    glGetProgramStageiv(prog, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &value);
    EXPECT_EQ(value, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_all_pnames_write_zero") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    const GLenum pnames[] = {
        GL_ACTIVE_SUBROUTINES,
        GL_ACTIVE_SUBROUTINE_UNIFORMS,
        GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS,
        GL_ACTIVE_SUBROUTINE_MAX_LENGTH,
        GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH,
        GL_MAX_SUBROUTINES,
        GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS,
    };
    for (GLenum p : pnames) {
        GLint value = -1;
        glGetProgramStageiv(prog, GL_FRAGMENT_SHADER, p, &value);
        EXPECT_EQ(value, 0);
        EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_invalid_stage_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint value = 0;
    glGetProgramStageiv(prog, kBadStage, GL_ACTIVE_SUBROUTINES, &value);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_invalid_pname_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint value = 0;
    glGetProgramStageiv(prog, GL_VERTEX_SHADER, kBadPname, &value);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_null_params_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    glGetProgramStageiv(prog, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = glCreateProgram();
    GLint value = 0;
    glGetProgramStageiv(prog, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &value);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_non_program_object_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    GLint value = 0;
    glGetProgramStageiv(buf, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &value);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("program_stage_no_context_is_safe_noop") {
    setCurrentContext(nullptr);
    GLint value = 0;
    glGetProgramStageiv(1, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &value);
    // No context: dispatch returns immediately, value untouched.
    EXPECT_EQ(value, 0);
}
