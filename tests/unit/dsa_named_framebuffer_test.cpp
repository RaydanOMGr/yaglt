#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// --- DSA renderbuffer surface (SPEC §8.2 / §9.2) ---

TEST_CASE("create_renderbuffers_generates_names") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb[2] = {0, 0};
    ctx.createRenderbuffers(2, rb);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(rb[0], 0u);
    EXPECT_NE(rb[1], 0u);
    EXPECT_NE(rb[0], rb[1]);
}

TEST_CASE("named_renderbuffer_storage_records_and_forwards") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, 128, 64);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mr = static_cast<MockRenderbuffer*>(ctx.getRenderbuffer(rb)->backend.get());
    EXPECT_EQ(mr->renderbufferStorageCalls, 1);
    EXPECT_EQ(mr->lastInternalFormat, GL_RGBA8);
    EXPECT_EQ(mr->lastWidth, 128);
    EXPECT_EQ(mr->lastHeight, 64);
    EXPECT_TRUE(ctx.getRenderbuffer(rb)->storageSet);
}

TEST_CASE("named_renderbuffer_storage_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, -1, 64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, 64, -2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("named_renderbuffer_storage_ungenerated_name") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.namedRenderbufferStorage(999, GL_RGBA8, 64, 64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("named_renderbuffer_storage_multisample_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorageMultisample(rb, 4, GL_RGBA8, 32, 32);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mr = static_cast<MockRenderbuffer*>(ctx.getRenderbuffer(rb)->backend.get());
    EXPECT_EQ(mr->renderbufferStorageMultisampleCalls, 1);
    EXPECT_EQ(mr->lastSamples, 4);
    EXPECT_EQ(ctx.getRenderbuffer(rb)->samples, 4);
}

TEST_CASE("get_named_renderbuffer_parameter_reads_storage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorageMultisample(rb, 2, GL_RGBA8, 48, 24);
    int32_t v = -1;
    ctx.getNamedRenderbufferParameteriv(rb, GL_RENDERBUFFER_WIDTH, &v);
    EXPECT_EQ(v, 48);
    ctx.getNamedRenderbufferParameteriv(rb, GL_RENDERBUFFER_HEIGHT, &v);
    EXPECT_EQ(v, 24);
    ctx.getNamedRenderbufferParameteriv(rb, GL_RENDERBUFFER_INTERNAL_FORMAT, &v);
    EXPECT_EQ(v, static_cast<int32_t>(GL_RGBA8));
    ctx.getNamedRenderbufferParameteriv(rb, GL_RENDERBUFFER_SAMPLES, &v);
    EXPECT_EQ(v, 2);
}

TEST_CASE("get_named_renderbuffer_parameter_null_invalid") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.getNamedRenderbufferParameteriv(rb, GL_RENDERBUFFER_WIDTH, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// Classic (non-DSA) glGetRenderbufferParameteriv (SPEC §9.2.4) operates on the
// renderbuffer currently bound to GL_RENDERBUFFER.
TEST_CASE("get_renderbuffer_parameter_iv_bound_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.bindRenderbuffer(rb);
    ctx.namedRenderbufferStorageMultisample(rb, 4, GL_RGBA8, 96, 48);
    int32_t v = -1;
    ctx.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(v, 96);
    ctx.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &v);
    EXPECT_EQ(v, 48);
    ctx.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &v);
    EXPECT_EQ(v, 4);
    ctx.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &v);
    EXPECT_EQ(v, static_cast<int32_t>(GL_RGBA8));
}

