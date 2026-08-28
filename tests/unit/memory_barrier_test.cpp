#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
// Barrier bits (SPEC §7.13.2). Names not all present in gl_types.hpp, so use the
// spec values directly for the test.
constexpr uint32_t kVertexAttribArrayBarrierBit = 0x00000001u;
constexpr uint32_t kElementArrayBarrierBit = 0x00000010u;
constexpr uint32_t kShaderImageAccessBarrierBit = 0x00000020u;
} // namespace

TEST_CASE("memorybarrier_delegates_to_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glMemoryBarrier(kVertexAttribArrayBarrierBit | kShaderImageAccessBarrierBit);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.memoryBarrierCalls, 1);
    EXPECT_EQ(backend.lastBarriers, kVertexAttribArrayBarrierBit | kShaderImageAccessBarrierBit);

    glMemoryBarrierByRegion(kElementArrayBarrierBit);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.memoryBarrierByRegionCalls, 1);
    EXPECT_EQ(backend.lastBarriersByRegion, kElementArrayBarrierBit);

    setCurrentContext(nullptr);
}
