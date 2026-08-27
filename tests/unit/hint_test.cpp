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

// glHint records the requested target/mode and is queryable; invalid target or
// mode is GL_INVALID_ENUM (SPEC §21.1.1).
TEST_CASE("hint_records_target_and_mode") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT), GL_NICEST);

    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_FASTEST);
    EXPECT_EQ(ctx.getHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT), GL_FASTEST);

    // Unknown target -> INVALID_ENUM.
    glHint(0xDEAD, GL_NICEST);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    // Unknown mode -> INVALID_ENUM.
    glHint(GL_FOG_HINT, 0xBEEF);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    setCurrentContext(nullptr);
}

// glHint pushes the hint to the backend only when it changes (SPEC §10: no
// redundant native call), at the next state flush.
TEST_CASE("hint_pushed_to_backend_on_flush") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
    EXPECT_EQ(backend.hintCalls, 0); // not yet flushed

    glUseProgram(3);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_EQ(backend.hintCalls, 1);
    EXPECT_EQ(backend.lastHintTarget, GL_TEXTURE_COMPRESSION_HINT);
    EXPECT_EQ(backend.lastHintMode, GL_NICEST);

    // A second draw with no hint change does not push again.
    glDrawArrays(GL_TRIANGLES, 3, 3);
    EXPECT_EQ(backend.hintCalls, 1);

    // Changing the mode pushes once more.
    glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
    glDrawArrays(GL_TRIANGLES, 6, 3);
    EXPECT_EQ(backend.hintCalls, 2);
    EXPECT_EQ(backend.lastHintMode, GL_FASTEST);

    setCurrentContext(nullptr);
}
