#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

TEST_CASE("clamp_color_invalid_target_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glClampColor(GL_TEXTURE0, GL_TRUE); // wrong target
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    ctx.flushState();
    EXPECT_EQ(backend.clampColorCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("clamp_color_invalid_mode_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glClampColor(GL_CLAMP_READ_COLOR, GL_INVALID_ENUM);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    ctx.flushState();
    EXPECT_EQ(backend.clampColorCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("clamp_color_pushed_and_queried") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default is FIXED_ONLY and already applied, so an initial flush pushes nothing.
    ctx.flushState();
    EXPECT_EQ(backend.clampColorCalls, 0);

    // GL_TRUE -> one push.
    glClampColor(GL_CLAMP_READ_COLOR, GL_TRUE);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ctx.flushState();
    EXPECT_EQ(backend.clampColorCalls, 1);
    EXPECT_EQ(backend.lastClampColorTarget,
              static_cast<uint32_t>(GL_CLAMP_READ_COLOR));
    EXPECT_EQ(backend.lastClampColorMode, static_cast<uint32_t>(GL_TRUE));

    // Re-flush with the same mode is skipped (SPEC §10: no redundant native call).
    ctx.flushState();
    EXPECT_EQ(backend.clampColorCalls, 1);

    // glGetIntegerv reads the tracked value.
    GLint i = 0;
    glGetIntegerv(GL_CLAMP_READ_COLOR, &i);
    EXPECT_EQ(i, static_cast<GLint>(GL_TRUE));

    // GL_FIXED_ONLY -> another push, and a re-flush is skipped.
    glClampColor(GL_CLAMP_READ_COLOR, GL_FIXED_ONLY);
    ctx.flushState();
    EXPECT_EQ(backend.clampColorCalls, 2);
    glGetIntegerv(GL_CLAMP_READ_COLOR, &i);
    EXPECT_EQ(i, static_cast<GLint>(GL_FIXED_ONLY));

    setCurrentContext(nullptr);
}
