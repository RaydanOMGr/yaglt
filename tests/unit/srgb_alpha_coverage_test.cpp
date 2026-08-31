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

// sRGB drawbuffer decoding (SPEC §15.1.1) and alpha-to-coverage (SPEC §15.3.1)
// are tracked capabilities, off by default, queryable/toggleable.
TEST_CASE("srgb_and_alpha_to_coverage_default_off") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(ctx.isEnabled(GL_FRAMEBUFFER_SRGB), false);
    EXPECT_EQ(ctx.isEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE), false);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    GLint v = 1;
    ctx.getIntegerv(GL_FRAMEBUFFER_SRGB, &v);
    EXPECT_EQ(v, 0);
    ctx.getIntegerv(GL_SAMPLE_ALPHA_TO_COVERAGE, &v);
    EXPECT_EQ(v, 0);

    setCurrentContext(nullptr);
}

// Enabling/disabling pushes to the backend only on change (SPEC §10: no
// redundant native call).
TEST_CASE("srgb_and_alpha_to_coverage_push_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    int enableAtStart = backend.enableCalls;
    int disableAtStart = backend.disableCalls;

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    EXPECT_EQ(ctx.isEnabled(GL_FRAMEBUFFER_SRGB), true);
    EXPECT_EQ(ctx.isEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE), true);
    EXPECT_EQ(backend.enableCalls, enableAtStart); // not yet flushed

    glDrawArrays(GL_TRIANGLES, 3, 3);
    EXPECT_EQ(backend.enableCalls, enableAtStart + 2);
    EXPECT_EQ(backend.disableCalls, disableAtStart);

    // A second draw with no change does not push again.
    glDrawArrays(GL_TRIANGLES, 6, 3);
    EXPECT_EQ(backend.enableCalls, enableAtStart + 2);

    // Disabling both pushes a single disable each.
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDrawArrays(GL_TRIANGLES, 9, 3);
    EXPECT_EQ(backend.disableCalls, disableAtStart + 2);
    EXPECT_EQ(backend.enableCalls, enableAtStart + 2);

    setCurrentContext(nullptr);
}
