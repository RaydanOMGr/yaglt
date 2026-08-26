#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <string>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

TEST_CASE("create_compile_link_program_end_to_end") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("#version 330 core\nvoid main(){}"));
    glCompileShader(vs);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_TRUE);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, nullptr, nullptr); // empty -> fails to compile
    glCompileShader(fs);
    EXPECT_EQ(glGetShaderiv(fs, GL_COMPILE_STATUS), GL_FALSE);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    // Attaching an uncompiled shader must report an error honestly.
    glAttachShader(prog, fs);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Attrib location is stable and assigned per name.
    int locA = ctx.getAttribLocation(prog, "a_position");
    int locB = ctx.getAttribLocation(prog, "a_color");
    EXPECT_NE(locA, locB);
    EXPECT_EQ(ctx.getAttribLocation(prog, "a_position"), locA);

    setCurrentContext(nullptr);
}

TEST_CASE("use_program_binds_native_id_at_draw") {
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
    glBindVertexArray(0); // no VAO bound: attrib calls must error
    glEnableVertexAttribArray(0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.drawArraysCalls, 1);
    // Program flushed (name recorded by the mock sink, as in state_flush_test).
    EXPECT_EQ(backend.useProgramCalls, 1);
    EXPECT_EQ(backend.lastProgram, prog);
    EXPECT_EQ(backend.bindVertexArrayCalls, 1);
    EXPECT_EQ(backend.lastBindVertexArray, vao);
    EXPECT_EQ(backend.enableVertexAttribArrayCalls, 1);
    EXPECT_EQ(backend.disableVertexAttribArrayCalls, 0);
    EXPECT_EQ(backend.vertexAttribPointerCalls, 1);
    EXPECT_EQ(backend.lastAttribIndex, 0u);
    EXPECT_EQ(backend.lastAttribSize, 3);
    EXPECT_EQ(backend.lastAttribType, GL_FLOAT);

    setCurrentContext(nullptr);
}

TEST_CASE("vertex_state_flushed_only_when_dirty") {
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
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // enableVertexAttribArray pushes a default attrib pointer at flush.
    EXPECT_EQ(backend.vertexAttribPointerCalls, 1);
    EXPECT_EQ(backend.enableVertexAttribArrayCalls, 1);

    // Second draw with unchanged attribs: no re-push.
    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.enableVertexAttribArrayCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("shader_program_capability_gate") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Mock supports shaders/programs; creating them succeeds.
    EXPECT_NE(glCreateShader(GL_VERTEX_SHADER), 0u);
    EXPECT_NE(glCreateProgram(), 0u);

    // Compiling a non-existent shader name reports an error.
    glCompileShader(99999);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
