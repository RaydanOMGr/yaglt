#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

namespace {

std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

} // namespace

// MockBackend is a complete GLStateSink, so we can use it to observe the
// point-parameter push (SPEC §10: only changed state is pushed).
TEST_CASE("point_param_pushes_to_backend_once") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.pointParameteri(GL_POINT_SIZE_MIN, 2);
    ctx.flushState();
    int afterSet = backend->pointParametersCalls;
    EXPECT_TRUE(afterSet >= 1);
    EXPECT_TRUE(backend->lastPointSizeMin == 2.0f);

    // A second flush with no change must not push again.
    ctx.flushState();
    EXPECT_EQ(backend->pointParametersCalls, afterSet);
}

TEST_CASE("point_param_pushes_all_fields") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.pointParameteri(GL_POINT_SIZE_MIN, 2);
    ctx.pointParameteri(GL_POINT_SIZE_MAX, 4);
    ctx.pointParameteri(GL_POINT_FADE_THRESHOLD_SIZE, 1);
    ctx.pointParameteri(GL_POINT_SPRITE_COORD_ORIGIN,
                        static_cast<GLint>(GL_LOWER_LEFT));
    ctx.flushState();
    EXPECT_TRUE(backend->lastPointSizeMin == 2.0f);
    EXPECT_TRUE(backend->lastPointSizeMax == 4.0f);
    EXPECT_TRUE(backend->lastPointFadeThreshold == 1.0f);
    EXPECT_EQ(backend->lastSpriteCoordOrigin, GL_LOWER_LEFT);
}

TEST_CASE("point_param_negative_value_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.pointParameteri(GL_POINT_SIZE_MIN, -1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.pointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, -0.5f);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("point_param_bad_origin_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.pointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, 0xDEAD);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("point_param_unknown_pname_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.pointParameteri(0x1234, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("point_param_float_and_vector_variants") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLfloat fmax = 5.0f;
    GLint ifade = 1;
    ctx.pointParameterfv(GL_POINT_SIZE_MAX, &fmax);
    ctx.pointParameteriv(GL_POINT_FADE_THRESHOLD_SIZE, &ifade);
    ctx.flushState();
    EXPECT_TRUE(backend->lastPointSizeMax == 5.0f);
    EXPECT_TRUE(backend->lastPointFadeThreshold == 1.0f);
}

TEST_CASE("point_param_gl_get_returns_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    glPointParameteri(GL_POINT_SIZE_MIN, 3);
    GLint out = 0;
    glGetIntegerv(GL_POINT_SIZE_MIN, &out);
    EXPECT_EQ(out, 3);
    GLfloat fout = 0.0f;
    glGetFloatv(GL_POINT_SIZE_MIN, &fout);
    EXPECT_TRUE(fout == 3.0f);
    setCurrentContext(nullptr);
}

TEST_CASE("gl_point_parameter_null_context_no_crash") {
    setCurrentContext(nullptr);
    glPointParameteri(GL_POINT_SIZE_MIN, 1);
    glPointParameterf(GL_POINT_SIZE_MAX, 1.0f);
    glPointParameteriv(GL_POINT_SIZE_MIN, nullptr);
    glPointParameterfv(GL_POINT_SIZE_MAX, nullptr);
    setCurrentContext(nullptr);
}
