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

template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

TEST_CASE("texsubimage2d_uploads_region_into_allocated_level") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    ctx.texSubImage2D(GL_TEXTURE_2D, 0, 4, 8, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE,
                      nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->subimages.size(), 1u);
    EXPECT_EQ(t->subimages[0].dim, 2);
    EXPECT_EQ(t->subimages[0].xoffset, 4);
    EXPECT_EQ(t->subimages[0].yoffset, 8);
    EXPECT_EQ(t->subimages[0].width, 16);
    EXPECT_EQ(t->subimages[0].height, 16);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texSubImage2DCalls, 1);
    EXPECT_EQ(mt->lastSubXoffset, 4);
    EXPECT_EQ(mt->lastSubYoffset, 8);
    EXPECT_EQ(mt->lastSubWidth, 16);
    EXPECT_EQ(mt->lastSubHeight, 16);
}

TEST_CASE("texsubimage3d_uploads_region") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_3D, tex);
    // Allocate level 0 with 64x32 (the frontend records 2D dims for the region
    // fit check; the backend 3D call still exercises the path).
    ctx.texImage2D(GL_TEXTURE_3D, 0, GL_RGBA, 64, 32, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    ctx.texSubImage3D(GL_TEXTURE_3D, 0, 1, 2, 3, 8, 8, 8, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->subimages.size(), 1u);
    EXPECT_EQ(t->subimages[0].dim, 3);
    EXPECT_EQ(t->subimages[0].zoffset, 3);
    EXPECT_EQ(t->subimages[0].depth, 8);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texSubImage3DCalls, 1);
    EXPECT_EQ(mt->lastSubZoffset, 3);
    EXPECT_EQ(mt->lastSubDepth, 8);
}

TEST_CASE("texsubimage_without_bound_texture_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE,
                      nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("texsubimage_on_unallocated_level_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texSubImage2D(GL_TEXTURE_2D, 5, 0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE,
                      nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("texsubimage_negative_dims_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    ctx.texSubImage2D(GL_TEXTURE_2D, 0, -1, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE,
                      nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("texsubimage_region_outside_level_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    ctx.texSubImage2D(GL_TEXTURE_2D, 0, 4, 4, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE,
                      nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("copyteximage2d_copies_from_framebuffer") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.copyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 32, 32, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexImage2DCalls, 1);
    EXPECT_EQ(mt->lastCopyInternalFormat, GL_RGBA);
    EXPECT_EQ(mt->lastCopyWidth, 32);
    EXPECT_EQ(mt->lastCopyHeight, 32);
    EXPECT_EQ(mt->lastCopyBorder, 0);
}

TEST_CASE("copyteximage2d_nonzero_border_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.copyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 32, 32, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("copyteximage1d_records_subimage") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_1D, tex);
    ctx.copyTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 0, 0, 16, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->copyTexImage1DCalls, 1);
    EXPECT_EQ(mt->lastCopyWidth, 16);
}
