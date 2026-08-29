#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "glcompat/state/gl_state.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {

TEST_CASE("blend_funci_buffer0_is_nonindexed_blend_path") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Buffer 0 mirrors glBlendFunc: pushed through the single-buffer sink.
    glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 1);
    EXPECT_EQ(backend.lastSrcRGB, GL_SRC_ALPHA);
    EXPECT_EQ(backend.lastDstRGB, GL_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(backend.blendFunciCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_funci_nonzero_buffer_uses_indexed_path") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFunci(2, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
    ctx.flushState();
    EXPECT_EQ(backend.blendFunciCalls, 1);
    EXPECT_EQ(backend.lastBlendFunciBuf, 2u);
    EXPECT_EQ(backend.lastSrcRGBi, GL_SRC_COLOR);
    EXPECT_EQ(backend.lastDstRGBi, GL_ONE_MINUS_SRC_COLOR);
    EXPECT_EQ(backend.blendFuncCalls, 0); // buffer 0 untouched
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    // Re-flush with unchanged buffer-2 state: no redundant push.
    glBlendFunci(2, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
    ctx.flushState();
    EXPECT_EQ(backend.blendFunciCalls, 1);

    // Changing buffer 2 re-pushes.
    glBlendFunci(2, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR);
    ctx.flushState();
    EXPECT_EQ(backend.blendFunciCalls, 2);
    EXPECT_EQ(backend.lastSrcRGBi, GL_DST_COLOR);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_func_separatei_nonzero_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFuncSeparatei(1, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
                         GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx.flushState();
    EXPECT_EQ(backend.blendFunciCalls, 1);
    EXPECT_EQ(backend.lastBlendFunciBuf, 1u);
    EXPECT_EQ(backend.lastSrcRGBi, GL_SRC_COLOR);
    EXPECT_EQ(backend.lastDstRGBi, GL_ONE_MINUS_SRC_COLOR);
    EXPECT_EQ(backend.lastSrcAlphai, GL_SRC_ALPHA);
    EXPECT_EQ(backend.lastDstAlphai, GL_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("blend_equationi_and_separatei") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendEquationi(0, GL_FUNC_REVERSE_SUBTRACT);
    ctx.flushState();
    EXPECT_EQ(backend.blendEquationCalls, 1);
    EXPECT_EQ(backend.lastEqRGB, GL_FUNC_REVERSE_SUBTRACT);
    EXPECT_EQ(backend.blendEquationiCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glBlendEquationi(3, GL_FUNC_REVERSE_SUBTRACT);
    ctx.flushState();
    EXPECT_EQ(backend.blendEquationiCalls, 1);
    EXPECT_EQ(backend.lastBlendEquationiBuf, 3u);
    EXPECT_EQ(backend.lastEqRGBi, GL_FUNC_REVERSE_SUBTRACT);
    EXPECT_EQ(backend.blendEquationCalls, 1); // buffer 0 still only pushed once

    glBlendEquationSeparatei(1, GL_FUNC_ADD, GL_FUNC_REVERSE_SUBTRACT);
    ctx.flushState();
    EXPECT_EQ(backend.blendEquationiCalls, 2);
    EXPECT_EQ(backend.lastBlendEquationiBuf, 1u);
    EXPECT_EQ(backend.lastEqRGBi, GL_FUNC_ADD);
    EXPECT_EQ(backend.lastEqAlphai, GL_FUNC_REVERSE_SUBTRACT);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("indexed_blend_validates_buffer_range") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFunci(GLStateTracker::kMaxDrawBuffers, GL_ONE, GL_ZERO);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.flushState();
    EXPECT_EQ(backend.blendFunciCalls, 0);

    glBlendEquationi(GLStateTracker::kMaxDrawBuffers, GL_FUNC_ADD);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Last valid index is accepted.
    glBlendFunci(GLStateTracker::kMaxDrawBuffers - 1, GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA);
    ctx.flushState();
    EXPECT_EQ(backend.blendFunciCalls, 1);
    EXPECT_EQ(backend.lastBlendFunciBuf,
              GLStateTracker::kMaxDrawBuffers - 1);

    setCurrentContext(nullptr);
}

TEST_CASE("indexed_blend_validates_factor_and_equation_enums") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFunci(0, 0xDEAD, GL_ZERO); // invalid src factor
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    glBlendFuncSeparatei(0, GL_ONE, GL_ZERO, GL_DST_ALPHA, 0xBEEF); // bad alpha
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    glBlendEquationi(0, 0xCAFE); // invalid equation
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    glBlendEquationSeparatei(0, GL_FUNC_ADD, 0xCAFE);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    ctx.flushState();
    EXPECT_EQ(backend.blendFuncCalls, 0);
    EXPECT_EQ(backend.blendFunciCalls, 0);
    EXPECT_EQ(backend.blendEquationCalls, 0);
    EXPECT_EQ(backend.blendEquationiCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("indexed_blend_safe_without_context") {
    setCurrentContext(nullptr);
    // Must not crash when no context is current.
    glBlendFunci(0, GL_ONE, GL_ZERO);
    glBlendFuncSeparatei(1, GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationi(0, GL_FUNC_ADD);
    glBlendEquationSeparatei(1, GL_FUNC_ADD, GL_FUNC_ADD);
}

} // namespace
