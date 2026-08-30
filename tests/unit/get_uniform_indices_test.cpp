#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
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

// glGetUniformIndices (SPEC 7.3.1) — map uniform names to their indices.
// The mock backend reports 0 active uniforms, so any name resolves to
// GL_INVALID_INDEX (no per-name error).

TEST_CASE("uniform_indices_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glCreateProgram(); // program exists but is not linked
    const char* names[] = {"x"};
    GLuint out[1] = {0};
    glGetUniformIndices(1, 1, names, out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_indices_negative_count_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    const char* names[] = {"x"};
    GLuint out[1] = {0};
    glGetUniformIndices(prog, -1, names, out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_indices_null_names_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint out[1] = {0};
    glGetUniformIndices(prog, 1, nullptr, out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_indices_null_indices_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    const char* names[] = {"x"};
    glGetUniformIndices(prog, 1, names, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_indices_name_not_found_returns_invalid_index_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    const char* names[] = {"nope"};
    GLuint out[1] = {0};
    glGetUniformIndices(prog, 1, names, out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(out[0], static_cast<GLuint>(GL_INVALID_INDEX));

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_indices_zero_count_is_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    glGetUniformIndices(prog, 0, nullptr, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_indices_each_name_resolved_independently") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    const char* names[] = {"a", "b", "c"};
    GLuint out[3] = {0, 0, 0};
    glGetUniformIndices(prog, 3, names, out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(out[0], static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(out[1], static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(out[2], static_cast<GLuint>(GL_INVALID_INDEX));

    setCurrentContext(nullptr);
}
