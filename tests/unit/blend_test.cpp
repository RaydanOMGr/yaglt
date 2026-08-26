#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
// Blend constants now live in glcompat:: (gl_types.hpp) and are pulled in via
// `using namespace glcompat;` above.
} // namespace

TEST_CASE("blend_func_separate_sets_rgb_and_alpha_independently") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR,
                        GL_ONE_MINUS_DST_COLOR);
    ctx.flushState();

    EXPECT_EQ(backend.blendFuncCalls, 1);
    EXPECT_EQ(backend.lastSrcRGB, GL_SRC_ALPHA);
    EXPECT_EQ(backend.lastDstRGB, GL_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(backend.lastSrcAlpha, GL_DST_COLOR);
    EXPECT_EQ(backend.lastDstAlpha, GL_ONE_MINUS_DST_COLOR);

    // Identical re-flush is skipped.
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR,
                        GL_ONE_MINUS_DST_COLOR);
    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 1);

    // Changing only alpha factors re-pushes (redundant func push avoided? no:
    // blend func/eq is one category, so it re-pushes, but only once).
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 2);
    EXPECT_EQ(backend.lastSrcAlpha, GL_ONE);
    EXPECT_EQ(backend.lastDstAlpha, GL_ZERO);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_func_maps_rgb_and_alpha_to_same_pair") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx.flushState();

    EXPECT_EQ(backend.blendFuncCalls, 1);
    EXPECT_EQ(backend.lastSrcRGB, GL_SRC_ALPHA);
    EXPECT_EQ(backend.lastSrcAlpha, GL_SRC_ALPHA);
    EXPECT_EQ(backend.lastDstRGB, GL_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(backend.lastDstAlpha, GL_ONE_MINUS_SRC_ALPHA);

    // glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA) must be a no-op vs the
    // equivalent separate call (no redundant push).
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA,
                        GL_ONE_MINUS_SRC_ALPHA);
    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_equation_separate_sets_rgb_and_alpha_equations") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
    ctx.flushState();

    EXPECT_EQ(backend.blendEquationCalls, 1);
    EXPECT_EQ(backend.lastEqRGB, GL_FUNC_REVERSE_SUBTRACT);
    EXPECT_EQ(backend.lastEqAlpha, GL_FUNC_ADD);

    // glBlendEquation sets both equations equal.
    glBlendEquation(GL_FUNC_ADD);
    ctx.flushState();
    EXPECT_EQ(backend.blendEquationCalls, 2);
    EXPECT_EQ(backend.lastEqRGB, GL_FUNC_ADD);
    EXPECT_EQ(backend.lastEqAlpha, GL_FUNC_ADD);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_color_pushed_only_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendColor(0.25f, 0.5f, 0.75f, 1.0f);
    ctx.flushState();
    EXPECT_EQ(backend.blendColorCalls, 1);
    EXPECT_EQ(backend.lastBlendR, 0.25f);
    EXPECT_EQ(backend.lastBlendG, 0.5f);
    EXPECT_EQ(backend.lastBlendB, 0.75f);
    EXPECT_EQ(backend.lastBlendA, 1.0f);

    // Same color: no re-push.
    glBlendColor(0.25f, 0.5f, 0.75f, 1.0f);
    ctx.flushState();
    EXPECT_EQ(backend.blendColorCalls, 1);

    // Different color: re-push.
    glBlendColor(1.0f, 0.0f, 0.0f, 0.0f);
    ctx.flushState();
    EXPECT_EQ(backend.blendColorCalls, 2);
    EXPECT_EQ(backend.lastBlendR, 1.0f);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_color_pushed_independently_of_blend_func") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx.flushState();
    EXPECT_EQ(backend.blendColorCalls, 0);
    EXPECT_EQ(backend.blendFuncCalls, 1);

    // Changing only the blend color must not re-push the func/eq category.
    glBlendColor(0.1f, 0.2f, 0.3f, 0.4f);
    ctx.flushState();
    EXPECT_EQ(backend.blendColorCalls, 1);
    EXPECT_EQ(backend.blendFuncCalls, 1);

    setCurrentContext(nullptr);
}
