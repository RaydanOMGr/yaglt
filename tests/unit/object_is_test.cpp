#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// glIsBuffer (SPEC §6.1.1) reports whether a name is a generated buffer object.
TEST_CASE("is_buffer_false_for_ungenerated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    EXPECT_FALSE(ctx.isBuffer(0));
    EXPECT_FALSE(ctx.isBuffer(123));
}

TEST_CASE("is_buffer_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    EXPECT_NE(buf, 0u);
    EXPECT_TRUE(ctx.isBuffer(buf));
    ctx.deleteBuffer(buf);
    EXPECT_FALSE(ctx.isBuffer(buf));
}

TEST_CASE("glIsBuffer_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName buf = 0;
    glGenBuffers(1, &buf);
    EXPECT_EQ(glIsBuffer(buf), GL_TRUE);
    EXPECT_EQ(glIsBuffer(0), GL_FALSE);
    setCurrentContext(nullptr);
}

// glIsTexture (SPEC §8.1) reports whether a name is a generated texture.
TEST_CASE("is_texture_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    EXPECT_TRUE(ctx.isTexture(tex));
    EXPECT_FALSE(ctx.isTexture(tex + 1));
    ctx.deleteTexture(tex);
    EXPECT_FALSE(ctx.isTexture(tex));
}

TEST_CASE("glIsTexture_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = 0;
    glGenTextures(1, &tex);
    EXPECT_EQ(glIsTexture(tex), GL_TRUE);
    setCurrentContext(nullptr);
}

// glIsRenderbuffer (SPEC §9.2.1) reports whether a name is a generated RBO.
TEST_CASE("is_renderbuffer_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rbo = ctx.genRenderbuffer();
    EXPECT_TRUE(ctx.isRenderbuffer(rbo));
    ctx.deleteRenderbuffer(rbo);
    EXPECT_FALSE(ctx.isRenderbuffer(rbo));
}

TEST_CASE("glIsRenderbuffer_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName rbo = 0;
    glGenRenderbuffers(1, &rbo);
    EXPECT_EQ(glIsRenderbuffer(rbo), GL_TRUE);
    setCurrentContext(nullptr);
}

// glIsFramebuffer (SPEC §9.2.1) reports whether a name is a generated FBO.
TEST_CASE("is_framebuffer_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    EXPECT_TRUE(ctx.isFramebuffer(fbo));
    ctx.deleteFramebuffer(fbo);
    EXPECT_FALSE(ctx.isFramebuffer(fbo));
}

TEST_CASE("glIsFramebuffer_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName fbo = 0;
    glGenFramebuffers(1, &fbo);
    EXPECT_EQ(glIsFramebuffer(fbo), GL_TRUE);
    setCurrentContext(nullptr);
}

// glIsTransformFeedback (SPEC §13.2) reports whether a name is a generated TF.
TEST_CASE("is_transform_feedback_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName xfb = ctx.genTransformFeedback();
    EXPECT_TRUE(ctx.isTransformFeedback(xfb));
    ctx.deleteTransformFeedback(xfb);
    EXPECT_FALSE(ctx.isTransformFeedback(xfb));
}

TEST_CASE("glIsTransformFeedback_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName xfb = 0;
    glGenTransformFeedbacks(1, &xfb);
    EXPECT_EQ(glIsTransformFeedback(xfb), GL_TRUE);
    setCurrentContext(nullptr);
}

// glIsShader (SPEC §7.1) reports whether a name is a generated shader.
TEST_CASE("is_shader_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName sh = ctx.createShader(GL_VERTEX_SHADER);
    EXPECT_TRUE(ctx.isShader(sh));
    ctx.deleteShader(sh);
    EXPECT_FALSE(ctx.isShader(sh));
}

TEST_CASE("glIsShader_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName sh = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_EQ(glIsShader(sh), GL_TRUE);
    setCurrentContext(nullptr);
}

// glIsProgram (SPEC §7.1) reports whether a name is a generated program.
TEST_CASE("is_program_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName prog = ctx.createProgram();
    EXPECT_TRUE(ctx.isProgram(prog));
    ctx.deleteProgram(prog);
    EXPECT_FALSE(ctx.isProgram(prog));
}

TEST_CASE("glIsProgram_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName prog = glCreateProgram();
    EXPECT_EQ(glIsProgram(prog), GL_TRUE);
    setCurrentContext(nullptr);
}
