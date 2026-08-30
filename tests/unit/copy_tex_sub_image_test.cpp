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

template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

// --- Copy texture sub-image from framebuffer (SPEC §8.5) ---

TEST_CASE("copyTexSubImage2D_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 1, 2, 3, 4, 8, 16);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexSubImage2DCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, static_cast<uint32_t>(GL_TEXTURE_2D));
    EXPECT_EQ(mt->lastSubLevel, 0);
    EXPECT_EQ(mt->lastCopySubXoffset, 1);
    EXPECT_EQ(mt->lastCopySubYoffset, 2);
    EXPECT_EQ(mt->lastCopySubX, 3);
    EXPECT_EQ(mt->lastCopySubY, 4);
    EXPECT_EQ(mt->lastCopySubWidth, 8);
    EXPECT_EQ(mt->lastCopySubHeight, 16);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTexSubImage1D_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_1D, tex);
    glCopyTexSubImage1D(GL_TEXTURE_1D, 0, 2, 3, 4, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexSubImage1DCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, static_cast<uint32_t>(GL_TEXTURE_1D));
    EXPECT_EQ(mt->lastCopySubXoffset, 2);
    EXPECT_EQ(mt->lastCopySubX, 3);
    EXPECT_EQ(mt->lastCopySubY, 4);
    EXPECT_EQ(mt->lastCopySubWidth, 8);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTexSubImage3D_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_3D, tex);
    glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 1, 2, 3, 4, 5, 8, 16);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexSubImage3DCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, static_cast<uint32_t>(GL_TEXTURE_3D));
    EXPECT_EQ(mt->lastCopySubXoffset, 1);
    EXPECT_EQ(mt->lastCopySubYoffset, 2);
    EXPECT_EQ(mt->lastCopySubZoffset, 3);
    EXPECT_EQ(mt->lastCopySubX, 4);
    EXPECT_EQ(mt->lastCopySubY, 5);
    EXPECT_EQ(mt->lastCopySubWidth, 8);
    EXPECT_EQ(mt->lastCopySubHeight, 16);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTextureSubImage2D_dsa_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glCopyTextureSubImage2D(tex, 0, 0, 0, 3, 4, 8, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexSubImage2DCalls, 1);
    EXPECT_EQ(mt->lastSubTarget, static_cast<uint32_t>(GL_TEXTURE_2D));
    EXPECT_EQ(mt->lastCopySubX, 3);
    EXPECT_EQ(mt->lastCopySubY, 4);
    EXPECT_EQ(mt->lastCopySubWidth, 8);
    EXPECT_EQ(mt->lastCopySubHeight, 8);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTextureSubImage3D_dsa_records_call") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = 0;
    glCreateTextures(GL_TEXTURE_3D, 1, &tex);
    glTextureStorage3D(tex, 1, GL_RGBA8, 8, 8, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glCopyTextureSubImage3D(tex, 0, 1, 2, 3, 4, 5, 8, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexSubImage3DCalls, 1);
    EXPECT_EQ(mt->lastCopySubX, 4);
    EXPECT_EQ(mt->lastCopySubY, 5);
    EXPECT_EQ(mt->lastCopySubWidth, 8);
    EXPECT_EQ(mt->lastCopySubHeight, 8);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTexSubImage2D_rejects_no_bound_texture") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 4, 4);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTexSubImage2D_rejects_rectangle_target") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_RECTANGLE, tex);
    glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, 4, 4);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTexSubImage2D_rejects_negative_offset") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, -1, 0, 0, 0, 4, 4);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTexSubImage2D_rejects_negative_width") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, -4, 4);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    setCurrentContext(nullptr);
}

TEST_CASE("copyTextureSubImage2D_dsa_rejects_ungenerated_name") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glCopyTextureSubImage2D(999u, 0, 0, 0, 0, 0, 4, 4);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    setCurrentContext(nullptr);
}
