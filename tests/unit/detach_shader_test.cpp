#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {

constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
constexpr char kFragSrc[] =
    "#version 330 core\nout vec4 c;\nvoid main(){c=vec4(1.0);}";

} // namespace

TEST_CASE("detach_shader_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // A non-program / non-shader name reports GL_INVALID_OPERATION (SPEC §7.4).
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string(kVertSrc));
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, std::string(kFragSrc));
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);

    glDetachShader(9999, vs);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glDetachShader(prog, 9999);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("detach_shader_removes_association") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string(kVertSrc));
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, std::string(kFragSrc));
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);

    GLsizei count = 0;
    GLuint shaders[4] = {0, 0, 0, 0};
    glGetAttachedShaders(prog, 4, &count, shaders);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(count, 2);

    // Detach the fragment shader; only the vertex shader remains attached.
    glDetachShader(prog, fs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glGetAttachedShaders(prog, 4, &count, shaders);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(shaders[0], vs);

    setCurrentContext(nullptr);
}

TEST_CASE("detach_shader_does_not_undo_successful_link") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string(kVertSrc));
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, std::string(kFragSrc));
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    // Detaching after a successful link must not break the linked program.
    glDetachShader(prog, fs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    setCurrentContext(nullptr);
}
