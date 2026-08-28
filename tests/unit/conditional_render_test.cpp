#include "test_framework.hpp"

#include "glcompat/core/capabilities.hpp"
#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

// A query of an allowed type can gate a conditional-render region; the backend
// sink records the open/close with the predicate query id and mode (SPEC §10.11).
TEST_CASE("conditional_render_opens_and_closes_region") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    EXPECT_NE(q, 0u);
    glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
    glEndQuery(GL_ANY_SAMPLES_PASSED);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glBeginConditionalRender(q, GL_QUERY_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 1);
    EXPECT_EQ(backend.lastConditionalRenderId, q);
    EXPECT_EQ(backend.lastConditionalRenderMode, GL_QUERY_WAIT);
    EXPECT_TRUE(ctx.conditionalRenderActive());

    glEndConditionalRender();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.endConditionalRenderCalls, 1);
    EXPECT_FALSE(ctx.conditionalRenderActive());

    setCurrentContext(nullptr);
}

// Every predicate mode is accepted, including the *_INVERTED variants added in
// 4.6 (ARB_conditional_render_inverted).
TEST_CASE("conditional_render_accepts_all_predicate_modes") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQuery(GL_PRIMITIVES_GENERATED, q);
    glEndQuery(GL_PRIMITIVES_GENERATED);

    const GLenum modes[] = {
        GL_QUERY_WAIT, GL_QUERY_NO_WAIT, GL_QUERY_BY_REGION_WAIT,
        GL_QUERY_BY_REGION_NO_WAIT, GL_QUERY_WAIT_INVERTED,
        GL_QUERY_NO_WAIT_INVERTED, GL_QUERY_BY_REGION_WAIT_INVERTED,
        GL_QUERY_BY_REGION_NO_WAIT_INVERTED};
    for (GLenum m : modes) {
        glBeginConditionalRender(q, m);
        EXPECT_EQ(ctx.getError(), GLError::NoError);
        EXPECT_EQ(backend.lastConditionalRenderMode, m);
        glEndConditionalRender();
    }
    EXPECT_EQ(backend.beginConditionalRenderCalls, 8);

    setCurrentContext(nullptr);
}

// Capability-gated: when the backend reports ConditionalRendering unsupported
// the region cannot be opened (honest, no fake support).
TEST_CASE("conditional_render_reports_unsupported_honestly") {
    MockBackend backend;
    backend.setCapability(Feature::ConditionalRendering, FeatureSupport::Unsupported);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
    glEndQuery(GL_ANY_SAMPLES_PASSED);

    glBeginConditionalRender(q, GL_QUERY_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 0);

    setCurrentContext(nullptr);
}

// Validation: a region cannot be opened twice, a non-query id is rejected, an
// active query cannot predicate a region, and an invalid mode is rejected.
TEST_CASE("conditional_render_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
    glEndQuery(GL_ANY_SAMPLES_PASSED);

    // Already-active region -> INVALID_OPERATION.
    glBeginConditionalRender(q, GL_QUERY_WAIT);
    glBeginConditionalRender(q, GL_QUERY_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 1);
    glEndConditionalRender();

    // Non-query id -> INVALID_OPERATION.
    glBeginConditionalRender(999u, GL_QUERY_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 1);

    // Query still active -> INVALID_OPERATION.
    glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
    glBeginConditionalRender(q, GL_QUERY_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    glEndQuery(GL_ANY_SAMPLES_PASSED);

    // Invalid mode -> INVALID_ENUM.
    glBeginConditionalRender(q, 0xDEAD);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 1);

    // End without an open region -> INVALID_OPERATION.
    glEndConditionalRender();
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    setCurrentContext(nullptr);
}

// The predicated query must be of an allowed type (samples-passed /
// any-samples-passed / primitives-generated); other query types are rejected.
TEST_CASE("conditional_render_rejects_disallowed_query_type") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQuery(GL_TIME_ELAPSED, q);
    glEndQuery(GL_TIME_ELAPSED);

    glBeginConditionalRender(q, GL_QUERY_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 0);

    setCurrentContext(nullptr);
}

// Public gl* entry points route through the current context.
TEST_CASE("conditional_render_public_dispatch") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
    glEndQuery(GL_ANY_SAMPLES_PASSED);

    glBeginConditionalRender(q, GL_QUERY_NO_WAIT);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend.beginConditionalRenderCalls, 1);
    glEndConditionalRender();
    EXPECT_EQ(backend.endConditionalRenderCalls, 1);

    (void)GL_TRIANGLES;
    setCurrentContext(nullptr);
}
