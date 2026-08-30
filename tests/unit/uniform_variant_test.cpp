#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cmath>
#include <string>

using namespace glcompat;

namespace {
constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;

GLuint makeLinkedProgram(Context& ctx) {
    GLuint vs = ctx.createShader(GL_VERTEX_SHADER);
    ctx.shaderSource(vs, "void main(){}");
    ctx.compileShader(vs);
    GLuint prog = ctx.createProgram();
    ctx.attachShader(prog, vs);
    ctx.linkProgram(prog);
    return prog;
}
} // namespace

TEST_CASE("uniform_double_scalar_records_and_roundtrips") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    ProgramObject* po = ctx.getProgram(prog);
    auto* mp = static_cast<MockProgram*>(po->backend.get());

    int ld = glGetUniformLocation(prog, "d");
    glUniform1d(ld, 1.5);
    glUniform2d(ld, 1.5, 2.5);
    glUniform3d(ld, 1.5, 2.5, 3.5);
    glUniform4d(ld, 1.5, 2.5, 3.5, 4.5);
    EXPECT_EQ(mp->uniform1dCalls, 1);
    EXPECT_EQ(mp->uniform4dCalls, 1);
    EXPECT_TRUE(std::abs(mp->lastD0 - 1.5) < 1e-9);
    EXPECT_TRUE(std::abs(mp->lastD3 - 4.5) < 1e-9);

    // Readback round-trips through the mock's double store.
    double out[4] = {0, 0, 0, 0};
    glGetUniformdv(prog, ld, out);
    EXPECT_TRUE(std::abs(out[3] - 4.5) < 1e-9);
    EXPECT_EQ(mp->getUniformdvCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_double_vector_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    auto* mp = static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());

    int ld = glGetUniformLocation(prog, "dv");
    double arr[4] = {1.0, 2.0, 3.0, 4.0};
    glUniform1dv(ld, 3, arr);
    glUniform3dv(ld, 1, arr);
    glUniform4dv(ld, 1, arr);
    EXPECT_EQ(mp->uniform1dvCalls, 1);
    EXPECT_EQ(mp->uniform3dvCalls, 1);
    EXPECT_EQ(mp->uniform4dvCalls, 1);
    EXPECT_EQ(mp->lastUniformCount, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_unsigned_records_and_roundtrips") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    auto* mp = static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());

    int lu = glGetUniformLocation(prog, "u");
    glUniform1ui(lu, 11u);
    glUniform2ui(lu, 11u, 22u);
    glUniform3ui(lu, 11u, 22u, 33u);
    glUniform4ui(lu, 11u, 22u, 33u, 44u);
    EXPECT_EQ(mp->uniform1uiCalls, 1);
    EXPECT_EQ(mp->uniform4uiCalls, 1);
    EXPECT_EQ(mp->lastU0, 11u);
    EXPECT_EQ(mp->lastU3, 44u);

    // Readback reflects the most recent scalar write before the vector writes.
    GLuint out[4] = {0, 0, 0, 0};
    glGetUniformuiv(prog, lu, out);
    EXPECT_EQ(out[3], 44u);
    EXPECT_EQ(mp->getUniformuivCalls, 1);

    GLuint uarr[2] = {5u, 6u};
    glUniform1uiv(lu, 2, uarr);
    glUniform2uiv(lu, 1, uarr);
    EXPECT_EQ(mp->uniform1uivCalls, 1);
    EXPECT_EQ(mp->uniform2uivCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_vector_and_matrix_variants_record") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    auto* mp = static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());

    int lf = glGetUniformLocation(prog, "f");
    float fa[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    glUniform2fv(lf, 1, fa);
    glUniform3fv(lf, 1, fa);
    glUniform4fv(lf, 1, fa);
    EXPECT_EQ(mp->uniform2fvCalls, 1);
    EXPECT_EQ(mp->uniform3fvCalls, 1);
    EXPECT_EQ(mp->uniform4fvCalls, 1);

    int li = glGetUniformLocation(prog, "i");
    int ia[4] = {7, 8, 9, 10};
    glUniform2iv(li, 1, ia);
    glUniform3iv(li, 1, ia);
    glUniform4iv(li, 1, ia);
    EXPECT_EQ(mp->uniform2ivCalls, 1);
    EXPECT_EQ(mp->uniform4ivCalls, 1);

    float m2[4] = {0};
    float m3[9] = {0};
    double m4d[16] = {0};
    glUniformMatrix2fv(lf, 1, false, m2);
    glUniformMatrix3fv(lf, 1, false, m3);
    glUniformMatrix4dv(lf, 1, false, m4d);
    EXPECT_EQ(mp->uniformMatrix2fvCalls, 1);
    EXPECT_EQ(mp->uniformMatrix3fvCalls, 1);
    EXPECT_EQ(mp->uniformMatrix4dvCalls, 1);
    EXPECT_EQ(mp->lastTranspose, false);

    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_variants_target_explicit_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    // No glUseProgram: explicit-program setters must still reach the backend.
    auto* mp = static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());

    int ld = glGetUniformLocation(prog, "d");
    glProgramUniform1d(prog, ld, 1.25);
    glProgramUniform3d(prog, ld, 1.0, 2.0, 3.0);
    EXPECT_EQ(mp->uniform1dCalls, 1);
    EXPECT_EQ(mp->uniform3dCalls, 1);

    int lu = glGetUniformLocation(prog, "u");
    glProgramUniform2ui(prog, lu, 9u, 8u);
    EXPECT_EQ(mp->uniform2uiCalls, 1);

    int lf = glGetUniformLocation(prog, "f");
    float fa[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    glProgramUniform2fv(prog, lf, 1, fa);
    glProgramUniformMatrix2fv(prog, lf, 1, false, fa);
    EXPECT_EQ(mp->uniform2fvCalls, 1);
    EXPECT_EQ(mp->uniformMatrix2fvCalls, 1);

    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_on_unlinked_program_errors") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = ctx.createProgram(); // never linked
    glProgramUniform1d(prog, 0, 1.0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
