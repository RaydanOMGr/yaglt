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
constexpr GLenum kBadInterface = 0xDEAD;
} // namespace

// Helper: build and link a minimal program on the mock backend.
static GLuint makeLinkedProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}

TEST_CASE("pr_index_invalid_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(glGetProgramResourceIndex(9999, GL_UNIFORM, "x"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_index_bad_interface_is_enum_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetProgramResourceIndex(prog, kBadInterface, "x"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_index_not_found_is_honest_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    // The mock has no introspection, so a name is not found; SPEC returns
    // GL_INVALID_INDEX and does NOT set an error.
    EXPECT_EQ(glGetProgramResourceIndex(prog, GL_UNIFORM, "nonexistent"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_name_invalid_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    char buf[16] = {0};
    glGetProgramResourceName(9999, GL_UNIFORM, 0, sizeof(buf), nullptr, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_name_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    // Mock reports 0 resources for every interface, so index 0 is out of range.
    char buf[16] = {0};
    glGetProgramResourceName(prog, GL_UNIFORM, 0, sizeof(buf), nullptr, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Negative buffer size is also GL_INVALID_VALUE.
    glGetProgramResourceName(prog, GL_UNIFORM, 0, -1, nullptr, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_name_valid_index_runs_without_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    // The mock exposes no introspection (0 resources), so index 0 is out of
    // range even with bufSize == 0; SPEC still raises GL_INVALID_VALUE.
    glGetProgramResourceName(prog, GL_UNIFORM, 0, 0, nullptr, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_location_not_found_is_honest_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetProgramResourceLocation(prog, GL_UNIFORM, "nonexistent"), -1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetProgramResourceLocationIndex(prog, GL_UNIFORM, "nonexistent"),
              -1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_iv_invalid_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLenum prop = GL_LOCATION;
    GLint out = -1;
    glGetProgramResourceiv(9999, GL_UNIFORM, 0, 1, &prop, 1, nullptr, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_iv_bad_property_is_enum_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLenum prop = kBadInterface;
    GLint out = -1;
    glGetProgramResourceiv(prog, GL_UNIFORM, 0, 1, &prop, 1, nullptr, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_iv_null_params_or_buffer_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLenum prop = GL_LOCATION;
    // Buffer too small for propCount.
    GLint out = -1;
    glGetProgramResourceiv(prog, GL_UNIFORM, 0, 1, &prop, 0, nullptr, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    // Null params with propCount > 0.
    glGetProgramResourceiv(prog, GL_UNIFORM, 0, 1, &prop, 1, nullptr, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_iv_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLenum prop = GL_LOCATION;
    GLint out = -1;
    glGetProgramResourceiv(prog, GL_UNIFORM, 0, 1, &prop, 1, nullptr, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("pr_iv_zero_props_runs_without_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint out = -1;
    // propCount == 0 still requires a valid index; the mock exposes 0 resources,
    // so index 0 is out of range -> GL_INVALID_VALUE (no crash).
    glGetProgramResourceiv(prog, GL_UNIFORM, 0, 0, nullptr, 0, nullptr, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
