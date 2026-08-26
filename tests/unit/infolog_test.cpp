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

TEST_CASE("getshaderinfolog_copies_log_and_reports_length") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    // Empty source fails on the mock backend; an info log is produced.
    glShaderSource(vs, "");
    glCompileShader(vs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_FALSE);

    // The log length reported by glGetShaderiv is strlen + 1.
    GLint lenParam = glGetShaderiv(vs, GL_INFO_LOG_LENGTH);
    EXPECT_TRUE(lenParam >= 1);

    // Copy with a generous buffer; length excludes the nul terminator.
    char buf[256];
    GLsizei written = 0;
    glGetShaderInfoLog(vs, sizeof(buf), &written, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(written, lenParam - 1);
    EXPECT_EQ(static_cast<size_t>(written), std::strlen(buf));

    // bufSize == 0 writes nothing and leaves length at 0.
    GLsizei written0 = 123;
    glGetShaderInfoLog(vs, 0, &written0, buf);
    EXPECT_EQ(written0, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("getshaderinfolog_truncates_to_bufsize") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, "");
    glCompileShader(vs);

    char small[4];
    GLsizei written = 0;
    glGetShaderInfoLog(vs, sizeof(small), &written, small);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(written, 3);                       // bufSize-1 characters
    EXPECT_EQ(small[3], '\0');                   // nul terminated

    setCurrentContext(nullptr);
}

TEST_CASE("getshaderinfolog_unknown_shader_errors") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    char buf[16];
    GLsizei written = 0;
    glGetShaderInfoLog(9999, sizeof(buf), &written, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(written, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("getprograminfolog_copies_log") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = glCreateProgram();
    // Linking a program with no attached shaders fails on the mock backend.
    glLinkProgram(prog);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION); // link failure is reported
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_FALSE);
    EXPECT_TRUE(glGetProgramiv(prog, GL_INFO_LOG_LENGTH) >= 1);

    char buf[256];
    GLsizei written = 0;
    glGetProgramInfoLog(prog, sizeof(buf), &written, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_TRUE(written >= 1);
    EXPECT_EQ(static_cast<size_t>(written), std::strlen(buf));

    setCurrentContext(nullptr);
}
