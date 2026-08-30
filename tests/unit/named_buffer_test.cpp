#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_factory.hpp"

using namespace glcompat;

namespace {

std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

} // namespace

// glNamedBufferData allocates storage on a named buffer (no bind required),
// mirrors it on the CPU, and forwards to the backend (SPEC §6.1).
TEST_CASE("named_buffer_data_allocates_and_mirrors") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    EXPECT_NE(buf, 0u);

    const uint8_t data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    ctx.namedBufferData(buf, 16, GL_STATIC_DRAW, data);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int64_t size = 0;
    ctx.getNamedBufferParameteri64v(buf, GL_BUFFER_SIZE, &size);
    EXPECT_EQ(size, 16);
    int32_t usage = 0;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_USAGE, &usage);
    EXPECT_EQ(static_cast<uint32_t>(usage), GL_STATIC_DRAW);

    auto& factory = static_cast<MockResourceFactory&>(backend->resourceFactory());
    MockBuffer* mb = factory.lastCreatedBuffer;
    EXPECT_TRUE(mb != nullptr);
    EXPECT_TRUE(mb->namedBufferDataCalls >= 1);
    EXPECT_EQ(mb->lastNamedSize, 16);
    EXPECT_EQ(mb->lastNamedUsage, GL_STATIC_DRAW);
    EXPECT_TRUE(mb->lastNamedHadData);
}

// glNamedBufferSubData updates an in-bounds region of a named buffer and is
// rejected (GL_INVALID_VALUE) when the region exceeds the allocation (SPEC §6).
TEST_CASE("named_buffer_sub_data_updates_in_bounds") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    ctx.namedBufferData(buf, 16, GL_DYNAMIC_DRAW, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    const uint8_t patch[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    ctx.namedBufferSubData(buf, 4, 4, patch);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    uint8_t read[4] = {0};
    ctx.getNamedBufferSubData(buf, 4, 4, read);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(read[0], 0xAA);
    EXPECT_EQ(read[3], 0xDD);

    // Region past the end of the allocation is invalid.
    const uint8_t over[4] = {0};
    ctx.namedBufferSubData(buf, 4, 16, over); // 4 + 16 > 16
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// glNamedBufferStorage allocates immutable storage: re-allocation of an already
// immutable buffer and a non-positive size are errors (SPEC §6).
TEST_CASE("named_buffer_storage_immutable") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);

    const uint8_t data[8] = {0};
    ctx.namedBufferStorage(buf, 8, data, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    int32_t immutable = 0;
    ctx.getNamedBufferParameteriv(buf, GL_BUFFER_IMMUTABLE_STORAGE, &immutable);
    EXPECT_EQ(immutable, GL_TRUE);

    // Re-allocating immutable storage fails.
    ctx.namedBufferData(buf, 8, GL_STATIC_DRAW, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.namedBufferStorage(buf, 8, data, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Non-positive size is invalid (checked on a fresh buffer, since an already
    // immutable buffer reports INVALID_OPERATION on any re-allocation first).
    GLuint buf2 = 0;
    ctx.createBuffers(1, &buf2);
    ctx.namedBufferStorage(buf2, 0, nullptr, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// A named buffer operation on an ungenerated name reports GL_INVALID_OPERATION
// (SPEC §6.1).
TEST_CASE("named_buffer_ungenerated_name_errors") {
    auto backend = makeBackend();
    Context ctx(*backend);
    const uint8_t data[4] = {0};

    ctx.namedBufferData(999u, 4, GL_STATIC_DRAW, data);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.namedBufferSubData(999u, 0, 4, data);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.namedBufferStorage(999u, 4, data, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// The public C dispatch surface forwards to the frontend context.
TEST_CASE("named_buffer_public_dispatch_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    const uint8_t data[8] = {0};
    glNamedBufferData(buf, 8, data, GL_STATIC_DRAW);
    glNamedBufferSubData(buf, 0, 4, data);
    glNamedBufferStorage(buf, 8, data, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glcompat::setCurrentContext(nullptr);
}
