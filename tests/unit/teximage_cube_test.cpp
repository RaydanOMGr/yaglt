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

template <typename MockT, typename BaseT>
static MockT* as(BaseT* b) {
    return static_cast<MockT*>(b);
}

// Cube-map faces (GL_TEXTURE_CUBE_MAP_POSITIVE_X, ...) address the cube map
// bound as GL_TEXTURE_CUBE_MAP (SPEC §8.1). A face target must resolve to the
// bound cube map, and the texture's canonical target stays GL_TEXTURE_CUBE_MAP.
TEST_CASE("cube_face_texImage2D_resolves_bound_cubemap") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_CUBE_MAP, tex);

    const GLenum faces[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };
    for (GLenum face : faces) {
        ctx.texImage2D(face, 0, GL_RGBA, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        EXPECT_EQ(ctx.getError(), GLError::NoError);
    }

    // All six faces resolve to the cube map bound as GL_TEXTURE_CUBE_MAP and are
    // forwarded to the backend; the texture keeps its canonical cube-map target.
    TextureObject* t = ctx.getTexture(tex);
    EXPECT_EQ(t->target, GL_TEXTURE_CUBE_MAP);
    EXPECT_TRUE(t->storageSet);

    MockTexture* mt = as<MockTexture>(t->backend.get());
    EXPECT_EQ(mt->texImage2DCalls, 6);
    EXPECT_EQ(mt->lastTarget, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    EXPECT_EQ(mt->lastWidth, 16);
}

TEST_CASE("cube_face_texImage2D_without_bound_cubemap_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    // Bind as 2D only; no GL_TEXTURE_CUBE_MAP binding exists.
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    // Bind a synthetic name to the cube map binding so no valid texture is bound
    // for cube map face targets (SPEC §8.1).
    ctx.bindTexture(GL_TEXTURE_CUBE_MAP, 9999);

    ctx.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, 16, 16, GL_RGBA,
                   GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("non_cube_target_texImage2D_still_resolves") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);

    ctx.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 4, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getTexture(tex)->target, GL_TEXTURE_2D);

    MockTexture* mt = as<MockTexture>(ctx.getTexture(tex)->backend.get());
    EXPECT_EQ(mt->texImage2DCalls, 1);
    EXPECT_EQ(mt->lastTarget, GL_TEXTURE_2D);
}
