// Non-square matrix uniforms (SPEC §7.6 Uniform/ProgramUniformMatrix
// {2x3,3x2,2x4,4x2,3x4,4x3}{fd}v). The first number in a command name is the
// column count and the second the row count, so a NxM matrix carries N*M
// components per matrix. These tests pin down that the frontend forwards the
// right shape (component count), the transpose flag, the "no active program"
// error, the silent no-op cases, and that the explicit-program spellings reach
// the named program without glUseProgram.
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
    return static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());
}
} // namespace

TEST_CASE("uniform_matrix_nonsquare_float_variants_record_shape") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    MockProgram* mp = mockProgOf(ctx, prog);

    // Payload large enough for the widest shape (4x3 / 3x4 = 12 components).
    float m[12];
    for (int i = 0; i < 12; ++i) m[i] = static_cast<float>(i + 1);

    struct Shape {
        const char* name;
        void (*call)(GLint, GLsizei, GLboolean, const GLfloat*);
        int components;
        int* counter;
    };
    const Shape shapes[] = {
        {"m2x3", &glUniformMatrix2x3fv, 6, &mp->uniformMatrix2x3fvCalls},
        {"m3x2", &glUniformMatrix3x2fv, 6, &mp->uniformMatrix3x2fvCalls},
        {"m2x4", &glUniformMatrix2x4fv, 8, &mp->uniformMatrix2x4fvCalls},
        {"m4x2", &glUniformMatrix4x2fv, 8, &mp->uniformMatrix4x2fvCalls},
        {"m3x4", &glUniformMatrix3x4fv, 12, &mp->uniformMatrix3x4fvCalls},
        {"m4x3", &glUniformMatrix4x3fv, 12, &mp->uniformMatrix4x3fvCalls},
    };

    for (const Shape& s : shapes) {
        int loc = glGetUniformLocation(prog, s.name);
        EXPECT_TRUE(loc >= 0);
        s.call(loc, 1, GL_TRUE, m);
        EXPECT_EQ(*s.counter, 1);
        EXPECT_EQ(mp->lastUniformLoc, loc);
        EXPECT_EQ(mp->lastUniformCount, 1);
        EXPECT_EQ(mp->lastTranspose, true);

        // The mock mirrors count * components values, so the readback tail proves
        // the frontend handed the backend the right matrix shape.
        float out[16] = {0};
        glGetUniformfv(prog, loc, out);
        EXPECT_EQ(out[s.components - 1], static_cast<float>(s.components));
        EXPECT_EQ(out[s.components], 0.0f);
    }

    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    setCurrentContext(nullptr);
}

TEST_CASE("uniform_matrix_nonsquare_double_variants_record_shape") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glUseProgram(prog);
    MockProgram* mp = mockProgOf(ctx, prog);

    double m[24];
    for (int i = 0; i < 24; ++i) m[i] = static_cast<double>(i + 1);

    struct Shape {
        const char* name;
        void (*call)(GLint, GLsizei, GLboolean, const GLdouble*);
        int components;
        int* counter;
    };
    const Shape shapes[] = {
        {"d2x3", &glUniformMatrix2x3dv, 6, &mp->uniformMatrix2x3dvCalls},
        {"d3x2", &glUniformMatrix3x2dv, 6, &mp->uniformMatrix3x2dvCalls},
        {"d2x4", &glUniformMatrix2x4dv, 8, &mp->uniformMatrix2x4dvCalls},
        {"d4x2", &glUniformMatrix4x2dv, 8, &mp->uniformMatrix4x2dvCalls},
        {"d3x4", &glUniformMatrix3x4dv, 12, &mp->uniformMatrix3x4dvCalls},
        {"d4x3", &glUniformMatrix4x3dv, 12, &mp->uniformMatrix4x3dvCalls},
    };

    for (const Shape& s : shapes) {
        int loc = glGetUniformLocation(prog, s.name);
        // Two matrices at once: the mock must mirror 2 * components values.
        s.call(loc, 2, GL_FALSE, m);
        EXPECT_EQ(*s.counter, 1);
        EXPECT_EQ(mp->lastUniformCount, 2);
        EXPECT_EQ(mp->lastTranspose, false);

        double out[32] = {0};
        glGetUniformdv(prog, loc, out);
        const int last = 2 * s.components - 1;
        EXPECT_TRUE(std::abs(out[last] - static_cast<double>(last + 1)) < 1e-9);
        EXPECT_TRUE(std::abs(out[last + 1]) < 1e-9);
    }

    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    setCurrentContext(nullptr);
}

