#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_BOGUS_STAGE = 0xDEAD;
} // namespace

TEST_CASE("shader_stage_vertex_and_fragment_create_succeed") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(vs, 0u);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(fs, 0u);

    setCurrentContext(nullptr);
}

TEST_CASE("shader_stage_unsupported_reported_honestly") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Geometry / tessellation stages have no GLES equivalent and are reported
    // Unsupported in the mock profile. Compute is native in GLES 3.1+ and
    // supported. Creating the unsupported stages must fail honestly with
    // GL_INVALID_OPERATION and return name 0 (no fake success).
    GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(gs, 0u);

    GLuint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(tcs, 0u);

    GLuint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(tes, 0u);

    setCurrentContext(nullptr);
}

TEST_CASE("shader_stage_unknown_type_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint bad = glCreateShader(GL_BOGUS_STAGE);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_EQ(bad, 0u);

    setCurrentContext(nullptr);
}
