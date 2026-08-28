#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_factory.hpp"

#include <cstdint>
#include <cstring>
#include <string>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";

// Creates + links a program and returns its frontend name. The most recently
// created MockProgram is available via the factory for direct configuration.
GLuint makeLinkedProgram(MockBackend& backend) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}
} // namespace

// glUniformBlockBinding (SPEC §7.6.2): associates a program's uniform block
// with a uniform-buffer binding point. The mock records the association and
// reports it back through glGetActiveUniformBlockiv(UNIFORM_BLOCK_BINDING).

TEST_CASE("uniform_block_binding_records_association") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeUniformBlocks = 2;

    glUniformBlockBinding(prog, 0, 3);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(factory.lastCreatedProgram->blockBindings[0], 3u);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_block_binding_visible_via_getactiveuniformblockiv") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeUniformBlocks = 2;

    glUniformBlockBinding(prog, 1, 5);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint binding = -1;
    glGetActiveUniformBlockiv(prog, 1, GL_UNIFORM_BLOCK_BINDING, &binding);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(binding, 5);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_block_binding_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glCreateProgram(); // program exists but is not linked
    glUniformBlockBinding(1, 0, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_block_binding_block_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeUniformBlocks = 1; // only block 0 valid

    glUniformBlockBinding(prog, 1, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_block_binding_binding_point_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeUniformBlocks = 2;

    glUniformBlockBinding(prog, 0, 36); // >= kMaxUniformBufferBindings (36)
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
