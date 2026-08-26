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
constexpr GLenum GL_TRIANGLES = 0x0004;

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

TEST_CASE("get_uniform_location_stable_per_name") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    int locA = ctx.getUniformLocation(prog, "u_a");
    int locB = ctx.getUniformLocation(prog, "u_b");
    EXPECT_NE(locA, locB);
    EXPECT_EQ(ctx.getUniformLocation(prog, "u_a"), locA); // stable
    EXPECT_EQ(backend.nativeMap_.count(prog), 1u);

    setCurrentContext(nullptr);
}

TEST_CASE("get_uniform_location_rejects_unlinked") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = ctx.createProgram(); // never linked
    EXPECT_EQ(ctx.getUniformLocation(prog, "u_x"), -1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_sets_active_program_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);

    int loc = glGetUniformLocation(prog, "u_val");
    EXPECT_TRUE(loc >= 0);
    glUniform1f(loc, 2.5f);

    // The mock records uniforms on the program resource.
    ProgramObject* po = ctx.getProgram(prog);
    auto* mockProg = static_cast<MockProgram*>(po->backend.get());
    EXPECT_EQ(mockProg->uniform1fCalls, 1);
    EXPECT_TRUE(std::abs(mockProg->lastF0 - 2.5f) < 1e-5f);
    EXPECT_EQ(mockProg->lastUniformLoc, loc);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_no_active_program_errors") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx); // program exists but not glUseProgram'd
    int loc = ctx.getUniformLocation(prog, "u_val");
    ctx.uniform1f(loc, 1.0f);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_minus_one_location_is_silent_noop") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    glUniform1f(-1, 1.0f); // must not error, must not call backend
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ProgramObject* po = ctx.getProgram(prog);
    auto* mockProg = static_cast<MockProgram*>(po->backend.get());
    EXPECT_EQ(mockProg->uniform1fCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("uniform_variants_record_correctly") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    ProgramObject* po = ctx.getProgram(prog);
    auto* mockProg = static_cast<MockProgram*>(po->backend.get());

    int lf = glGetUniformLocation(prog, "f");
    glUniform2f(lf, 1.0f, 2.0f);
    glUniform3f(lf, 1.0f, 2.0f, 3.0f);
    glUniform4f(lf, 1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(mockProg->uniform2fCalls, 1);
    EXPECT_EQ(mockProg->uniform3fCalls, 1);
    EXPECT_EQ(mockProg->uniform4fCalls, 1);

    int li = glGetUniformLocation(prog, "i");
    glUniform1i(li, 7);
    glUniform2i(li, 7, 8);
    EXPECT_EQ(mockProg->uniform1iCalls, 1);
    EXPECT_EQ(mockProg->uniform2iCalls, 1);
    EXPECT_EQ(mockProg->lastI0, 7);

    float arr[3] = {1.0f, 2.0f, 3.0f};
    glUniform1fv(lf, 3, arr);
    EXPECT_EQ(mockProg->uniform1fvCalls, 1);
    EXPECT_EQ(mockProg->lastUniformCount, 3);

    float mat[16] = {0};
    mat[0] = 1.0f;
    glUniformMatrix4fv(lf, 1, false, mat);
    EXPECT_EQ(mockProg->uniformMatrix4fvCalls, 1);
    EXPECT_EQ(mockProg->lastUniformCount, 1);
    EXPECT_EQ(mockProg->lastTranspose, false);

    setCurrentContext(nullptr);
}