TEST_CASE("uniform_matrix_nonsquare_requires_active_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    float m[12] = {0};
    double d[12] = {0};

    // No program in use: GL_INVALID_OPERATION per SPEC §7.6.
    glUniformMatrix3x4fv(0, 1, GL_FALSE, m);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glUniformMatrix4x3dv(0, 1, GL_FALSE, d);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // A -1 location, a null pointer, and a non-positive count are silent no-ops
    // (they must not raise an error, matching the square variants).
    glUniformMatrix2x3fv(-1, 1, GL_FALSE, m);
    glUniformMatrix2x4fv(0, 1, GL_FALSE, nullptr);
    glUniformMatrix4x2dv(0, 0, GL_FALSE, d);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_matrix_nonsquare_targets_explicit_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint progA = makeLinkedProgram(ctx);
    GLuint progB = makeLinkedProgram(ctx);
    // Deliberately no glUseProgram: the explicit-program spellings must still
    // reach progA's backend program and leave progB untouched.
    float m[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    double d[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    int loc = glGetUniformLocation(progA, "m");
    glProgramUniformMatrix2x3fv(progA, loc, 1, GL_TRUE, m);
    glProgramUniformMatrix3x2fv(progA, loc, 1, GL_FALSE, m);
    glProgramUniformMatrix2x4fv(progA, loc, 1, GL_FALSE, m);
    glProgramUniformMatrix4x2fv(progA, loc, 1, GL_FALSE, m);
    glProgramUniformMatrix3x4fv(progA, loc, 1, GL_FALSE, m);
    glProgramUniformMatrix4x3fv(progA, loc, 1, GL_FALSE, m);
    glProgramUniformMatrix2x3dv(progA, loc, 1, GL_FALSE, d);
    glProgramUniformMatrix3x2dv(progA, loc, 1, GL_FALSE, d);
    glProgramUniformMatrix2x4dv(progA, loc, 1, GL_FALSE, d);
    glProgramUniformMatrix4x2dv(progA, loc, 1, GL_FALSE, d);
    glProgramUniformMatrix3x4dv(progA, loc, 1, GL_FALSE, d);
    glProgramUniformMatrix4x3dv(progA, loc, 1, GL_TRUE, d);

    MockProgram* a = mockProgOf(ctx, progA);
    EXPECT_EQ(a->uniformMatrix2x3fvCalls, 1);
    EXPECT_EQ(a->uniformMatrix3x2fvCalls, 1);
    EXPECT_EQ(a->uniformMatrix2x4fvCalls, 1);
    EXPECT_EQ(a->uniformMatrix4x2fvCalls, 1);
    EXPECT_EQ(a->uniformMatrix3x4fvCalls, 1);
    EXPECT_EQ(a->uniformMatrix4x3fvCalls, 1);
    EXPECT_EQ(a->uniformMatrix2x3dvCalls, 1);
    EXPECT_EQ(a->uniformMatrix3x2dvCalls, 1);
    EXPECT_EQ(a->uniformMatrix2x4dvCalls, 1);
    EXPECT_EQ(a->uniformMatrix4x2dvCalls, 1);
    EXPECT_EQ(a->uniformMatrix3x4dvCalls, 1);
    EXPECT_EQ(a->uniformMatrix4x3dvCalls, 1);
    EXPECT_EQ(a->lastTranspose, true); // from the final 4x3dv call

    MockProgram* b = mockProgOf(ctx, progB);
    EXPECT_EQ(b->uniformMatrix2x3fvCalls, 0);
    EXPECT_EQ(b->uniformMatrix4x3dvCalls, 0);

    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    setCurrentContext(nullptr);
}

TEST_CASE("program_uniform_matrix_nonsquare_on_unlinked_program_errors") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = ctx.createProgram(); // never linked
    float m[12] = {0};
    glProgramUniformMatrix2x3fv(prog, 0, 1, GL_FALSE, m);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    double d[12] = {0};
    glProgramUniformMatrix4x3dv(prog, 0, 1, GL_FALSE, d);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
