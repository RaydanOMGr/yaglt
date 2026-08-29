#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

namespace {

std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

} // namespace

// glViewportIndexedf / glViewportIndexedfv (SPEC §10.3.1): indexed viewport
// state, pushed to the backend only when the slot changes (SPEC §10).
TEST_CASE("viewport_indexed_default_is_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t v[4] = {};
    ctx.getIntegerv(GL_VIEWPORT, v);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 0);
    EXPECT_EQ(v[2], 0);
    EXPECT_EQ(v[3], 0);
    int32_t vi[4] = {};
    ctx.getIntegeri_v(GL_VIEWPORT, 5, vi);
    EXPECT_EQ(vi[0], 0);
    EXPECT_EQ(vi[3], 0);
}

TEST_CASE("viewport_indexed_pushes_changed_slot") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setViewportIndexed(3, 1, 2, 3, 4);
    ctx.flushState();
    EXPECT_TRUE(backend->viewportIndexedCalls >= 1);
    EXPECT_EQ(backend->lastViewportIndexed, 3u);
    EXPECT_EQ(backend->lastViewportIndexedX, 1);
    EXPECT_EQ(backend->lastViewportIndexedY, 2);
    EXPECT_EQ(backend->lastViewportIndexedW, 3);
    EXPECT_EQ(backend->lastViewportIndexedH, 4);

    // Re-flush without change must not push again.
    int after = backend->viewportIndexedCalls;
    ctx.flushState();
    EXPECT_EQ(backend->viewportIndexedCalls, after);
}

TEST_CASE("viewport_indexed_reports_via_get") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setViewportIndexed(7, 10, 20, 30, 40);
    int32_t v[4] = {};
    ctx.getIntegeri_v(GL_VIEWPORT, 7, v);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
    EXPECT_EQ(v[3], 40);
    float vf[4] = {};
    ctx.getFloati_v(GL_VIEWPORT, 7, vf);
    EXPECT_EQ(vf[2], 30.0f);
    double vd[4] = {};
    ctx.getDoublei_v(GL_VIEWPORT, 7, vd);
    EXPECT_EQ(vd[3], 40.0);
}

TEST_CASE("viewport_indexedfv_sets_from_array") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLfloat v[4] = {5, 6, 7, 8};
    glViewportIndexedfv(2, v);
    ctx.flushState();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->lastViewportIndexed, 2u);
    EXPECT_EQ(backend->lastViewportIndexedX, 5);
    EXPECT_EQ(backend->lastViewportIndexedH, 8);
    int32_t got[4] = {};
    ctx.getIntegeri_v(GL_VIEWPORT, 2, got);
    EXPECT_EQ(got[0], 5);
    EXPECT_EQ(got[3], 8);
    setCurrentContext(nullptr);
}

TEST_CASE("viewport_indexed_negative_width_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setViewportIndexed(1, 0, 0, -1, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    int32_t v[4] = {-1, -1, -1, -1};
    ctx.getIntegeri_v(GL_VIEWPORT, 1, v);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[2], 0);
}

TEST_CASE("viewport_indexed_out_of_range_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setViewportIndexed(16, 0, 0, 1, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.setViewportIndexed(100, 0, 0, 1, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("viewport_indexed_via_public_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glViewportIndexedf(4, 11.0f, 12.0f, 13.0f, 14.0f);
    ctx.flushState();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->lastViewportIndexed, 4u);
    EXPECT_EQ(backend->lastViewportIndexedX, 11);
    EXPECT_EQ(backend->lastViewportIndexedH, 14);
    setCurrentContext(nullptr);
}

// glScissorIndexed / glScissorIndexedv (SPEC §10.3.1).
TEST_CASE("scissor_indexed_pushes_changed_slot") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setScissorIndexed(2, 1, 2, 3, 4);
    ctx.flushState();
    EXPECT_TRUE(backend->scissorIndexedCalls >= 1);
    EXPECT_EQ(backend->lastScissorIndexed, 2u);
    EXPECT_EQ(backend->lastScissorIndexedX, 1);
    EXPECT_EQ(backend->lastScissorIndexedH, 4);
    int32_t v[4] = {};
    ctx.getIntegeri_v(GL_SCISSOR_BOX, 2, v);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[3], 4);
}

TEST_CASE("scissor_indexedv_sets_from_array") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLint v[4] = {5, 6, 7, 8};
    glScissorIndexedv(3, v);
    ctx.flushState();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->lastScissorIndexed, 3u);
    EXPECT_EQ(backend->lastScissorIndexedW, 7);
    int32_t got[4] = {};
    ctx.getIntegeri_v(GL_SCISSOR_BOX, 3, got);
    EXPECT_EQ(got[1], 6);
    setCurrentContext(nullptr);
}

TEST_CASE("scissor_indexed_negative_height_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setScissorIndexed(1, 0, 0, 4, -2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("scissor_indexed_via_public_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glScissorIndexed(5, 21, 22, 23, 24);
    ctx.flushState();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->lastScissorIndexed, 5u);
    EXPECT_EQ(backend->lastScissorIndexedY, 22);
    setCurrentContext(nullptr);
}

// glViewport (single) sets slot 0 through the single-viewport path.
TEST_CASE("viewport_single_sets_slot_zero") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.setViewport(1, 2, 3, 4);
    ctx.flushState();
    EXPECT_TRUE(backend->viewportCalls >= 1);
    EXPECT_EQ(backend->lastViewportX, 1);
    EXPECT_EQ(backend->lastViewportH, 4);
    int32_t v[4] = {};
    ctx.getIntegerv(GL_VIEWPORT, v);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[3], 4);
}
