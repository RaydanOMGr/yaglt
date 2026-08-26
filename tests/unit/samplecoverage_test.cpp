#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glSampleCoverage_pushes_only_on_change_and_records_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default coverage is (1.0, invert=false); a different value pushes.
    glSampleCoverage(0.5f, GL_FALSE);
    glFlushState();
    EXPECT_EQ(backend.sampleCoverageCalls, 1);
    EXPECT_EQ(backend.lastSampleCoverageValue, 0.5f);
    EXPECT_EQ(backend.lastSampleCoverageInvert, false);

    // Same value again must not re-push (SPEC §10).
    glSampleCoverage(0.5f, GL_FALSE);
    glFlushState();
    EXPECT_EQ(backend.sampleCoverageCalls, 1);

    // Changed value or invert flag pushes again.
    glSampleCoverage(0.5f, GL_TRUE);
    glFlushState();
    EXPECT_EQ(backend.sampleCoverageCalls, 2);
    EXPECT_EQ(backend.lastSampleCoverageInvert, true);

    setCurrentContext(nullptr);
}

TEST_CASE("glGetSampleCoverage_returns_tracked_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glSampleCoverage(0.25f, GL_TRUE);

    GLfloat fv = 0.0f;
    glGetFloatv(GL_SAMPLE_COVERAGE_VALUE, &fv);
    EXPECT_EQ(fv, 0.25f);

    GLboolean bv = GL_FALSE;
    glGetBooleanv(GL_SAMPLE_COVERAGE_INVERT, &bv);
    EXPECT_EQ(bv, GL_TRUE);

    setCurrentContext(nullptr);
}
