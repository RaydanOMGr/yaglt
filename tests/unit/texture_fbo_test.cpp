#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// Helper: cast an opaque backend handle to its concrete Mock type.
template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

TEST_CASE("texture_texImage2D_allocates_storage_and_records_state") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);

    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_NE(t, nullptr);
    EXPECT_TRUE(t->storageSet);
    EXPECT_EQ(t->images.size(), 1u);
    EXPECT_EQ(t->images[0].width, 64);
    EXPECT_EQ(t->images[0].height, 32);
    EXPECT_EQ(t->images[0].internalFormat, GL_RGBA);
    EXPECT_EQ(t->images[0].format, GL_RGBA);
    EXPECT_EQ(t->images[0].type, GL_UNSIGNED_BYTE);
    EXPECT_FALSE(t->images[0].hasData);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_NE(mt, nullptr);
    EXPECT_EQ(mt->texImage2DCalls, 1);
    EXPECT_EQ(mt->lastWidth, 64);
    EXPECT_EQ(mt->lastInternalFormat, GL_RGBA);
}

TEST_CASE("texture_texParameteri_records_and_pushes") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);

    ctx.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->params[GL_TEXTURE_MIN_FILTER], GL_LINEAR);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texParameteriCalls, 1);
    EXPECT_EQ(mt->lastParamPname, GL_TEXTURE_MIN_FILTER);
    EXPECT_EQ(mt->lastParam, GL_LINEAR);
}

TEST_CASE("texture_texImage2D_without_bound_texture_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("texture_texImage2D_negative_size_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, -1, 4, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("bufferData_pushes_to_backend_resource") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);

    int data[4] = {42, 0, 0, 0};
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, data);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    BufferObject* b = ctx.getBuffer(buf);
    EXPECT_EQ(b->size, 16);
    EXPECT_EQ(b->usage, GL_STATIC_DRAW);

    MockBuffer* mb = as<MockBuffer>(b->backend.get());
    EXPECT_EQ(mb->bufferDataCalls, 1);
    EXPECT_EQ(mb->lastSize, 16);
    EXPECT_EQ(mb->lastUsage, GL_STATIC_DRAW);
    EXPECT_TRUE(mb->lastHadData);
}

TEST_CASE("framebuffer_texture_attachment_records_and_pushes") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    GLObjectName tex = ctx.genTexture();
    ctx.bindFramebuffer(fbo);
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);

    ctx.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    FramebufferObject* f = ctx.getFramebuffer(fbo);
    EXPECT_EQ(f->attachments.size(), 1u);
    EXPECT_EQ(f->attachments[0].attachment, GL_COLOR_ATTACHMENT0);
    EXPECT_EQ(f->attachments[0].name, tex);
    EXPECT_EQ(f->attachments[0].texTarget, GL_TEXTURE_2D);
    EXPECT_TRUE(f->isStructurallyComplete());
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

    MockFramebuffer* mf = as<MockFramebuffer>(f->backend.get());
    EXPECT_EQ(mf->framebufferTexture2DCalls, 1);
    EXPECT_EQ(mf->lastAttachment, GL_COLOR_ATTACHMENT0);
    // native texture id should be the mock texture id (non-zero).
    EXPECT_NE(mf->lastNativeTexture, 0u);
}

TEST_CASE("framebuffer_renderbuffer_attachment_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    GLObjectName rbo = ctx.genRenderbuffer();
    ctx.bindFramebuffer(fbo);
    ctx.bindRenderbuffer(rbo);

    ctx.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, rbo);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    FramebufferObject* f = ctx.getFramebuffer(fbo);
    EXPECT_EQ(f->attachments.size(), 1u);
    EXPECT_EQ(f->attachments[0].type, 1u); // renderbuffer
    EXPECT_EQ(f->attachments[0].name, rbo);

    MockFramebuffer* mf = as<MockFramebuffer>(f->backend.get());
    EXPECT_EQ(mf->framebufferRenderbufferCalls, 1);
    EXPECT_EQ(mf->lastAttachment, GL_DEPTH_ATTACHMENT);
}

TEST_CASE("framebuffer_attach_missing_texture_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    ctx.bindFramebuffer(fbo);
    ctx.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             12345, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("checkFramebufferStatus_incomplete_without_attachments") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    ctx.bindFramebuffer(fbo);
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
}

TEST_CASE("checkFramebufferStatus_incomplete_attachment_no_texture_storage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    GLObjectName tex = ctx.genTexture(); // generated but no texImage2D yet
    ctx.bindFramebuffer(fbo);
    ctx.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             tex, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
    // Allocating storage makes the attachment complete.
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_COMPLETE);
}

