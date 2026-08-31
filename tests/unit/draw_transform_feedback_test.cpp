#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

// glDrawTransformFeedback (SPEC §13.3.3) draws the captured vertex count of the
// named transform-feedback object; the backend records the resolved count.
TEST_CASE("draw_transform_feedback_uses_captured_count") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName tf = ctx.genTransformFeedback();
    EXPECT_NE(tf, 0u);
    auto* mtf = dynamic_cast<MockTransformFeedback*>(ctx.getTransformFeedback(tf)->backend.get());
    EXPECT_NE(mtf, nullptr);
    mtf->capturedVertexCount = 17;

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawTransformFeedback(GL_TRIANGLES, tf);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawTransformFeedbackCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawTfId, 0u); // mock has no native handle
    EXPECT_EQ(backend.lastDrawCount, 17);

    setCurrentContext(nullptr);
}

// Requires an active program (like every draw).
TEST_CASE("draw_transform_feedback_requires_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName tf = ctx.genTransformFeedback();
    glUseProgram(0);
    glDrawTransformFeedback(GL_TRIANGLES, tf);
    EXPECT_EQ(backend.drawTransformFeedbackCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// A non-existent transform-feedback object is rejected.
TEST_CASE("draw_transform_feedback_unknown_object_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawTransformFeedback(GL_TRIANGLES, 12345u);
    EXPECT_EQ(backend.drawTransformFeedbackCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// Drawing while feedback is active and not paused is a feedback loop.
TEST_CASE("draw_transform_feedback_active_loop_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName tf = ctx.genTransformFeedback();
    auto* mtf = dynamic_cast<MockTransformFeedback*>(ctx.getTransformFeedback(tf)->backend.get());
    mtf->capturedVertexCount = 4;

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    ctx.beginTransformFeedback(GL_TRIANGLES);
    glDrawTransformFeedback(GL_TRIANGLES, tf);
    EXPECT_EQ(backend.drawTransformFeedbackCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Paused capture is fine to draw from.
    ctx.pauseTransformFeedback();
    glDrawTransformFeedback(GL_TRIANGLES, tf);
    EXPECT_EQ(backend.drawTransformFeedbackCalls, 1);
    EXPECT_EQ(backend.lastDrawCount, 4);
    ctx.endTransformFeedback();

    setCurrentContext(nullptr);
}

// glDrawTransformFeedbackInstanced forwards primcount; stream variant forwards the
// stream index and the per-stream captured count.
TEST_CASE("draw_transform_feedback_instanced_and_stream") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName tf = ctx.genTransformFeedback();
    auto* mtf = dynamic_cast<MockTransformFeedback*>(ctx.getTransformFeedback(tf)->backend.get());
    mtf->capturedVertexCount = 9;

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawTransformFeedbackInstanced(GL_TRIANGLES, tf, 5);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawTransformFeedbackInstancedCalls, 1);
    EXPECT_EQ(backend.lastDrawCount, 9);
    EXPECT_EQ(backend.lastDrawPrimcount, 5);

    glDrawTransformFeedbackStream(GL_TRIANGLES, tf, 1u);
    EXPECT_EQ(backend.drawTransformFeedbackStreamCalls, 1);
    EXPECT_EQ(backend.lastDrawStream, 1u);
    EXPECT_EQ(backend.lastDrawCount, 9);

    glDrawTransformFeedbackStreamInstanced(GL_TRIANGLES, tf, 2u, 3);
    EXPECT_EQ(backend.drawTransformFeedbackStreamInstancedCalls, 1);
    EXPECT_EQ(backend.lastDrawStream, 2u);
    EXPECT_EQ(backend.lastDrawCount, 9);
    EXPECT_EQ(backend.lastDrawPrimcount, 3);

    setCurrentContext(nullptr);
}
