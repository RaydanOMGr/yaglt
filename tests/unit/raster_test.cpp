#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

TEST_CASE("raster_scalar_state_pushed_only_when_changed") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Defaults (pointSize=1, lineWidth=1, polygonOffset=0,0) push nothing.
    ctx.flushState();
    EXPECT_EQ(backend.pointSizeCalls, 0);
    EXPECT_EQ(backend.lineWidthCalls, 0);
    EXPECT_EQ(backend.polygonOffsetCalls, 0);

    // Changing point size pushes only the point-size call.
    glPointSize(3.0f);
    ctx.flushState();
    EXPECT_EQ(backend.pointSizeCalls, 1);
    EXPECT_EQ(backend.lastPointSize, 3.0f);
    EXPECT_EQ(backend.lineWidthCalls, 0);
    EXPECT_EQ(backend.polygonOffsetCalls, 0);

    // Re-flush with the same point size is skipped.
    glPointSize(3.0f);
    ctx.flushState();
    EXPECT_EQ(backend.pointSizeCalls, 1);

    // Changing line width pushes only the line-width call.
    glLineWidth(2.5f);
    ctx.flushState();
    EXPECT_EQ(backend.lineWidthCalls, 1);
    EXPECT_EQ(backend.lastLineWidth, 2.5f);
    EXPECT_EQ(backend.pointSizeCalls, 1);

    // Changing polygon offset pushes only the polygon-offset call.
    glPolygonOffset(1.5f, 4.0f);
    ctx.flushState();
    EXPECT_EQ(backend.polygonOffsetCalls, 1);
    EXPECT_EQ(backend.lastPolygonOffsetFactor, 1.5f);
    EXPECT_EQ(backend.lastPolygonOffsetUnits, 4.0f);
    EXPECT_EQ(backend.pointSizeCalls, 1);
    EXPECT_EQ(backend.lineWidthCalls, 1);

    // No redundant pushes when nothing changed.
    ctx.flushState();
    EXPECT_EQ(backend.pointSizeCalls, 1);
    EXPECT_EQ(backend.lineWidthCalls, 1);
    EXPECT_EQ(backend.polygonOffsetCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("raster_scalar_queried_via_glget") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPointSize(3.0f);
    glLineWidth(2.5f);
    glPolygonOffset(2.0f, 4.0f);

    GLint i = 0;
    glGetIntegerv(GL_POINT_SIZE, &i);
    EXPECT_EQ(i, 3);
    glGetIntegerv(GL_LINE_WIDTH, &i);
    EXPECT_EQ(i, 2);
    glGetIntegerv(GL_POLYGON_OFFSET_FACTOR, &i);
    EXPECT_EQ(i, 2);
    glGetIntegerv(GL_POLYGON_OFFSET_UNITS, &i);
    EXPECT_EQ(i, 4);

    GLfloat f = 0.0f;
    glGetFloatv(GL_POINT_SIZE, &f);
    EXPECT_EQ(f, 3.0f);
    glGetFloatv(GL_LINE_WIDTH, &f);
    EXPECT_EQ(f, 2.5f);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &f);
    EXPECT_EQ(f, 2.0f);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &f);
    EXPECT_EQ(f, 4.0f);

    GLdouble d = 0.0;
    glGetDoublev(GL_POINT_SIZE, &d);
    EXPECT_EQ(d, 3.0);
    glGetDoublev(GL_POLYGON_OFFSET_UNITS, &d);
    EXPECT_EQ(d, 4.0);

    // Unknown pname forwarded to mock backend (no-op); no error set.
    GLenum errBefore = glGetError();
    (void)errBefore;
    glGetIntegerv(0xDEAD, &i);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("polygon_offset_fill_is_tracked_capability") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(glIsEnabled(GL_POLYGON_OFFSET_FILL), GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    ctx.flushState();
    EXPECT_EQ(backend.enableCalls, 1);
    EXPECT_EQ(backend.lastEnableCap, GL_POLYGON_OFFSET_FILL);
    EXPECT_EQ(glIsEnabled(GL_POLYGON_OFFSET_FILL), GL_TRUE);

    glDisable(GL_POLYGON_OFFSET_FILL);
    ctx.flushState();
    EXPECT_EQ(backend.disableCalls, 1);
    EXPECT_EQ(backend.lastDisableCap, GL_POLYGON_OFFSET_FILL);
    EXPECT_EQ(glIsEnabled(GL_POLYGON_OFFSET_FILL), GL_FALSE);

    setCurrentContext(nullptr);
}
