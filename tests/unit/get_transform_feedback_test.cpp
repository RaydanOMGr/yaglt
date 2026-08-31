// Transform-feedback object state queries (SPEC §22.4 glGetTransformFeedbackiv /
// glGetTransformFeedbacki_v / glGetTransformFeedbacki64_v). These read
// frontend-owned state: the per-object capture flags (ACTIVE / PAUSED) and the
// indexed buffer bindings (BINDING / START / SIZE). Capture state belongs to the
// transform-feedback object, so a paused object that has been unbound still
// reports ACTIVE.
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

} // namespace

TEST_CASE("get_transform_feedback_reports_default_object_capture_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint active = -1;
    GLint paused = -1;
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_PAUSED, &paused);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(active, 0);
    EXPECT_EQ(paused, 0);

    glBeginTransformFeedback(0x0000 /*GL_POINTS*/);
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_PAUSED, &paused);
    EXPECT_EQ(active, 1);
    EXPECT_EQ(paused, 0);

    glPauseTransformFeedback();
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_PAUSED, &paused);
    EXPECT_EQ(active, 1);
    EXPECT_EQ(paused, 1);

    glResumeTransformFeedback();
    glEndTransformFeedback();
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_PAUSED, &paused);
    EXPECT_EQ(active, 0);
    EXPECT_EQ(paused, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("get_transform_feedback_capture_state_is_per_object") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint xfb = glGenTransformFeedback();
    glBindTransformFeedback(xfb);
    glBeginTransformFeedback(0x0000 /*GL_POINTS*/);
    glPauseTransformFeedback();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // A paused object may be unbound (SPEC §13.3.1) and keeps its capture state.
    glBindTransformFeedback(0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint namedActive = -1, namedPaused = -1, defaultActive = -1;
    glGetTransformFeedbackiv(xfb, GL_TRANSFORM_FEEDBACK_ACTIVE, &namedActive);
    glGetTransformFeedbackiv(xfb, GL_TRANSFORM_FEEDBACK_PAUSED, &namedPaused);
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, &defaultActive);
    EXPECT_EQ(namedActive, 1);
    EXPECT_EQ(namedPaused, 1);
    EXPECT_EQ(defaultActive, 0); // the default object was never begun
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("bind_transform_feedback_refused_while_capturing_unpaused") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint xfb = glGenTransformFeedback();
    glBeginTransformFeedback(0x0000 /*GL_POINTS*/); // capture on the default object
    glBindTransformFeedback(xfb);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Pausing releases the binding restriction.
    glPauseTransformFeedback();
    glBindTransformFeedback(xfb);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("get_transform_feedback_indexed_reads_buffer_bindings") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint xfb = glGenTransformFeedback();
    GLuint b = makeBuffer();
    glTransformFeedbackBufferRange(xfb, 2, b, 16, 32);
    glTransformFeedbackBufferBase(xfb, 1, b);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint binding = 0;
    glGetTransformFeedbacki_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 2, &binding);
    EXPECT_EQ(binding, static_cast<GLint>(b));

    GLint64 start = 0, size = 0;
    glGetTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_START, 2, &start);
    glGetTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, 2, &size);
    EXPECT_EQ(start, static_cast<GLint64>(16));
    EXPECT_EQ(size, static_cast<GLint64>(32));

    // glTransformFeedbackBufferBase resets the sub-range to the whole buffer.
    glGetTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_START, 1, &start);
    glGetTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, 1, &size);
    EXPECT_EQ(start, static_cast<GLint64>(0));
    EXPECT_EQ(size, static_cast<GLint64>(0));

    // An untouched binding point reads back unbound.
    glGetTransformFeedbacki_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 3, &binding);
    EXPECT_EQ(binding, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // The default object has its own binding set.
    glGetTransformFeedbacki_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 2, &binding);
    EXPECT_EQ(binding, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("get_transform_feedback_validates_object_pname_and_index") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint param = 0;
    GLint64 param64 = 0;

    // Unknown (never generated) object name.
    glGetTransformFeedbackiv(77, GL_TRANSFORM_FEEDBACK_ACTIVE, &param);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glGetTransformFeedbacki_v(77, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &param);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glGetTransformFeedbacki64_v(77, GL_TRANSFORM_FEEDBACK_BUFFER_START, 0, &param64);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Each command accepts only its own pname set.
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, &param);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetTransformFeedbacki_v(0, GL_TRANSFORM_FEEDBACK_ACTIVE, 0, &param);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetTransformFeedbacki64_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &param64);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Out-of-range binding index (only the indexed forms take one).
    glGetTransformFeedbacki_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 64, &param);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetTransformFeedbacki64_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, 64, &param64);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Null destination.
    glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetTransformFeedbacki_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetTransformFeedbacki64_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_START, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