TEST_CASE("checkFramebufferStatus_incomplete_attachment_no_rbo_storage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    GLObjectName rbo = ctx.genRenderbuffer(); // generated but no storage yet
    ctx.bindFramebuffer(fbo);
    ctx.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, rbo);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
    // Allocating storage makes the attachment complete.
    ctx.bindRenderbuffer(rbo);
    ctx.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 8, 8);
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_COMPLETE);
}

TEST_CASE("pixelStorei_records_state_and_pushes_to_sink") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->pixelStoreiCalls, 1);
    // Setting the same value again should not push a redundant call.
    ctx.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
    EXPECT_EQ(backend->pixelStoreiCalls, 1);
    ctx.pixelStorei(GL_UNPACK_ALIGNMENT, 8);
    EXPECT_EQ(backend->pixelStoreiCalls, 2);
}

TEST_CASE("renderbuffer_storage_allocates_and_records_state") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rbo = ctx.genRenderbuffer();
    ctx.bindRenderbuffer(rbo);

    ctx.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 32, 16);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    RenderbufferObject* r = ctx.getRenderbuffer(rbo);
    EXPECT_NE(r, nullptr);
    EXPECT_TRUE(r->storageSet);
    EXPECT_EQ(r->internalFormat, GL_RGBA8);
    EXPECT_EQ(r->width, 32);
    EXPECT_EQ(r->height, 16);

    MockRenderbuffer* mr = as<MockRenderbuffer>(r->backend.get());
    EXPECT_NE(mr, nullptr);
    EXPECT_EQ(mr->renderbufferStorageCalls, 1);
    EXPECT_EQ(mr->lastTarget, GL_RENDERBUFFER);
    EXPECT_EQ(mr->lastInternalFormat, GL_RGBA8);
    EXPECT_EQ(mr->lastWidth, 32);
    EXPECT_EQ(mr->lastHeight, 16);
}

TEST_CASE("renderbuffer_storage_requires_bound_renderbuffer") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 4, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("renderbuffer_storage_rejects_negative_size") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rbo = ctx.genRenderbuffer();
    ctx.bindRenderbuffer(rbo);
    ctx.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, -1, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("renderbuffer_storage_multisample_allocates_and_records") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rbo = ctx.genRenderbuffer();
    ctx.bindRenderbuffer(rbo);

    ctx.renderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, 32, 32);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    RenderbufferObject* r = ctx.getRenderbuffer(rbo);
    EXPECT_NE(r, nullptr);
    EXPECT_TRUE(r->storageSet);
    EXPECT_EQ(r->internalFormat, GL_RGBA8);
    EXPECT_EQ(r->width, 32);
    EXPECT_EQ(r->height, 32);
    EXPECT_EQ(r->samples, 4);

    MockRenderbuffer* mr = as<MockRenderbuffer>(r->backend.get());
    EXPECT_NE(mr, nullptr);
    EXPECT_EQ(mr->renderbufferStorageMultisampleCalls, 1);
    EXPECT_EQ(mr->lastTarget, GL_RENDERBUFFER);
    EXPECT_EQ(mr->lastSamples, 4);
    EXPECT_EQ(mr->lastInternalFormat, GL_RGBA8);
    EXPECT_EQ(mr->lastWidth, 32);
    EXPECT_EQ(mr->lastHeight, 32);
}

TEST_CASE("renderbuffer_storage_multisample_requires_bound_renderbuffer") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.renderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, 4, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("renderbuffer_storage_multisample_rejects_negative") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName rbo = ctx.genRenderbuffer();
    ctx.bindRenderbuffer(rbo);
    ctx.renderbufferStorageMultisample(GL_RENDERBUFFER, -1, GL_RGBA8, 4, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.renderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_RGBA8, -2, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("gl_renderbuffer_storage_multisample_public_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint rb = 0;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, 16, 16);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    RenderbufferObject* r = ctx.getRenderbuffer(rb);
    MockRenderbuffer* mr = as<MockRenderbuffer>(r->backend.get());
    EXPECT_EQ(mr->renderbufferStorageMultisampleCalls, 1);
    EXPECT_EQ(mr->lastSamples, 4);

    setCurrentContext(nullptr);
}

TEST_CASE("framebuffer_renderbuffer_depth_completeness_via_mock") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName fbo = ctx.genFramebuffer();
    GLObjectName rbo = ctx.genRenderbuffer();
    ctx.bindFramebuffer(fbo);
    ctx.bindRenderbuffer(rbo);
    ctx.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 8, 8);
    ctx.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, rbo);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.checkFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_COMPLETE);
}

TEST_CASE("gl_api_surface_for_new_texture_fbo_pixelstore_calls") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                          tex, 0);
    EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}
