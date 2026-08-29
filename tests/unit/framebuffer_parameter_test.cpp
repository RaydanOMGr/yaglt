#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

static GLObjectName makeFBO(Context& ctx) {
    GLObjectName fb = 0;
    ctx.createFramebuffers(1, &fb);
    ctx.bindFramebuffer(fb);
    return fb;
}

TEST_CASE("framebuffer_parameteri_invalid_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    glFramebufferParameteri(GL_TEXTURE_2D, GL_FRAMEBUFFER_DEFAULT_WIDTH, 64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    setCurrentContext(nullptr);
}

TEST_CASE("framebuffer_parameteri_default_bound_rejected") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    // No user framebuffer bound -> default framebuffer -> INVALID_OPERATION.
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    setCurrentContext(nullptr);
}

TEST_CASE("framebuffer_parameteri_invalid_pname") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName fb = makeFBO(ctx);

    glFramebufferParameteri(GL_FRAMEBUFFER, GL_TEXTURE_2D, 64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    (void)fb;
    setCurrentContext(nullptr);
}

TEST_CASE("framebuffer_parameteri_negative_param_rejected") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName fb = makeFBO(ctx);

    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, -1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    (void)fb;
    setCurrentContext(nullptr);
}

TEST_CASE("framebuffer_parameteri_valid_records_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName fb = makeFBO(ctx);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 64);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf = static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferParameteriCalls, 1);
    EXPECT_EQ(mf->lastTarget, static_cast<uint32_t>(GL_FRAMEBUFFER));
    EXPECT_EQ(mf->lastParamPname,
              static_cast<uint32_t>(GL_FRAMEBUFFER_DEFAULT_WIDTH));
    EXPECT_EQ(mf->lastParamValue, 64);

    setCurrentContext(nullptr);
}

TEST_CASE("named_framebuffer_parameteri_unknown_object_rejected") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    glNamedFramebufferParameteri(4242u, GL_FRAMEBUFFER_DEFAULT_WIDTH, 64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    setCurrentContext(nullptr);
}

TEST_CASE("named_framebuffer_parameteri_valid_records_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName fb = makeFBO(ctx);

    glNamedFramebufferParameteri(fb, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf = static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferParameteriCalls, 1);
    EXPECT_EQ(mf->lastParamPname,
              static_cast<uint32_t>(GL_FRAMEBUFFER_DEFAULT_SAMPLES));
    EXPECT_EQ(mf->lastParamValue, 4);

    setCurrentContext(nullptr);
}
