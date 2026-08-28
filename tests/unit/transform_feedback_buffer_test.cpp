#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {

GLuint makeBuffer() {
    GLuint b = 0;
    glGenBuffers(1, &b);
    glBindBuffer(GL_ARRAY_BUFFER, b);
    glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    return b;
}

GLuint makeUnlinkedProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    return prog;
}

} // namespace

TEST_CASE("transform_feedback_buffer_base_binds_default_object") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint b = makeBuffer();
    glTransformFeedbackBufferBase(0, 0, b);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint bound = 0;
    glGetIntegeri_v(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &bound);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(bound, static_cast<GLint>(b));

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_range_records_offset_and_size") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Bind a named TF object and address it directly via the DSA entry point; the
    // indexed query reads the same (active) object.
    GLuint xfb = glGenTransformFeedback();
    glBindTransformFeedback(xfb);
    GLuint b = makeBuffer();
    glTransformFeedbackBufferRange(xfb, 2, b, 16, 32);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint bound = 0;
    glGetIntegeri_v(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 2, &bound);
    EXPECT_EQ(bound, static_cast<GLint>(b));

    TransformFeedbackObject* tfo = ctx.getTransformFeedback(xfb);
    EXPECT_NE(tfo, nullptr);
    if (tfo != nullptr) {
        EXPECT_EQ(tfo->bufferBindings[2].offset, intptr_t(16));
        EXPECT_EQ(tfo->bufferBindings[2].size, intptr_t(32));
    }

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_base_binds_named_object") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint xfb = glGenTransformFeedback();
    GLuint b = makeBuffer();
    // xfb != 0 picks the named object (does not require it to be bound).
    glTransformFeedbackBufferBase(xfb, 1, b);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    TransformFeedbackObject* tfo = ctx.getTransformFeedback(xfb);
    EXPECT_NE(tfo, nullptr);
    if (tfo != nullptr) {
        EXPECT_EQ(tfo->bufferBindings[1].buffer, b);
        EXPECT_EQ(tfo->bufferBindings[1].offset, intptr_t(0));
        EXPECT_EQ(tfo->bufferBindings[1].size, intptr_t(0));
    }

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_route_via_bind_buffer_base") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Bind a named TF object so the active-object binding path is exercised.
    GLuint xfb = glGenTransformFeedback();
    glBindTransformFeedback(xfb);
    GLuint b = makeBuffer();

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, b);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint bound = 0;
    glGetIntegeri_v(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &bound);
    EXPECT_EQ(bound, static_cast<GLint>(b));

    TransformFeedbackObject* tfo = ctx.getTransformFeedback(xfb);
    EXPECT_NE(tfo, nullptr);
    if (tfo != nullptr) EXPECT_EQ(tfo->bufferBindings[0].buffer, b);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_out_of_range_index_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint b = makeBuffer();
    glTransformFeedbackBufferBase(0, 9999, b);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_ungenerated_buffer_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glTransformFeedbackBufferBase(0, 0, 7777);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_ungenerated_object_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint b = makeBuffer();
    glTransformFeedbackBufferBase(7777, 0, b);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_buffer_program_queries") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeUnlinkedProgram();
    const char* names[] = {"v_position", "v_normal"};
    glTransformFeedbackVaryings(prog, 2, names, GL_SEPARATE_ATTRIBS);
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    GLint mode = 0;
    glGetProgramiv(prog, GL_TRANSFORM_FEEDBACK_BUFFER_MODE, &mode);
    EXPECT_EQ(mode, static_cast<GLint>(GL_SEPARATE_ATTRIBS));

    GLint varyings = 0;
    glGetProgramiv(prog, GL_TRANSFORM_FEEDBACK_VARYINGS, &varyings);
    EXPECT_EQ(varyings, GLint(2));

    setCurrentContext(nullptr);
}
