#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glColorMask_pushes_only_on_change_and_records_channels") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default is (true,true,true,true); a different mask pushes on flush.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glFlushState();
    EXPECT_EQ(backend.colorMaskCalls, 1);
    EXPECT_EQ(backend.lastColorMaskR, false);
    EXPECT_EQ(backend.lastColorMaskA, false);

    // Same mask again must not re-push (SPEC §10).
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glFlushState();
    EXPECT_EQ(backend.colorMaskCalls, 1);

    // A changed mask pushes again.
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glFlushState();
    EXPECT_EQ(backend.colorMaskCalls, 2);
    EXPECT_EQ(backend.lastColorMaskG, false);
    EXPECT_EQ(backend.lastColorMaskA, false);

    setCurrentContext(nullptr);
}

TEST_CASE("glGetColorMask_returns_tracked_channels") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);

    GLint iv[4] = {0};
    glGetIntegerv(GL_COLOR_WRITEMASK, iv);
    EXPECT_EQ(iv[0], 1);
    EXPECT_EQ(iv[1], 0);
    EXPECT_EQ(iv[2], 1);
    EXPECT_EQ(iv[3], 0);

    GLboolean bv[4] = {0};
    glGetBooleanv(GL_COLOR_WRITEMASK, bv);
    EXPECT_EQ(bv[0], GL_TRUE);
    EXPECT_EQ(bv[1], GL_FALSE);

    setCurrentContext(nullptr);
}
