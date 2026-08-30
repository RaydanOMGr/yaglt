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

// glMapNamedBuffer maps a named buffer and returns a pointer into the frontend
// CPU mirror; writes through it reach the store and the backend records the call
// (SPEC §6.1).
TEST_CASE("map_named_buffer_returns_mirror_pointer") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    ctx.namedBufferData(buf, 16, GL_STATIC_DRAW, nullptr);

    void* p = ctx.mapNamedBuffer(buf, GL_READ_WRITE);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* obj = ctx.getBuffer(buf);
    EXPECT_TRUE(obj->mapped);

    auto& factory = static_cast<MockResourceFactory&>(backend->resourceFactory());
    EXPECT_EQ(factory.lastCreatedBuffer->mapNamedBufferRangeCalls, 1);

    // Writes through the mapped pointer reach the frontend store.
    *reinterpret_cast<int*>(p) = 0x12345678;
    EXPECT_EQ(*reinterpret_cast<int*>(obj->store.data()), 0x12345678);

    ctx.unmapNamedBuffer(buf);
    EXPECT_FALSE(obj->mapped);
    EXPECT_EQ(factory.lastCreatedBuffer->unmapNamedBufferCalls, 1);
}

// glMapNamedBufferRange maps a sub-region; the mapped pointer is offset into the
// store (SPEC §6.1).
TEST_CASE("map_named_buffer_range_offsets_into_store") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    ctx.namedBufferData(buf, 16, GL_STATIC_DRAW, nullptr);

    void* p = ctx.mapNamedBufferRange(buf, 4, 8, GL_READ_WRITE);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* obj = ctx.getBuffer(buf);
    EXPECT_EQ(obj->mapOffset, 4);
    EXPECT_EQ(obj->mapLength, 8);
    EXPECT_EQ(static_cast<uint8_t*>(p) - obj->store.data(), 4);

    ctx.unmapNamedBuffer(buf);
    EXPECT_FALSE(obj->mapped);
}

// An out-of-bounds map region is rejected (GL_INVALID_VALUE) and a second map of
// an already-mapped buffer is rejected (GL_INVALID_OPERATION) (SPEC §6).
TEST_CASE("map_named_buffer_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    ctx.namedBufferData(buf, 16, GL_STATIC_DRAW, nullptr);

    EXPECT_EQ(ctx.mapNamedBufferRange(buf, 4, 32, GL_READ_WRITE), nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    EXPECT_NE(ctx.mapNamedBuffer(buf, GL_READ_WRITE), nullptr);
    EXPECT_EQ(ctx.mapNamedBuffer(buf, GL_READ_WRITE), nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.unmapNamedBuffer(buf);
}

// Mapping an ungenerated name reports GL_INVALID_OPERATION (SPEC §6.1).
TEST_CASE("map_named_buffer_ungenerated_name_errors") {
    auto backend = makeBackend();
    Context ctx(*backend);
    EXPECT_EQ(ctx.mapNamedBuffer(999u, GL_READ_WRITE), nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_FALSE(ctx.unmapNamedBuffer(999u));
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// glFlushMappedNamedBufferRange records the flushed region and pushes the CPU
// mirror back through the backend (SPEC §6.1). Unmapped flush is an error.
TEST_CASE("flush_mapped_named_buffer_range_pushes_region") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.createBuffers(1, &buf);
    ctx.namedBufferData(buf, 16, GL_STATIC_DRAW, nullptr);

    EXPECT_NE(ctx.mapNamedBuffer(buf, GL_READ_WRITE), nullptr);
    auto& factory = static_cast<MockResourceFactory&>(backend->resourceFactory());
    auto* mb = factory.lastCreatedBuffer;

    auto* obj = ctx.getBuffer(buf);
    *reinterpret_cast<int*>(obj->store.data() + 4) = 0xDEADBEEF;

    ctx.flushMappedNamedBufferRange(buf, 4, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(mb->flushMappedNamedBufferRangeCalls, 1);
    EXPECT_EQ(mb->lastNamedFlushOffset, 4);
    EXPECT_EQ(mb->lastNamedFlushLength, 4);

    // An ungenerated name / not-mapped flush is rejected.
    ctx.unmapNamedBuffer(buf);
    ctx.flushMappedNamedBufferRange(buf, 0, 4);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// The missing target-based glFlushMappedBufferRange (SPEC §6) flushes the bound
// mapped buffer's region and records the call.
TEST_CASE("flush_mapped_buffer_range_target_based") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_STATIC_DRAW, nullptr);

    EXPECT_NE(ctx.mapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE), nullptr);
    auto& factory = static_cast<MockResourceFactory&>(backend->resourceFactory());
    auto* mb = factory.lastCreatedBuffer;

    ctx.flushMappedBufferRange(GL_ARRAY_BUFFER, 0, 8);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(mb->flushMappedBufferRangeCalls, 1);
    EXPECT_EQ(mb->lastFlushOffset, 0);
    EXPECT_EQ(mb->lastFlushLength, 8);

    ctx.unmapBuffer(GL_ARRAY_BUFFER);
    ctx.flushMappedBufferRange(GL_ARRAY_BUFFER, 0, 8);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// The public C dispatch surface forwards to the frontend context (SPEC §6.1).
TEST_CASE("named_buffer_map_public_dispatch_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    glNamedBufferData(buf, 16, nullptr, GL_STATIC_DRAW);

    EXPECT_NE(glMapNamedBufferRange(buf, 0, 8, GL_READ_WRITE), nullptr);
    glFlushMappedNamedBufferRange(buf, 0, 8);
    EXPECT_EQ(glUnmapNamedBuffer(buf), GL_TRUE);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glcompat::setCurrentContext(nullptr);
}
