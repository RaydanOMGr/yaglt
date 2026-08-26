#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <string>

using namespace glcompat;

namespace {
constexpr GLenum GL_BOGUS_PNAME = 0xDEAD;
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
} // namespace

TEST_CASE("getshaderiv_returns_frontend_owned_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(vs, 0u);

    // Before source/compile: type known, compile status false, source length 1.
    EXPECT_EQ(glGetShaderiv(vs, GL_SHADER_TYPE),
              static_cast<GLint>(GL_VERTEX_SHADER));
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_FALSE);
    EXPECT_EQ(glGetShaderiv(vs, GL_DELETE_STATUS), GL_FALSE);
    EXPECT_EQ(glGetShaderiv(vs, GL_SHADER_SOURCE_LENGTH), 1);

    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_TRUE);
    // Source length is recorded as source.size() + 1 (nul terminator, SPEC §7.3).
    EXPECT_EQ(glGetShaderiv(vs, GL_SHADER_SOURCE_LENGTH),
              static_cast<GLint>(std::string(kVertSrc).size() + 1));
    // Successful compile clears the info log -> length 1 (empty + nul).
    EXPECT_EQ(glGetShaderiv(vs, GL_INFO_LOG_LENGTH), 1);

    // Unknown pname is GL_INVALID_ENUM.
    EXPECT_EQ(glGetShaderiv(vs, GL_BOGUS_PNAME), 0);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Unknown shader name is GL_INVALID_OPERATION.
    EXPECT_EQ(glGetShaderiv(9999, GL_COMPILE_STATUS), 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("getprogramiv_returns_frontend_owned_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    EXPECT_NE(prog, 0u);

    // Link status false until linked; attached shaders 0.
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_FALSE);
    EXPECT_EQ(glGetProgramiv(prog, GL_ATTACHED_SHADERS), 0);

    glAttachShader(prog, vs);
    glLinkProgram(prog);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);
    EXPECT_EQ(glGetProgramiv(prog, GL_ATTACHED_SHADERS), 1);
    EXPECT_EQ(glGetProgramiv(prog, GL_INFO_LOG_LENGTH), 1);
    EXPECT_EQ(glGetProgramiv(prog, GL_DELETE_STATUS), GL_FALSE);

    // Active counts are delegated to the backend; the mock reports 0.
    EXPECT_EQ(glGetProgramiv(prog, GL_ACTIVE_UNIFORMS), 0);
    EXPECT_EQ(glGetProgramiv(prog, GL_ACTIVE_ATTRIBUTES), 0);
    EXPECT_EQ(glGetProgramiv(prog, GL_ACTIVE_UNIFORM_BLOCKS), 0);

    // Unknown pname / unknown program error honestly.
    EXPECT_EQ(glGetProgramiv(prog, GL_BOGUS_PNAME), 0);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_EQ(glGetProgramiv(9999, GL_LINK_STATUS), 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
