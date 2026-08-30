#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cmath>
#include <string>

using namespace glcompat;

namespace {

// Uniform value queries (SPEC §7.9 glGetUniform{f,i,ui,d}v and the robust
// glGetnUniform* variants). The frontend validates the program/link state, the
// location, and a null params; the backend returns the stored value.

GLuint linkMockProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}

TEST_CASE("uniform_get_round_trip_float") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    GLint loc = glGetUniformLocation(prog, "u_color");
    EXPECT_NE(loc, -1);
    glProgramUniform4f(prog, loc, 0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    float v[4] = {};
    glGetUniformfv(prog, loc, v);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_TRUE(std::fabs(v[0] - 0.1f) < 1e-5f);
    EXPECT_TRUE(std::fabs(v[1] - 0.2f) < 1e-5f);
    EXPECT_TRUE(std::fabs(v[2] - 0.3f) < 1e-5f);
    EXPECT_TRUE(std::fabs(v[3] - 0.4f) < 1e-5f);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_get_round_trip_int") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    GLint loc = glGetUniformLocation(prog, "u_mode");
    glProgramUniform2i(prog, loc, 7, 9);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint iv[2] = {};
    glGetUniformiv(prog, loc, iv);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(iv[0], 7);
    EXPECT_EQ(iv[1], 9);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_get_double_widens_float_store") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    GLint loc = glGetUniformLocation(prog, "u_scale");
    glProgramUniform1f(prog, loc, 2.5f);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLdouble d = 0;
    glGetUniformdv(prog, loc, &d);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_TRUE(std::fabs(d - 2.5) < 1e-9);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_get_unlinked_program_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = glCreateProgram(); // never linked
    float v = 0;
    glGetUniformfv(prog, 0, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_get_negative_location_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    float v = 0;
    glGetUniformfv(prog, -1, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_get_null_params_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    GLint loc = glGetUniformLocation(prog, "u_x");
    glGetUniformfv(prog, loc, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_getn_negative_bufsize_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    GLint loc = glGetUniformLocation(prog, "u_x");
    float v = 0;
    glGetnUniformfv(prog, loc, -1, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Valid bufSize round-trips like the unbounded variant.
    glProgramUniform1f(prog, loc, 1.0f);
    glGetnUniformfv(prog, loc, sizeof(float), &v);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_TRUE(std::fabs(v - 1.0f) < 1e-5f);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_get_unsigned_variant_records_call") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = linkMockProgram();
    GLint loc = glGetUniformLocation(prog, "u_u");
    GLuint u = 0;
    glGetUniformuiv(prog, loc, &u);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

} // namespace
