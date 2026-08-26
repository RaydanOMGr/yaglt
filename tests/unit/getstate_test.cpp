#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstring>

using namespace glcompat;

TEST_CASE("glget: viewport and scissor box query tracked state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glViewport(1, 2, 640, 480);
    glScissor(3, 4, 100, 200);

    GLint vp[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, vp);
    EXPECT_EQ(vp[0], 1);
    EXPECT_EQ(vp[1], 2);
    EXPECT_EQ(vp[2], 640);
    EXPECT_EQ(vp[3], 480);

    GLint sc[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, sc);
    EXPECT_EQ(sc[0], 3);
    EXPECT_EQ(sc[3], 200);

    // Float/double forms of the same state.
    GLfloat vpf[4] = {0, 0, 0, 0};
    glGetFloatv(GL_VIEWPORT, vpf);
    EXPECT_EQ(static_cast<int>(vpf[2]), 640);

    setCurrentContext(nullptr);
}

TEST_CASE("glget: capability and isEnabled agree") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_FALSE(glIsEnabled(GL_BLEND));
    glEnable(GL_BLEND);
    EXPECT_TRUE(glIsEnabled(GL_BLEND));

    GLint b = 0;
    glGetIntegerv(GL_BLEND, &b);
    EXPECT_EQ(b, 1);

    GLboolean bb = 0;
    glGetBooleanv(GL_BLEND, &bb);
    EXPECT_EQ(bb, 1);

    // CULL_FACE default off, DEPTH_TEST off.
    EXPECT_FALSE(glIsEnabled(GL_CULL_FACE));

    // Untracked cap -> error, glIsEnabled returns false.
    EXPECT_FALSE(glIsEnabled(GL_DITHER));
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("glget: blend/depth/clear value queries") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glBlendColor(0.1f, 0.2f, 0.3f, 0.4f);
    glClearColor(0.5f, 0.6f, 0.7f, 0.8f);
    glClearDepth(0.25);
    glDepthRange(0.1, 0.9);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(false);

    GLint s = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &s);
    EXPECT_EQ(s, static_cast<GLint>(GL_SRC_ALPHA));
    GLint d = 0;
    glGetIntegerv(GL_BLEND_DST_ALPHA, &d);
    EXPECT_EQ(d, static_cast<GLint>(GL_ZERO));

    GLfloat bc[4] = {0, 0, 0, 0};
    glGetFloatv(GL_BLEND_COLOR, bc);
    EXPECT_EQ(static_cast<int>(bc[3] * 10), 4);

    GLfloat cc[4] = {0, 0, 0, 0};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, cc);
    EXPECT_EQ(static_cast<int>(cc[0] * 10), 5);

    GLdouble cd = 0;
    glGetDoublev(GL_DEPTH_CLEAR_VALUE, &cd);
    EXPECT_EQ(static_cast<int>(cd * 100), 25);

    GLdouble dr[2] = {0, 0};
    glGetDoublev(GL_DEPTH_RANGE, dr);
    EXPECT_EQ(static_cast<int>(dr[0] * 10), 1);
    EXPECT_EQ(static_cast<int>(dr[1] * 10), 9);

    GLint dw = 1;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &dw);
    EXPECT_EQ(dw, 0);
    GLint df = 0;
    glGetIntegerv(GL_DEPTH_FUNC, &df);
    EXPECT_EQ(df, static_cast<GLint>(GL_LEQUAL));

    setCurrentContext(nullptr);
}

TEST_CASE("glget: unknown pname and null buffer error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint v = 0;
    glGetIntegerv(0xDEAD, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    glGetIntegerv(GL_BLEND, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
