#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cmath>
#include <string>

using namespace glcompat;

namespace {
constexpr GLenum GL_VERTEX_SHADER = 0x8B31;

GLuint makeLinkedProgram(Context& ctx) {
    GLuint vs = ctx.createShader(GL_VERTEX_SHADER);
    ctx.shaderSource(vs, "void main(){}");
    ctx.compileShader(vs);
    GLuint prog = ctx.createProgram();
    ctx.attachShader(prog, vs);
    ctx.linkProgram(prog);
    return prog;
}

MockProgram* mockProgOf(Context& ctx, GLuint prog) {
    ProgramObject* po = ctx.getProgram(prog);
    return static_cast<MockProgram*>(po->backend.get());
}
} // namespace

TEST_CASE("program_uniform_targets_explicit_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint progA = makeLinkedProgram(ctx);
    GLuint progB = makeLinkedProgram(ctx);
    int locA = glGetUniformLocation(progA, "u_val");
    int locB = glGetUniformLocation(progB, "u_val");
    EXPECT_TRUE(locA >= 0 && locB >= 0);

    // No glUseProgram: glProgramUniform* still reaches the named program.
    glProgramUniform1f(progA, locA, 2.5f);

    EXPECT_EQ(mockProgOf(ctx, progA)->uniform1fCalls, 1);
    EXPECT_EQ(mockProgOf(ctx, progA)->lastF0, 2.5f);
    EXPECT_EQ(mockProgOf(ctx, progB)->uniform1fCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_rejects_unlinked_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = ctx.createProgram(); // never linked
    glProgramUniform1f(prog, 0, 1.0f);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    GLuint linked = makeLinkedProgram(ctx);
    // program 0 is never a valid program object.
    glProgramUniform1i(0, 0, 1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_minus_one_location_is_silent_noop") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glProgramUniform1f(prog, -1, 1.0f);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(mockProgOf(ctx, prog)->uniform1fCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_variants_record_correctly") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    auto* mp = mockProgOf(ctx, prog);
    int lf = glGetUniformLocation(prog, "f");
    int li = glGetUniformLocation(prog, "i");

    glProgramUniform2f(prog, lf, 1.0f, 2.0f);
    glProgramUniform3f(prog, lf, 1.0f, 2.0f, 3.0f);
    glProgramUniform4f(prog, lf, 1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(mp->uniform2fCalls, 1);
    EXPECT_EQ(mp->uniform3fCalls, 1);
    EXPECT_EQ(mp->uniform4fCalls, 1);

    glProgramUniform1i(prog, li, 7);
    glProgramUniform2i(prog, li, 7, 8);
    EXPECT_EQ(mp->uniform1iCalls, 1);
    EXPECT_EQ(mp->lastI0, 7);

    float arr[3] = {1.0f, 2.0f, 3.0f};
    glProgramUniform1fv(prog, lf, 3, arr);
    EXPECT_EQ(mp->uniform1fvCalls, 1);
    EXPECT_EQ(mp->lastUniformCount, 3);

    float mat[16] = {0};
    mat[0] = 1.0f;
    glProgramUniformMatrix4fv(prog, lf, 1, false, mat);
    EXPECT_EQ(mp->uniformMatrix4fvCalls, 1);
    EXPECT_EQ(mp->lastUniformCount, 1);
    EXPECT_EQ(mp->lastTranspose, false);

    setCurrentContext(nullptr);
}
