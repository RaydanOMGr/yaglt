#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace glcompat;

namespace {

// Builds an unlinked program with one attached (compiled) vertex shader.
GLuint makeUnlinkedProgram(Context& ctx) {
    (void)ctx;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    return prog;
}

// Builds a linked program behind the public API.
GLuint makeLinkedProgram(Context& ctx) {
    (void)ctx;
    GLuint prog = makeUnlinkedProgram(ctx);
    glLinkProgram(prog);
    return prog;
}

} // namespace

TEST_CASE("transform_feedback_varyings_unknown_program_is_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    const char* names[] = {"gl_Position"};
    glTransformFeedbackVaryings(9999, 1, names, GL_INTERLEAVED_ATTRIBS);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_varyings_negative_count_is_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeUnlinkedProgram(ctx);
    const char* names[] = {"gl_Position"};
    glTransformFeedbackVaryings(prog, -1, names, GL_INTERLEAVED_ATTRIBS);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_varyings_bad_buffer_mode_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeUnlinkedProgram(ctx);
    const char* names[] = {"gl_Position"};
    glTransformFeedbackVaryings(prog, 1, names, 0xDEAD);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_varyings_after_link_is_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    const char* names[] = {"gl_Position"};
    glTransformFeedbackVaryings(prog, 1, names, GL_INTERLEAVED_ATTRIBS);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_varyings_records_on_program_object") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeUnlinkedProgram(ctx);
    const char* names[] = {"v_position", "v_normal"};
    glTransformFeedbackVaryings(prog, 2, names, GL_SEPARATE_ATTRIBS);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    ProgramObject* p = ctx.getProgram(prog);
    EXPECT_NE(p, nullptr);
    if (p == nullptr) return;
    EXPECT_EQ(p->tfVaryings.size(), size_t(2));
    if (p->tfVaryings.size() == 2) {
        EXPECT_EQ(p->tfVaryings[0], std::string("v_position"));
        EXPECT_EQ(p->tfVaryings[1], std::string("v_normal"));
    }
    EXPECT_EQ(p->tfBufferMode, static_cast<uint32_t>(GL_SEPARATE_ATTRIBS));

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_varyings_applied_before_link_to_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Request capture BEFORE link, then link: the backend program records the
    // varying names + buffer mode (SPEC §13.3.1).
    GLuint prog = makeUnlinkedProgram(ctx);
    const char* names[] = {"v_position", "v_normal"};
    glTransformFeedbackVaryings(prog, 2, names, GL_SEPARATE_ATTRIBS);
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    MockProgram* mp = static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());
    EXPECT_NE(mp, nullptr);
    if (mp == nullptr) return;
    EXPECT_EQ(mp->tfRequestedVaryings.size(), size_t(2));
    if (mp->tfRequestedVaryings.size() == 2) {
        EXPECT_EQ(mp->tfRequestedVaryings[0], std::string("v_position"));
        EXPECT_EQ(mp->tfRequestedVaryings[1], std::string("v_normal"));
    }
    EXPECT_EQ(mp->tfRequestedBufferMode, static_cast<uint32_t>(GL_SEPARATE_ATTRIBS));

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_varyings_public_dispatch") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeUnlinkedProgram(ctx);
    const char* names[] = {"gl_Position"};
    glTransformFeedbackVaryings(prog, 1, names, GL_INTERLEAVED_ATTRIBS);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    ProgramObject* p = ctx.getProgram(prog);
    EXPECT_NE(p, nullptr);
    if (p == nullptr) return;
    EXPECT_EQ(p->tfVaryings.size(), size_t(1));
    EXPECT_EQ(p->tfBufferMode, static_cast<uint32_t>(GL_INTERLEAVED_ATTRIBS));

    setCurrentContext(nullptr);
}
