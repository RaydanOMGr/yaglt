#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("polygon_offset_clamp_records_and_pushes_clamp") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default (factor=0, units=0, clamp=0) matches initial state: no push.
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 0);

    glPolygonOffsetClamp(1.0f, 2.0f, 3.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 1);
    EXPECT_EQ(backend.lastPolygonOffsetFactor, 1.0f);
    EXPECT_EQ(backend.lastPolygonOffsetUnits, 2.0f);
    EXPECT_EQ(backend.lastPolygonOffsetClamp, 3.0f);

    // Identical re-flush: no further push (SPEC §10).
    glPolygonOffsetClamp(1.0f, 2.0f, 3.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 1);

    // Changing only the clamp pushes once more.
    glPolygonOffsetClamp(1.0f, 2.0f, -4.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 2);
    EXPECT_EQ(backend.lastPolygonOffsetClamp, -4.0f);

    setCurrentContext(nullptr);
}

TEST_CASE("polygon_offset_legacy_leaves_clamp_unchanged") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Set a non-default clamp first.
    glPolygonOffsetClamp(1.0f, 2.0f, 5.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 1);
    EXPECT_EQ(backend.lastPolygonOffsetClamp, 5.0f);

    // Legacy glPolygonOffset with the same factor/units does not change state
    // (clamp stays 5.0), so nothing is re-pushed.
    glPolygonOffset(1.0f, 2.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 1);
    EXPECT_EQ(backend.lastPolygonOffsetClamp, 5.0f);

    // Legacy call that does change factor/units pushes once more (clamp retained).
    glPolygonOffset(0.0f, 0.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 2);
    EXPECT_EQ(backend.lastPolygonOffsetClamp, 5.0f);

    setCurrentContext(nullptr);
}
