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

// glCopyNamedBufferSubData copies a region between two named buffers and pushes
// the written region to the destination backend (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_copies_and_pushes") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint src = 0, dst = 0;
    ctx.createBuffers(1, &src);
    ctx.createBuffers(1, &dst);
    const uint8_t sd[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    ctx.namedBufferData(src, 16, GL_STATIC_DRAW, sd);
    ctx.namedBufferData(dst, 16, GL_STATIC_DRAW, nullptr);

    ctx.copyNamedBufferSubData(src, dst, 4, 8, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* d = ctx.getBuffer(dst);
    EXPECT_EQ(d->store[8], 5);
    EXPECT_EQ(d->store[11], 8);

    auto& factory = static_cast<MockResourceFactory&>(backend->resourceFactory());
    auto* mb = factory.lastCreatedBuffer; // most recently created is dst
    EXPECT_EQ(mb->namedBufferSubDataCalls, 1);
    EXPECT_EQ(mb->lastNamedSubOffset, 8);
    EXPECT_EQ(mb->lastNamedSubSize, 4);
}

// An out-of-bounds copy region is rejected (GL_INVALID_VALUE); an ungenerated
// name is rejected (GL_INVALID_OPERATION) (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint src = 0, dst = 0;
    ctx.createBuffers(1, &src);
    ctx.createBuffers(1, &dst);
    ctx.namedBufferData(src, 16, GL_STATIC_DRAW, nullptr);
    ctx.namedBufferData(dst, 16, GL_STATIC_DRAW, nullptr);

    ctx.copyNamedBufferSubData(src, dst, 4, 8, 32); // 8 + 32 > 16
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    ctx.copyNamedBufferSubData(src, 999u, 0, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.copyNamedBufferSubData(999u, dst, 0, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// The public C dispatch surface forwards to the frontend context (SPEC §6).
TEST_CASE("copy_named_buffer_sub_data_public_dispatch_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint src = 0, dst = 0;
    glCreateBuffers(1, &src);
    glCreateBuffers(1, &dst);
    const uint8_t sd[8] = {0};
    glNamedBufferData(src, 8, sd, GL_STATIC_DRAW);
    glNamedBufferData(dst, 8, sd, GL_STATIC_DRAW);
    glCopyNamedBufferSubData(src, dst, 0, 0, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glcompat::setCurrentContext(nullptr);
}
