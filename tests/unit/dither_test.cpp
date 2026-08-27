#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

// GL_DITHER is enabled by default (SPEC §17.3.7) and is queryable/toggleable
// like any tracked capability.
TEST_CASE("dither_enabled_by_default") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(ctx.isEnabled(GL_DITHER), true);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    GLint v = 0;
    ctx.getIntegerv(GL_DITHER, &v);
    EXPECT_EQ(v, 1);

    setCurrentContext(nullptr);
}

// glDisable/glEnable(GL_DITHER) flip tracked state and push to the backend only
// on change (SPEC §10: no redundant native call).
TEST_CASE("dither_toggle_pushes_to_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default dithering does not push anything at first flush.
    glUseProgram(3);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    int enableAtStart = backend.enableCalls;
    int disableAtStart = backend.disableCalls;

    glDisable(GL_DITHER);
    EXPECT_EQ(ctx.isEnabled(GL_DITHER), false);
    EXPECT_EQ(backend.disableCalls, disableAtStart); // not yet flushed

    glDrawArrays(GL_TRIANGLES, 3, 3);
    EXPECT_EQ(backend.disableCalls, disableAtStart + 1);
    EXPECT_EQ(backend.enableCalls, enableAtStart);

    // A second draw with no change does not push again.
    glDrawArrays(GL_TRIANGLES, 6, 3);
    EXPECT_EQ(backend.disableCalls, disableAtStart + 1);

    // Re-enabling pushes a single enable.
    glEnable(GL_DITHER);
    glDrawArrays(GL_TRIANGLES, 9, 3);
    EXPECT_EQ(backend.enableCalls, enableAtStart + 1);
    EXPECT_EQ(backend.disableCalls, disableAtStart + 1);

    setCurrentContext(nullptr);
}
