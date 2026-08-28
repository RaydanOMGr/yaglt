#include "test_framework.hpp"

#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/backend/gles/gles_backend.hpp"

#include <cstdint>
#include <string>

// End-to-end: 1D texture emulation on GLES backend (stored as 2D with height=1).
// Skips cleanly when no driver is available.
TEST_CASE("gles_e2e_1d_texture_emulated_as_2d") {
    glcompat::GLESBackend backend;
    if (!backend.initialize()) {
        return;
    }

    glcompat::Context ctx(backend);
    glcompat::setCurrentContext(&ctx);

    glcompat::GLuint tex = ctx.genTexture();
    ctx.bindTexture(glcompat::GL_TEXTURE_1D, tex);
    ctx.texImage1D(glcompat::GL_TEXTURE_1D, 0, GL_RGBA, 16, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(ctx.getError(), glcompat::GLError::NoError);

    GLint w = 0;
    ctx.getTextureLevelParameteriv(tex, 0, GL_TEXTURE_WIDTH, &w);
    EXPECT_EQ(w, 16);
    EXPECT_EQ(ctx.getError(), glcompat::GLError::NoError);
}
