#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glget: getStringi reports no extensions") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Only GL_EXTENSIONS is indexable (SPEC §22.2); we expose none, so every
    // index is out of range.
    const GLubyte* ext = glGetStringi(GL_EXTENSIONS, 0);
    EXPECT_EQ(ext, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // A non-indexable name is GL_INVALID_ENUM.
    const GLubyte* vendor = glGetStringi(GL_VENDOR, 0);
    EXPECT_EQ(vendor, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("glget: getGraphicsResetStatus is NO_ERROR") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    EXPECT_EQ(glGetGraphicsResetStatus(), GL_NO_ERROR);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("glget: getInteger64v matches tracked integer state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glViewport(0, 0, 640, 480);
    GLint64 vp[4] = {0, 0, 0, 0};
    glGetInteger64v(GL_VIEWPORT, vp);
    EXPECT_EQ(vp[2], 640);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glEnable(GL_BLEND);
    GLint64 blend = 0;
    glGetInteger64v(GL_BLEND, &blend);
    EXPECT_EQ(blend, 1);

    // Unknown pname -> INVALID_ENUM; null params -> INVALID_VALUE.
    GLint64 v = 0;
    glGetInteger64v(0xDEAD, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetInteger64v(GL_BLEND, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("glget: indexed queries read indexed capability state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Enabled at index 0 -> 1; untouched index 1 -> 0.
    glEnablei(GL_BLEND, 0);
    GLint b0 = 0;
    glGetIntegeri_v(GL_BLEND, 0, &b0);
    EXPECT_EQ(b0, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint b1 = 9;
    glGetIntegeri_v(GL_BLEND, 1, &b1);
    EXPECT_EQ(b1, 0);

    GLboolean bb = 0;
    glGetBooleani_v(GL_BLEND, 0, &bb);
    EXPECT_EQ(bb, 0x01);
    GLboolean bb1 = 0x01;
    glGetBooleani_v(GL_BLEND, 1, &bb1);
    EXPECT_EQ(bb1, 0x00);

    // SCISSOR_TEST indexed read.
    glEnablei(GL_SCISSOR_TEST, 2);
    GLint s = 0;
    glGetIntegeri_v(GL_SCISSOR_TEST, 2, &s);
    EXPECT_EQ(s, 1);

    // Out-of-range index -> INVALID_VALUE; non-indexable cap -> INVALID_ENUM.
    GLint oob = 0;
    glGetIntegeri_v(GL_BLEND, 16, &oob);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    GLint nonidx = 0;
    glGetIntegeri_v(GL_DITHER, 0, &nonidx);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Null params -> INVALID_VALUE.
    glGetIntegeri_v(GL_BLEND, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
