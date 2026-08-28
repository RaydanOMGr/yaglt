#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
// A minimal compute program is needed only to satisfy the active-program gate;
// the mock records the dispatch without a driver.
} // namespace

TEST_CASE("compute_dispatch_requires_feature_and_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Without ComputeShaders support, dispatch is rejected honestly.
    EXPECT_EQ(backend.capabilities().isSupported(Feature::ComputeShaders), false);
    glDispatchCompute(1, 1, 1);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_OPERATION));
    EXPECT_EQ(backend.dispatchComputeCalls, 0);

    // Enable compute support (the mock records the call without a driver).
    backend.setCapability(Feature::ComputeShaders, FeatureSupport::Emulated);
    EXPECT_EQ(backend.capabilities().isSupported(Feature::ComputeShaders), true);

    // Still requires an active program.
    glDispatchCompute(4, 2, 1);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_OPERATION));
    EXPECT_EQ(backend.dispatchComputeCalls, 0);

    // With a program in use, the dispatch is issued and recorded.
    glUseProgram(7);
    glDispatchCompute(4, 2, 1);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
    EXPECT_EQ(backend.dispatchComputeCalls, 1);
    EXPECT_EQ(backend.lastDispatchX, 4u);
    EXPECT_EQ(backend.lastDispatchY, 2u);
    EXPECT_EQ(backend.lastDispatchZ, 1u);

    setCurrentContext(nullptr);
}

TEST_CASE("compute_dispatch_indirect_requires_indirect_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);
    backend.setCapability(Feature::ComputeShaders, FeatureSupport::Emulated);
    glUseProgram(7);

    // No GL_DISPATCH_INDIRECT_BUFFER bound → INVALID_OPERATION.
    glDispatchComputeIndirect(reinterpret_cast<const void*>(0));
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_OPERATION));
    EXPECT_EQ(backend.dispatchComputeIndirectCalls, 0);

    // Bind a buffer to the dispatch indirect target, then dispatch reads the
    // byte offset from it.
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buf);
    glDispatchComputeIndirect(reinterpret_cast<const void*>(24));
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
    EXPECT_EQ(backend.dispatchComputeIndirectCalls, 1);
    EXPECT_EQ(backend.lastDispatchIndirect, static_cast<uintptr_t>(24));

    setCurrentContext(nullptr);
}

TEST_CASE("compute_program_stage_is_gated_by_feature") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // ComputeShaders unsupported in the default mock profile → object rejected.
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    EXPECT_EQ(cs, 0u);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_INVALID_OPERATION));

    setCurrentContext(nullptr);
}
