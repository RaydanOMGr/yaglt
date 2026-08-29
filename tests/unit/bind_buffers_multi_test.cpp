#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

// Multi-bind indexed buffer bindings (SPEC §6.1.1 / ARB_multi_bind):
// glBindBuffersBase / glBindBuffersRange.

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("bind_buffers_base_binds_consecutive_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName bufs[3] = {ctx.genBuffer(), ctx.genBuffer(), ctx.genBuffer()};

    ctx.bindBuffersBase(GL_UNIFORM_BUFFER, 1, 3, bufs);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    // One native bindBufferBase push per binding point.
    EXPECT_EQ(backend->bindBufferBaseCalls, 3);
    EXPECT_EQ(backend->lastBindTarget, GL_UNIFORM_BUFFER);
    EXPECT_EQ(backend->lastBindIndex, 3u);
    EXPECT_EQ(backend->lastBindBuffer, bufs[2]);
}

TEST_CASE("bind_buffers_base_null_array_resets_range") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName bufs[2] = {ctx.genBuffer(), ctx.genBuffer()};
    ctx.bindBuffersBase(GL_UNIFORM_BUFFER, 0, 2, bufs);
    EXPECT_EQ(backend->bindBufferBaseCalls, 2);

    // SPEC §6.1.1: a null array resets each touched point to the unbound state.
    ctx.bindBuffersBase(GL_UNIFORM_BUFFER, 0, 2, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->bindBufferBaseCalls, 4);
    EXPECT_EQ(backend->lastBindBuffer, 0u);
}

TEST_CASE("bind_buffers_range_forwards_offsets_and_sizes") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName bufs[2] = {ctx.genBuffer(), ctx.genBuffer()};
    intptr_t offsets[2] = {0, 64};
    intptr_t sizes[2] = {32, 32};

    ctx.bindBuffersRange(GL_SHADER_STORAGE_BUFFER, 0, 2, bufs, offsets, sizes);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->bindBufferRangeCalls, 2);
    EXPECT_EQ(backend->lastBindTarget, GL_SHADER_STORAGE_BUFFER);
    EXPECT_EQ(backend->lastBindIndex, 1u);
    EXPECT_EQ(backend->lastBindBuffer, bufs[1]);
}

TEST_CASE("bind_buffers_range_null_array_resets_range") {
    auto backend = makeBackend();
    Context ctx(*backend);
    // offsets/sizes are ignored when `buffers` is null (SPEC §6.1.1).
    ctx.bindBuffersRange(GL_UNIFORM_BUFFER, 0, 2, nullptr, nullptr, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->bindBufferRangeCalls, 2);
    EXPECT_EQ(backend->lastBindBuffer, 0u);
}

TEST_CASE("bind_buffers_bad_target_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffersBase(0xDEAD, 0, 1, &buf);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    EXPECT_EQ(backend->bindBufferBaseCalls, 0);
}

TEST_CASE("bind_buffers_negative_count_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindBuffersBase(GL_UNIFORM_BUFFER, 0, -1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    EXPECT_EQ(backend->bindBufferBaseCalls, 0);
}

TEST_CASE("bind_buffers_range_overflow_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    // SPEC §6.1.1: first + count past the indexed binding-point count.
    ctx.bindBuffersBase(GL_UNIFORM_BUFFER, 15, 4, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend->bindBufferBaseCalls, 0);
}

TEST_CASE("bind_buffers_base_validates_entries_per_binding_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName good = ctx.genBuffer();
    GLObjectName bufs[3] = {good, 4242 /* ungenerated */, good};

    ctx.bindBuffersBase(GL_UNIFORM_BUFFER, 0, 3, bufs);
    // SPEC §6.1.1: the invalid entry reports an error per binding point...
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    // ...while the two valid entries are still bound (2 native pushes, not 3).
    EXPECT_EQ(backend->bindBufferBaseCalls, 2);
    EXPECT_EQ(backend->lastBindIndex, 2u);
}

TEST_CASE("bind_buffers_range_validates_offsets_and_sizes_per_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName good = ctx.genBuffer();
    GLObjectName bufs[3] = {good, good, good};
    intptr_t offsets[3] = {0, -8 /* negative */, 16};
    intptr_t sizes[3] = {16, 16, 0 /* non-positive with a real buffer */};

    ctx.bindBuffersRange(GL_UNIFORM_BUFFER, 0, 3, bufs, offsets, sizes);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // Only binding point 0 was legal.
    EXPECT_EQ(backend->bindBufferRangeCalls, 1);
    EXPECT_EQ(backend->lastBindIndex, 0u);
}

TEST_CASE("bind_buffer_range_single_validates_offset_and_size") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();

    // SPEC §6.1.1: negative offset, or a non-positive size with a non-zero
    // buffer, are GL_INVALID_VALUE (the single-bind form shares the check).
    ctx.bindBufferRange(GL_UNIFORM_BUFFER, 0, buf, -4, 16);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.bindBufferRange(GL_UNIFORM_BUFFER, 0, buf, 0, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    EXPECT_EQ(backend->bindBufferRangeCalls, 0);
    // Index beyond the tracked binding points is GL_INVALID_VALUE.
    ctx.bindBufferBase(GL_UNIFORM_BUFFER, 16, buf);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    EXPECT_EQ(backend->bindBufferBaseCalls, 0);
}

TEST_CASE("bind_buffers_unsupported_target_is_invalid_operation") {
    auto backend = makeBackend();
    backend->setCapability(Feature::ShaderStorageBufferObjects,
                           FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    ctx.bindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, &buf);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(backend->bindBufferBaseCalls, 0);
}

TEST_CASE("gl_api_bind_buffers_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint bufs[2] = {0, 0};
    glGenBuffers(2, bufs);
    glBindBuffersBase(GL_UNIFORM_BUFFER, 0, 2, bufs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend->bindBufferBaseCalls, 2);

    GLintptr offsets[2] = {0, 32};
    GLsizeiptr sizes[2] = {16, 16};
    glBindBuffersRange(GL_UNIFORM_BUFFER, 0, 2, bufs, offsets, sizes);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend->bindBufferRangeCalls, 2);

    setCurrentContext(nullptr);
}
