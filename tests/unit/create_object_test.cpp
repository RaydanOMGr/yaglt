#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

namespace {

// Exercise the DSA object-creation entry points (SPEC §6.1 / §7.4 / §8.2 / §13.2.1
// / §4): glCreate* mirrors glGen* but is the DSA-friendly variant that reserves
// usable names without a prior bind.

TEST_CASE("create_buffers_reserves_usable_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint b = 0;
    glCreateBuffers(1, &b);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(b, 0u);

    // The reserved buffer must be immediately bindable + uploadable.
    glBindBuffer(GL_ARRAY_BUFFER, b);
    glBufferData(GL_ARRAY_BUFFER, 32, nullptr, GL_STATIC_DRAW);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Distinct calls yield distinct names.
    GLuint b2 = 0;
    glCreateBuffers(1, &b2);
    EXPECT_NE(b2, b);

    setCurrentContext(nullptr);
}

TEST_CASE("create_samplers_reserves_usable_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint s = 0;
    glCreateSamplers(1, &s);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(s, 0u);
    EXPECT_EQ(glIsSampler(s), GL_TRUE);

    setCurrentContext(nullptr);
}

TEST_CASE("create_queries_valid_target_reserves_usable_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glCreateQueries(GL_SAMPLES_PASSED, 1, &q);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(q, 0u);
    EXPECT_EQ(glIsQuery(q), GL_TRUE);

    setCurrentContext(nullptr);
}

TEST_CASE("create_queries_invalid_target_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glCreateQueries(GL_TEXTURE_2D, 1, &q);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_EQ(q, 0u);

    setCurrentContext(nullptr);
}

TEST_CASE("create_queries_null_names_is_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glCreateQueries(GL_SAMPLES_PASSED, 1, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("create_program_pipelines_reserves_usable_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint p = 0;
    glCreateProgramPipelines(1, &p);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(p, 0u);
    EXPECT_EQ(glIsProgramPipeline(p), GL_TRUE);

    setCurrentContext(nullptr);
}

TEST_CASE("create_transform_feedbacks_reserves_usable_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint xfb = 0;
    glCreateTransformFeedbacks(1, &xfb);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(xfb, 0u);

    // The reserved TF object must be immediately bindable.
    glBindTransformFeedback(xfb);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("create_multiple_fills_distinct_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint names[4] = {0, 0, 0, 0};
    glCreateBuffers(4, names);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    for (uint32_t i = 0; i < 4; ++i) EXPECT_NE(names[i], 0u);
    EXPECT_NE(names[0], names[1]);
    EXPECT_NE(names[1], names[2]);
    EXPECT_NE(names[2], names[3]);

    setCurrentContext(nullptr);
}

} // namespace
