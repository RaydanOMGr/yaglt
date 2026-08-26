#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>

using namespace glcompat;

TEST_CASE("transform_feedback_gen_bind_delete_records_and_validates") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // TransformFeedback is Native in the mock profile.
    GLuint tf = glGenTransformFeedback();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(tf, 0u);

    // A second one, plus ungenerated-name rejection on bind.
    glBindTransformFeedback(tf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ctx.boundTransformFeedback(), tf);

    glBindTransformFeedback(9999);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Bind 0 unbinds (back to default).
    glBindTransformFeedback(0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ctx.boundTransformFeedback(), 0u);

    glDeleteTransformFeedback(tf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_begin_end_pause_resume_forwards_to_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tf = glGenTransformFeedback();
    glBindTransformFeedback(tf);
    MockTransformFeedback* mt =
        static_cast<MockTransformFeedback*>(ctx.getTransformFeedback(tf)->backend.get());
    EXPECT_NE(mt, nullptr);

    glBeginTransformFeedback(0x0004 /*GL_TRIANGLES*/);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(mt->beginCalls, 1);
    EXPECT_EQ(mt->lastBeginMode, static_cast<uint32_t>(0x0004));

    // Begin while active is an error.
    glBeginTransformFeedback(0x0004 /*GL_TRIANGLES*/);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glPauseTransformFeedback();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(mt->pauseCalls, 1);

    // Pause while already paused is an error.
    glPauseTransformFeedback();
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glResumeTransformFeedback();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(mt->resumeCalls, 1);

    // Resume while not paused is an error.
    glResumeTransformFeedback();
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glEndTransformFeedback();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(mt->endCalls, 1);

    // End while not active is an error.
    glEndTransformFeedback();
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("transform_feedback_unsupported_reported_honestly") {
    MockBackend backend;
    // Force TransformFeedback unsupported via the capability table.
    backend.setCapability(Feature::TransformFeedback, FeatureSupport::Unsupported);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint tf = glGenTransformFeedback();
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(tf, 0u);

    glBeginTransformFeedback(0x0004 /*GL_TRIANGLES*/);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
