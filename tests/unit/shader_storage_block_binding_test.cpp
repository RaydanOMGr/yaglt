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

// glShaderStorageBlockBinding (SPEC §7.6.2): associates a program's shader-storage
// block with a shader-storage-buffer binding point. The mock records the
// association for test observability.

TEST_CASE("shader_storage_block_binding_records_association") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeShaderStorageBlocks = 2;

    glShaderStorageBlockBinding(prog, 0, 3);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(factory.lastCreatedProgram->shaderStorageBlockBindings[0], 3u);

    setCurrentContext(nullptr);
}

TEST_CASE("shader_storage_block_binding_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glCreateProgram(); // program exists but is not linked
    glShaderStorageBlockBinding(1, 0, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("shader_storage_block_binding_block_index_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeShaderStorageBlocks = 1; // only block 0 valid

    glShaderStorageBlockBinding(prog, 1, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("shader_storage_block_binding_binding_point_out_of_range_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(backend);
    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    factory.lastCreatedProgram->activeShaderStorageBlocks = 2;

    glShaderStorageBlockBinding(prog, 0, 8); // >= kMaxShaderStorageBufferBindings (8)
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
