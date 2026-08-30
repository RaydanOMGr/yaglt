#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_factory.hpp"
#include "src/backend/mock/mock_resources.hpp"

using namespace glcompat;

namespace {

std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

} // namespace

// glCopyNamedBufferSubData copies a region between two named buffers (no bind
// required), mirrors it on the CPU, and pushes the written region to the
// destination backend (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_round_trip") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint src = 0, dst = 0;
    ctx.createBuffers(1, &src);
    ctx.createBuffers(1, &dst);
    ctx.namedBufferData(src, 16, GL_STATIC_DRAW, nullptr);
    ctx.namedBufferData(dst, 16, GL_STATIC_DRAW, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    const uint8_t pattern[16] = {1, 2, 3, 4, 5, 6, 7, 8,
                                 9, 10, 11, 12, 13, 14, 15, 16};
    ctx.namedBufferSubData(src, 0, 16, pattern);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    ctx.copyNamedBufferSubData(src, dst, 4, 8, 4); // src[4..8) -> dst[8..12)
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    uint8_t read[4] = {0};
    ctx.getNamedBufferSubData(dst, 8, 4, read);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(read[0], 5);
    EXPECT_EQ(read[3], 8);

    // The destination backend received the copied region. dst is the most
    // recently created buffer, so it is the factory's lastCreatedBuffer.
    auto& factory = static_cast<MockResourceFactory&>(backend->resourceFactory());
    MockBuffer* mb = factory.lastCreatedBuffer;
    EXPECT_TRUE(mb != nullptr);
    EXPECT_EQ(mb->namedBufferSubDataCalls, 1);
    EXPECT_EQ(mb->lastNamedSubOffset, 8);
    EXPECT_EQ(mb->lastNamedSubSize, 4);
}

// An ungenerated read or write name reports GL_INVALID_OPERATION (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_ungenerated_name_errors") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    ctx.namedBufferData(buf, 16, GL_STATIC_DRAW, nullptr);

    ctx.copyNamedBufferSubData(999u, buf, 0, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.copyNamedBufferSubData(buf, 999u, 0, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// An out-of-bounds copy region reports GL_INVALID_VALUE (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_out_of_bounds_errors") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint src = 0, dst = 0;
    ctx.createBuffers(1, &src);
    ctx.createBuffers(1, &dst);
    ctx.namedBufferData(src, 16, GL_STATIC_DRAW, nullptr);
    ctx.namedBufferData(dst, 16, GL_STATIC_DRAW, nullptr);

    // Read region past the source end.
    ctx.copyNamedBufferSubData(src, dst, 4, 0, 32);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // Write region past the destination end.
    ctx.copyNamedBufferSubData(src, dst, 0, 4, 32);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // Negative offset.
    ctx.copyNamedBufferSubData(src, dst, -1, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // Negative size.
    ctx.copyNamedBufferSubData(src, dst, 0, 0, -4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// The public C dispatch surface forwards to the frontend context (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_public_dispatch_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint src = 0, dst = 0;
    glCreateBuffers(1, &src);
    glCreateBuffers(1, &dst);
    glNamedBufferData(src, 16, nullptr, GL_STATIC_DRAW);
    glNamedBufferData(dst, 16, nullptr, GL_STATIC_DRAW);

    glCopyNamedBufferSubData(src, dst, 0, 0, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glcompat::setCurrentContext(nullptr);
}
