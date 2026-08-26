#include "test_framework.hpp"

#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/backend/gles/gles_backend.hpp"

#include <cstdint>
#include <string>

// End-to-end: compile + link a desktop GLSL program through the real GLES backend
// (desktop -> glslang -> SPIRV-Cross -> GLSL ES -> Mesa driver) and draw. Skips
// cleanly when no driver is available (initialize() == false).
//
// This TU includes the native GLES3 headers (via the backend), whose GL_* macros
// would collide with the frontend's glcompat::GL_* constants, so we use the
// native macros/values directly for any GL enumeration argument here.
TEST_CASE("gles_e2e_program_link_and_draw") {
    glcompat::GLESBackend backend;
    if (!backend.initialize()) {
        // No driver (e.g. CI without Mesa): nothing to exercise.
        return;
    }

    glcompat::Context ctx(backend);
    glcompat::setCurrentContext(&ctx);

    const std::string vs =
        "#version 330 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const std::string fs =
        "#version 330 core\n"
        "out vec4 o_color;\n"
        "void main() { o_color = vec4(1.0); }\n";

    glcompat::GLuint vsh = glcompat::glCreateShader(GL_VERTEX_SHADER);
    glcompat::glShaderSource(vsh, vs);
    glcompat::glCompileShader(vsh);
    EXPECT_EQ(glcompat::glGetShaderiv(vsh, GL_COMPILE_STATUS), GL_TRUE);

    glcompat::GLuint fsh = glcompat::glCreateShader(GL_FRAGMENT_SHADER);
    glcompat::glShaderSource(fsh, fs);
    glcompat::glCompileShader(fsh);
    EXPECT_EQ(glcompat::glGetShaderiv(fsh, GL_COMPILE_STATUS), GL_TRUE);

    glcompat::GLuint prog = glcompat::glCreateProgram();
    glcompat::glAttachShader(prog, vsh);
    glcompat::glAttachShader(prog, fsh);
    glcompat::glLinkProgram(prog);
    EXPECT_EQ(glcompat::glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    // A real native program id was registered (frontend -> driver mapping).
    EXPECT_NE(backend.nativeMap().count(prog), 0u);

    glcompat::glUseProgram(prog);

    glcompat::GLuint vao = 0;
    glcompat::glGenVertexArrays(1, &vao);
    glcompat::glBindVertexArray(vao);
    glcompat::glEnableVertexAttribArray(0);
    glcompat::glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);

    // Drawing with a linked program flushes state then issues the native draw.
    // With a real driver the native draw executes; assert no GL error results.
    glcompat::glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(glcompat::glGetError(), 0);

    glcompat::setCurrentContext(nullptr);
}

// End-to-end: set a uniform on a linked program through the real driver
// (desktop uniform -> translator -> GLES backend -> Mesa). Exercises the
// GLESBackendProgram uniform path (bind + glUniform*) against a real driver and
// skips cleanly when no driver is available.
TEST_CASE("gles_e2e_uniform_set") {
    glcompat::GLESBackend backend;
    if (!backend.initialize()) return; // no driver: nothing to exercise

    glcompat::Context ctx(backend);
    glcompat::setCurrentContext(&ctx);

    const std::string vs =
        "#version 330 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const std::string fs =
        "#version 330 core\n"
        "uniform float u_alpha;\n"
        "out vec4 o_color;\n"
        "void main() { o_color = vec4(u_alpha); }\n";

    glcompat::GLuint vsh = glcompat::glCreateShader(GL_VERTEX_SHADER);
    glcompat::glShaderSource(vsh, vs);
    glcompat::glCompileShader(vsh);
    EXPECT_EQ(glcompat::glGetShaderiv(vsh, GL_COMPILE_STATUS), GL_TRUE);

    glcompat::GLuint fsh = glcompat::glCreateShader(GL_FRAGMENT_SHADER);
    glcompat::glShaderSource(fsh, fs);
    glcompat::glCompileShader(fsh);
    EXPECT_EQ(glcompat::glGetShaderiv(fsh, GL_COMPILE_STATUS), GL_TRUE);

    glcompat::GLuint prog = glcompat::glCreateProgram();
    glcompat::glAttachShader(prog, vsh);
    glcompat::glAttachShader(prog, fsh);
    glcompat::glLinkProgram(prog);
    EXPECT_EQ(glcompat::glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    glcompat::glUseProgram(prog);

    glcompat::GLint loc = glcompat::glGetUniformLocation(prog, "u_alpha");
    EXPECT_TRUE(loc >= 0);
    glcompat::glUniform1f(loc, 0.5f);
    EXPECT_EQ(glcompat::glGetError(), 0);

    glcompat::setCurrentContext(nullptr);
}
