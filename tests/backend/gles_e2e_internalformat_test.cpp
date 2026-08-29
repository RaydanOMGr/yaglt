#include "test_framework.hpp"

#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/backend/gles/gles_backend.hpp"

#include <cstdint>
#include <string>

// End-to-end: internal format queries reach the real GLES driver (Mesa softpipe
// in CI). Skips cleanly when no driver is available.
TEST_CASE("gles_e2e_internalformat_query_via_driver") {
    glcompat::GLESBackend backend;
    if (!backend.initialize()) {
        return;
    }

    glcompat::Context ctx(backend);
    glcompat::setCurrentContext(&ctx);

    // NUM_SAMPLE_COUNTS must be answered by the driver without error and be >= 0.
    GLint numSamples = -1;
    glcompat::glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS,
                                    1, &numSamples);
    EXPECT_EQ(ctx.getError(), glcompat::GLError::NoError);
    EXPECT_TRUE(numSamples >= 0);

    // The 64-bit variant widens from the driver's iv query.
    GLint64 numSamples64 = -1;
    glcompat::glGetInternalformati64v(GL_RENDERBUFFER, GL_RGBA8,
                                     GL_NUM_SAMPLE_COUNTS, 1, &numSamples64);
    EXPECT_EQ(ctx.getError(), glcompat::GLError::NoError);
    EXPECT_TRUE(numSamples64 >= 0);

    // Backing-driver string query reaches the real GLES driver (not the frontend
    // synthetic values) and returns a non-empty driver string for the four names.
    const char* vendor = glcompat::yagltGetBackingGlString(GL_VENDOR);
    const char* renderer = glcompat::yagltGetBackingGlString(GL_RENDERER);
    const char* version = glcompat::yagltGetBackingGlString(GL_VERSION);
    const char* slVersion =
        glcompat::yagltGetBackingGlString(GL_SHADING_LANGUAGE_VERSION);
    EXPECT_NE(vendor, nullptr);
    EXPECT_NE(renderer, nullptr);
    EXPECT_NE(version, nullptr);
    EXPECT_NE(slVersion, nullptr);
    EXPECT_TRUE(std::string(version).find("OpenGL ES") != std::string::npos);
    EXPECT_EQ(glcompat::yagltGetBackingGlString(0xDEAD), nullptr);

    glcompat::setCurrentContext(nullptr);
}
