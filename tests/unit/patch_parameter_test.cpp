#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("patch_parameter_i_records_and_pushes") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default patch vertices (3) matches initial state: no push.
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 0);
    EXPECT_EQ(backend.lastPatchVertices, 3u);

    glPatchParameteri(GL_PATCH_VERTICES, 12);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 1);
    EXPECT_EQ(backend.lastPatchPname, static_cast<uint32_t>(GL_PATCH_VERTICES));
    EXPECT_EQ(backend.lastPatchVertices, 12u);

    // Identical re-flush: no further push (SPEC §10).
    glPatchParameteri(GL_PATCH_VERTICES, 12);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 1);

    // Changing the count pushes once more.
    glPatchParameteri(GL_PATCH_VERTICES, 8);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 2);
    EXPECT_EQ(backend.lastPatchVertices, 8u);

    // Invalid pname -> GL_INVALID_ENUM, no state push.
    glPatchParameteri(GL_PATCH_DEFAULT_OUTER_LEVEL, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 2);

    // Non-positive vertex count -> GL_INVALID_VALUE, no state push.
    glPatchParameteri(GL_PATCH_VERTICES, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 2);
    EXPECT_EQ(backend.lastPatchVertices, 8u);

    setCurrentContext(nullptr);
}

TEST_CASE("patch_parameter_fv_records_and_pushes") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    float outer[4] = {2.0f, 3.0f, 4.0f, 5.0f};
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outer);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 1);
    EXPECT_EQ(backend.lastPatchPname,
              static_cast<uint32_t>(GL_PATCH_DEFAULT_OUTER_LEVEL));
    EXPECT_EQ(backend.lastPatchOuterLevel[0], 2.0f);
    EXPECT_EQ(backend.lastPatchOuterLevel[3], 5.0f);

    // Identical re-flush: no further push (SPEC §10).
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outer);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 1);

    float inner[2] = {6.0f, 7.0f};
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, inner);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 2);
    EXPECT_EQ(backend.lastPatchPname,
              static_cast<uint32_t>(GL_PATCH_DEFAULT_INNER_LEVEL));
    EXPECT_EQ(backend.lastPatchInnerLevel[0], 6.0f);
    EXPECT_EQ(backend.lastPatchInnerLevel[1], 7.0f);

    // Invalid pname -> GL_INVALID_ENUM, no state push.
    float bad[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glPatchParameterfv(GL_PATCH_VERTICES, bad);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 2);

    // Null values pointer is ignored: no error, no push.
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    ctx.flushState();
    EXPECT_EQ(backend.patchParameterCalls, 2);

    setCurrentContext(nullptr);
}
