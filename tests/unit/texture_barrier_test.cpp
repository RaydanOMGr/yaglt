#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glTextureBarrier_forwards_to_backend_after_state_flush") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // glTextureBarrier has no parameters and never raises an error (SPEC §10.9.2).
    glTextureBarrier();

    EXPECT_EQ(backend.textureBarrierCalls, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    setCurrentContext(nullptr);
}

TEST_CASE("glTextureBarrier_null_context_is_safe_noop") {
    setCurrentContext(nullptr);

    // With no current context the C dispatch must not crash and must not call
    // the backend (no context owns one).
    glTextureBarrier();

    setCurrentContext(nullptr);
}
