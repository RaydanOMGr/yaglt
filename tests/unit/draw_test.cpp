#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

TEST_CASE("draw_arrays_flushes_state_then_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glEnable(GL_BLEND);
    glUseProgram(7);

    // Draw with no program -> INVALID_OPERATION, no native draw.
    glUseProgram(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.drawArraysCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Bind a program; draw flushes pending state and records the draw.
    glUseProgram(7);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.enableCalls, 1); // flushed before draw
    EXPECT_EQ(backend.useProgramCalls, 1); // flushed (program) before draw
    EXPECT_EQ(backend.drawArraysCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawFirst, 0);
    EXPECT_EQ(backend.lastDrawCount, 3);

    setCurrentContext(nullptr);
}

TEST_CASE("draw_elements_records_mode_count_type_indices") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                   reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(24)));
    EXPECT_EQ(backend.drawElementsCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawCount, 6);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawIndices, 24);

    setCurrentContext(nullptr);
}

TEST_CASE("draw_instanced_requires_capability_and_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Mock marks InstancedRendering Native, so capability is satisfied.
    EXPECT_TRUE(backend.capabilities().isSupported(Feature::InstancedRendering));

    // No program: INVALID_OPERATION even though instancing is supported.
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 4);
    EXPECT_EQ(backend.drawArraysInstancedCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    glUseProgram(2);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 4);
    EXPECT_EQ(backend.drawArraysInstancedCalls, 1);
    EXPECT_EQ(backend.lastDrawPrimcount, 4);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr, 2);
    EXPECT_EQ(backend.drawElementsInstancedCalls, 1);
    EXPECT_EQ(backend.lastDrawPrimcount, 2);

    setCurrentContext(nullptr);
}

TEST_CASE("draw_without_current_context_is_safe") {
    setCurrentContext(nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 3); // must not crash
    glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_INT, nullptr);
}
