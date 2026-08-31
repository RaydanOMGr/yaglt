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

// Indexed capabilities (SPEC §10.3.1) are tracked per slot, default off.
TEST_CASE("indexed_caps_default_off") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(glIsEnabledi(0x0BE2, 0), 0);
    EXPECT_EQ(glIsEnabledi(0x0C11, 3), 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

// glEnablei/glDisablei flip tracked state and push to the backend only on
// change (SPEC §10: no redundant native call).
TEST_CASE("indexed_caps_push_on_change") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    int enableAtStart = backend.enableIndexedCalls;
    int disableAtStart = backend.disableIndexedCalls;

    glEnablei(0x0BE2, 0);
    glEnablei(0x0BE2, 1);
    EXPECT_EQ(glIsEnabledi(0x0BE2, 0), 1);
    EXPECT_EQ(glIsEnabledi(0x0BE2, 1), 1);
    EXPECT_EQ(backend.enableIndexedCalls, enableAtStart); // not yet flushed

    glDrawArrays(GL_TRIANGLES, 3, 3);
    EXPECT_EQ(backend.enableIndexedCalls, enableAtStart + 2);
    EXPECT_EQ(backend.disableIndexedCalls, disableAtStart);

    // No change on a repeat draw.
    glDrawArrays(GL_TRIANGLES, 6, 3);
    EXPECT_EQ(backend.enableIndexedCalls, enableAtStart + 2);

    // Disabling one slot pushes a single disable.
    glDisablei(0x0BE2, 0);
    glDrawArrays(GL_TRIANGLES, 9, 3);
    EXPECT_EQ(backend.disableIndexedCalls, disableAtStart + 1);
    EXPECT_EQ(backend.enableIndexedCalls, enableAtStart + 2);

    setCurrentContext(nullptr);
}

// Only 0x0BE2 and 0x0C11 are indexable (SPEC §10.3.1 / §22.3).
TEST_CASE("indexed_caps_validate_cap_and_index") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glEnablei(0x0B71, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    glEnablei(0x0BE2, 16);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glIsEnabledi(0x0C11, 100);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
