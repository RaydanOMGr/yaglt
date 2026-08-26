#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <string>

using namespace glcompat;

TEST_CASE("glsl_version_supported_desktop_compiles") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}");
    glCompileShader(vs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_TRUE);

    setCurrentContext(nullptr);
}

TEST_CASE("glsl_version_default_no_directive_compiles") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, "void main(){gl_Position=vec4(0.0);}");
    glCompileShader(vs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_TRUE);

    setCurrentContext(nullptr);
}

TEST_CASE("glsl_version_unsupported_desktop_rejected_before_translate") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, "#version 999 core\nvoid main(){gl_Position=vec4(0.0);}");
    glCompileShader(vs);
    // Compile failure is reported via COMPILE_STATUS + info log, not glGetError.
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_FALSE);

    char buf[256];
    GLsizei written = 0;
    glGetShaderInfoLog(vs, sizeof(buf), &written, buf);
    EXPECT_TRUE(written > 0);
    EXPECT_TRUE(std::string(buf).find("unsupported GLSL version") !=
                std::string::npos);

    setCurrentContext(nullptr);
}

TEST_CASE("glsl_version_supported_es_compiles") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, "#version 310 es\nprecision mediump float;\nvoid main(){}");
    glCompileShader(fs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(fs, GL_COMPILE_STATUS), GL_TRUE);

    setCurrentContext(nullptr);
}

TEST_CASE("glsl_version_unsupported_es_rejected") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, "#version 999 es\nvoid main(){}");
    glCompileShader(fs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(fs, GL_COMPILE_STATUS), GL_FALSE);

    char buf[256];
    GLsizei written = 0;
    glGetShaderInfoLog(fs, sizeof(buf), &written, buf);
    EXPECT_TRUE(std::string(buf).find("unsupported GLSL version") !=
                std::string::npos);

    setCurrentContext(nullptr);
}
