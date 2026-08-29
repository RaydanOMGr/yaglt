#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cstring>
#include <string>

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

// The mock backend reports no introspection (0 active resources of every
// interface), so every index is out of range and these legacy reflection
// commands must report GL_INVALID_VALUE honestly (SPEC §7.6 / §11.1).

TEST_CASE("active_uniform_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glCreateProgram(); // program exists but is not linked
    GLuint prog = 1;
    char name[16] = {0};
    glGetActiveUniform(prog, 0, sizeof(name), nullptr, nullptr, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char name[16] = {0};
    glGetActiveUniform(prog, 0, sizeof(name), nullptr, nullptr, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_negative_bufsize_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char name[16] = {0};
    glGetActiveUniform(prog, 0, -1, nullptr, nullptr, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_attrib_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char name[16] = {0};
    glGetActiveAttrib(prog, 0, sizeof(name), nullptr, nullptr, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_block_index_not_found_is_honest_no_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    // Mock has no blocks, so the name is not found: GL_INVALID_INDEX, no error.
    EXPECT_EQ(glGetUniformBlockIndex(prog, "nope"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_block_index_invalid_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(glGetUniformBlockIndex(9999, "nope"),
              static_cast<GLuint>(GL_INVALID_INDEX));
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_block_iv_bad_pname_is_enum_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint out = -1;
    glGetActiveUniformBlockiv(prog, 0, 0xDEAD, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_block_iv_null_params_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    glGetActiveUniformBlockiv(prog, 0, GL_UNIFORM_BLOCK_BINDING, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_block_iv_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint out = -1;
    glGetActiveUniformBlockiv(prog, 0, GL_UNIFORM_BLOCK_BINDING, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_block_iv_valid_pname_maps_without_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    // Every pname maps to a known property, so the only error on index 0 (out of
    // range) is the index-value check; the pname itself is accepted.
    GLint out = -1;
    glGetActiveUniformBlockiv(prog, 0, GL_UNIFORM_BLOCK_DATA_SIZE, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_block_name_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char name[16] = {0};
    glGetActiveUniformBlockName(prog, 0, sizeof(name), nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

// glGetActiveUniformName (SPEC §7.3.1) — equivalent to GetProgramResourceName on
// the UNIFORM interface. The mock exposes 0 active uniforms, so every index is
// out of range and these legacy reflection commands report honestly.

TEST_CASE("active_uniform_name_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glCreateProgram(); // program exists but is not linked
    char name[16] = {0};
    glGetActiveUniformName(1, 0, sizeof(name), nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_name_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char name[16] = {0};
    glGetActiveUniformName(prog, 0, sizeof(name), nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniform_name_negative_bufsize_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    char name[16] = {0};
    glGetActiveUniformName(prog, 0, -1, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

// glGetActiveUniformsiv (SPEC §7.3.1) — per-index UNIFORM property query.

TEST_CASE("active_uniformsiv_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = 1; // unlinked program
    GLuint idx = 0;
    GLint out = -1;
    glGetActiveUniformsiv(prog, 1, &idx, GL_UNIFORM_TYPE, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniformsiv_negative_count_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint idx = 0;
    GLint out = -1;
    glGetActiveUniformsiv(prog, -1, &idx, GL_UNIFORM_TYPE, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniformsiv_null_indices_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLint out = -1;
    glGetActiveUniformsiv(prog, 1, nullptr, GL_UNIFORM_TYPE, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniformsiv_null_params_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint idx = 0;
    glGetActiveUniformsiv(prog, 1, &idx, GL_UNIFORM_TYPE, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniformsiv_bad_pname_is_enum_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint idx = 0;
    GLint out = -1;
    glGetActiveUniformsiv(prog, 1, &idx, 0xDEAD, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniformsiv_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint idx = 0; // mock has 0 active uniforms -> out of range
    GLint out = -1;
    glGetActiveUniformsiv(prog, 1, &idx, GL_UNIFORM_TYPE, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("active_uniformsiv_valid_pname_accepted_index_maps_only_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    GLuint idx = 0;
    GLint out = -1;
    // A known pname maps to a GetProgramResource property, so the only error is
    // the out-of-range index check (InvalidValue), not InvalidEnum.
    glGetActiveUniformsiv(prog, 1, &idx, GL_UNIFORM_SIZE, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
