#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glPrimitiveRestartIndex_pushes_only_on_change_and_records_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default restart index is 0; a different value pushes.
    glPrimitiveRestartIndex(0xFFFF);
    glFlushState();
    EXPECT_EQ(backend.primitiveRestartCalls, 1);
    EXPECT_EQ(backend.lastPrimitiveRestartIndex, 0xFFFFu);

    // Same value again must not re-push (SPEC §10).
    glPrimitiveRestartIndex(0xFFFF);
    glFlushState();
    EXPECT_EQ(backend.primitiveRestartCalls, 1);

    // Changed value pushes again.
    glPrimitiveRestartIndex(1);
    glFlushState();
    EXPECT_EQ(backend.primitiveRestartCalls, 2);
    EXPECT_EQ(backend.lastPrimitiveRestartIndex, 1u);

    setCurrentContext(nullptr);
}

TEST_CASE("glPrimitiveRestartIndex_new_context_pushes_nondefault_index") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // A non-default index must be pushed on the first flush of a fresh context.
    glPrimitiveRestartIndex(0x10);
    glFlushState();
    EXPECT_EQ(backend.primitiveRestartCalls, 1);
    EXPECT_EQ(backend.lastPrimitiveRestartIndex, 0x10u);

    setCurrentContext(nullptr);
}

TEST_CASE("glPrimitiveRestart_activates_via_capability_and_query_roundtrips") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Activation is a normal capability, pushed to the backend on the next flush.
    glEnable(GL_PRIMITIVE_RESTART);
    glFlushState();
    EXPECT_EQ(backend.lastEnableCap, GL_PRIMITIVE_RESTART);
    EXPECT_EQ(glIsEnabled(GL_PRIMITIVE_RESTART), GL_TRUE);

    // glGetIntegerv(GL_PRIMITIVE_RESTART_INDEX) returns the tracked index.
    glPrimitiveRestartIndex(0xABCD);
    GLint idx = -1;
    glGetIntegerv(GL_PRIMITIVE_RESTART_INDEX, &idx);
    EXPECT_EQ(static_cast<GLuint>(idx), 0xABCDu);

    setCurrentContext(nullptr);
}
