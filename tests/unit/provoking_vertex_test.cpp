#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

TEST_CASE("provoking_vertex_invalid_mode_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glProvokingVertex(GL_INVALID_ENUM); // not a valid convention
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    ctx.flushState();
    EXPECT_EQ(backend.provokingVertexCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("provoking_vertex_pushed_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default LAST is already applied, so an initial flush pushes nothing.
    ctx.flushState();
    EXPECT_EQ(backend.provokingVertexCalls, 0);

    // Switch to FIRST -> one push.
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ctx.flushState();
    EXPECT_EQ(backend.provokingVertexCalls, 1);
    EXPECT_EQ(backend.lastProvokingVertexMode,
              static_cast<uint32_t>(GL_FIRST_VERTEX_CONVENTION));

    // Re-flush with the same mode is skipped (SPEC §10: no redundant native call).
    ctx.flushState();
    EXPECT_EQ(backend.provokingVertexCalls, 1);

    // Back to LAST -> another push.
    glProvokingVertex(GL_LAST_VERTEX_CONVENTION);
    ctx.flushState();
    EXPECT_EQ(backend.provokingVertexCalls, 2);
    EXPECT_EQ(backend.lastProvokingVertexMode,
              static_cast<uint32_t>(GL_LAST_VERTEX_CONVENTION));

    setCurrentContext(nullptr);
}

TEST_CASE("provoking_vertex_queried_via_glget") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default convention is LAST.
    GLint i = 0;
    glGetIntegerv(GL_PROVOKING_VERTEX, &i);
    EXPECT_EQ(i, static_cast<GLint>(GL_LAST_VERTEX_CONVENTION));

    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
    glGetIntegerv(GL_PROVOKING_VERTEX, &i);
    EXPECT_EQ(i, static_cast<GLint>(GL_FIRST_VERTEX_CONVENTION));

    setCurrentContext(nullptr);
}