TEST_CASE("get_renderbuffer_parameter_invalid_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rb;
    ctx.createRenderbuffers(1, &rb);
    ctx.bindRenderbuffer(rb);
    int32_t v = -1;
    ctx.getRenderbufferParameteriv(GL_TEXTURE_2D, GL_RENDERBUFFER_WIDTH, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("get_renderbuffer_parameter_no_bound_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t v = -1;
    ctx.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// --- DSA framebuffer surface (SPEC §9.2) ---

TEST_CASE("create_framebuffers_generates_names") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb[2] = {0, 0};
    ctx.createFramebuffers(2, fb);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(fb[0], 0u);
    EXPECT_NE(fb[1], 0u);
}

TEST_CASE("named_framebuffer_renderbuffer_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, rb;
    ctx.createFramebuffers(1, &fb);
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, 64, 64);
    ctx.namedFramebufferRenderbuffer(fb, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferRenderbufferCalls, 1);
    EXPECT_EQ(mf->lastAttachment, GL_COLOR_ATTACHMENT0);
}

TEST_CASE("named_framebuffer_renderbuffer_unknown_object_invalid") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.namedFramebufferRenderbuffer(fb, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 777);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("named_framebuffer_texture_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 64, 64);
    ctx.namedFramebufferTexture(fb, GL_COLOR_ATTACHMENT0, tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTexture2DCalls, 1);
    EXPECT_EQ(mf->lastAttachment, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(mf->lastLevel, 0);
}

TEST_CASE("named_framebuffer_texture_layer_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createTextures(GL_TEXTURE_2D_ARRAY, 1, &tex);
    ctx.textureStorage3D(tex, 1, GL_RGBA8, 64, 64, 4);
    ctx.namedFramebufferTextureLayer(fb, GL_COLOR_ATTACHMENT0, tex, 0, 2);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferTextureLayerCalls, 1);
    EXPECT_EQ(mf->lastLayer, 2);
    EXPECT_EQ(mf->lastLayerLevel, 0);
}

TEST_CASE("check_named_framebuffer_status_completeness") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, rb;
    ctx.createFramebuffers(1, &fb);
    // Empty FBO -> missing attachment.
    EXPECT_EQ(ctx.checkNamedFramebufferStatus(fb, GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
    // Attach a renderbuffer with storage -> complete.
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, 64, 64);
    ctx.namedFramebufferRenderbuffer(fb, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
    EXPECT_EQ(ctx.checkNamedFramebufferStatus(fb, GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_COMPLETE);
    // Attachment without storage -> incomplete.
    GLObjectName rb2;
    ctx.createRenderbuffers(1, &rb2); // no storage
    ctx.namedFramebufferRenderbuffer(fb, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb2);
    EXPECT_EQ(ctx.checkNamedFramebufferStatus(fb, GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
}

TEST_CASE("named_framebuffer_parameteri_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.namedFramebufferParameteri(fb, GL_FRAMEBUFFER_DEFAULT_WIDTH, 512);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* mf =
        static_cast<MockFramebuffer*>(ctx.getFramebuffer(fb)->backend.get());
    EXPECT_EQ(mf->framebufferParameteriCalls, 1);
    EXPECT_EQ(mf->lastParamPname, GL_FRAMEBUFFER_DEFAULT_WIDTH);
    EXPECT_EQ(mf->lastParamValue, 512);
}

TEST_CASE("get_named_framebuffer_attachment_parameter_reads_object") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, rb, tex;
    ctx.createFramebuffers(1, &fb);
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, 64, 64);
    ctx.namedFramebufferRenderbuffer(fb, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
    int32_t type = 0, name = 0, level = -1;
    ctx.getNamedFramebufferAttachmentParameteriv(
        fb, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    EXPECT_EQ(type, GL_RENDERBUFFER);
    ctx.getNamedFramebufferAttachmentParameteriv(
        fb, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
    EXPECT_EQ(name, static_cast<int32_t>(rb));
    ctx.getNamedFramebufferAttachmentParameteriv(
        fb, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
    EXPECT_EQ(level, 0);

    // Swap in a texture attachment and confirm type/name flip.
    ctx.createTextures(GL_TEXTURE_2D, 1, &tex);
    ctx.textureStorage2D(tex, 1, GL_RGBA8, 64, 64);
    ctx.namedFramebufferTexture(fb, GL_COLOR_ATTACHMENT0, tex, 3);
    ctx.getNamedFramebufferAttachmentParameteriv(
        fb, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    EXPECT_EQ(type, GL_TEXTURE);
    ctx.getNamedFramebufferAttachmentParameteriv(
        fb, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
    EXPECT_EQ(level, 3);
}

// Classic (non-DSA) glGetFramebufferAttachmentParameteriv (SPEC §9.2.3) operates
// on the framebuffer currently bound to the given target.
TEST_CASE("get_framebuffer_attachment_parameter_bound_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb, rb;
    ctx.createFramebuffers(1, &fb);
    ctx.createRenderbuffers(1, &rb);
    ctx.namedRenderbufferStorage(rb, GL_RGBA8, 64, 64);
    ctx.namedFramebufferRenderbuffer(fb, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
    ctx.bindFramebuffer(fb);
    int32_t type = 0, name = 0;
    ctx.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                            &type);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(type, GL_RENDERBUFFER);
    ctx.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                            &name);
    EXPECT_EQ(name, static_cast<int32_t>(rb));
}

TEST_CASE("get_framebuffer_attachment_parameter_invalid_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.bindFramebuffer(fb);
    int32_t v = 0;
    ctx.getFramebufferAttachmentParameteriv(GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                            &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("get_framebuffer_attachment_parameter_no_bound_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int32_t v = 0;
    ctx.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                            &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("get_named_framebuffer_parameter_null_invalid") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.getNamedFramebufferParameteriv(fb, GL_FRAMEBUFFER_DEFAULT_WIDTH, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("blit_named_framebuffer_binds_and_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName src, dst;
    ctx.createFramebuffers(1, &src);
    ctx.createFramebuffers(1, &dst);
    ctx.blitNamedFramebuffer(src, dst, 0, 0, 64, 64, 0, 0, 64, 64,
                            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->blitFramebufferCalls, 1);
    // The named read/draw framebuffers were bound to the driver (plus a restore
    // of the tracked default binding, since DSA must not leave a side effect):
    // read bind, draw bind, then restore to the default (0) bind.
    EXPECT_EQ(backend->bindFramebufferCalls, 3);
}

TEST_CASE("blit_named_framebuffer_invalid_mask") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName src, dst;
    ctx.createFramebuffers(1, &src);
    ctx.createFramebuffers(1, &dst);
    ctx.blitNamedFramebuffer(src, dst, 0, 0, 64, 64, 0, 0, 64, 64, 0x1234,
                            GL_NEAREST);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("invalidate_named_framebuffer_data_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    uint32_t atts[1] = {GL_COLOR_ATTACHMENT0};
    ctx.invalidateNamedFramebufferData(fb, 1, atts);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->invalidateFramebufferCalls, 1);
}

TEST_CASE("clear_named_framebuffer_color_pushes_value_and_clears") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    int32_t color[4] = {10, 20, 30, 40};
    ctx.clearNamedFramebufferiv(fb, GL_COLOR, 0, color);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->clearColorCalls, 1);
    EXPECT_EQ(backend->clearCalls, 1);
    EXPECT_EQ(backend->lastClearR, 10.0f);
    EXPECT_EQ(backend->lastClearG, 20.0f);
}

TEST_CASE("clear_named_framebuffer_depth_pushes_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    int32_t depth = 5;
    ctx.clearNamedFramebufferiv(fb, GL_DEPTH, 0, &depth);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->clearDepthCalls, 1);
    EXPECT_EQ(backend->clearCalls, 1);
}

TEST_CASE("clear_named_framebuffer_null_value_invalid") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fb;
    ctx.createFramebuffers(1, &fb);
    ctx.clearNamedFramebufferiv(fb, GL_COLOR, 0, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}
