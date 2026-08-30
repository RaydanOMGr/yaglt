#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

// glDrawArraysInstancedBaseInstance (SPEC §10, ARB_base_instance) records the
// draw and forwards first/count/primcount/baseinstance to the backend.
TEST_CASE("draw_arrays_instanced_base_instance_forwards_base_instance") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(4);
    glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 2, 12, 7, 5u);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawArraysInstancedBaseInstanceCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawFirst, 2);
    EXPECT_EQ(backend.lastDrawCount, 12);
    EXPECT_EQ(backend.lastDrawPrimcount, 7);
    EXPECT_EQ(backend.lastDrawBaseInstance, 5u);

    setCurrentContext(nullptr);
}

// glDrawElementsInstancedBaseInstance forwards mode/count/type/indices/baseinstance.
TEST_CASE("draw_elements_instanced_base_instance_forwards_base_instance") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                                         reinterpret_cast<const GLvoid*>(
                                             static_cast<intptr_t>(24)),
                                         3, 9u);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawElementsInstancedBaseInstanceCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawCount, 6);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawIndices, 24);
    EXPECT_EQ(backend.lastDrawPrimcount, 3);
    EXPECT_EQ(backend.lastDrawBaseInstance, 9u);

    setCurrentContext(nullptr);
}

// glDrawElementsInstancedBaseVertexBaseInstance forwards basevertex AND baseinstance.
TEST_CASE("draw_elements_instanced_base_vertex_base_instance_forwards_both") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(1);
    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                                                  reinterpret_cast<const GLvoid*>(
                                                      static_cast<intptr_t>(8)),
                                                  3, 11, 2u);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.drawElementsInstancedBaseVertexBaseInstanceCalls, 1);
    EXPECT_EQ(backend.lastDrawCount, 6);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastDrawIndices, 8);
    EXPECT_EQ(backend.lastDrawPrimcount, 3);
    EXPECT_EQ(backend.lastDrawBaseInstance, 2u);

    setCurrentContext(nullptr);
}

// Like every draw, the base-instance variants require an active program.
TEST_CASE("draw_arrays_instanced_base_instance_requires_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(0);
    glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0u);
    EXPECT_EQ(backend.drawArraysInstancedBaseInstanceCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// baseinstance = 0 is a legal specialization and must still record the draw.
TEST_CASE("draw_elements_instanced_base_instance_zero_is_valid") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glUseProgram(2);
    glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 4, GL_UNSIGNED_INT,
                                        reinterpret_cast<const GLvoid*>(
                                            static_cast<intptr_t>(0)),
                                        2, 0u);
    EXPECT_EQ(backend.drawElementsInstancedBaseInstanceCalls, 1);
    EXPECT_EQ(backend.lastDrawBaseInstance, 0u);

    setCurrentContext(nullptr);
}
