#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("buffer_subdata_updates_region") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 8, GL_STATIC_DRAW, nullptr);

    int data[2] = {0x11223344, 0x55667788};
    ctx.bufferSubData(GL_ARRAY_BUFFER, 0, 8, data);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* obj = ctx.getBuffer(buf);
    EXPECT_EQ(obj->store.size(), 8u);
    EXPECT_EQ(*reinterpret_cast<int*>(obj->store.data()), 0x11223344);
    EXPECT_EQ(*reinterpret_cast<int*>(obj->store.data() + 4), 0x55667788);
    EXPECT_EQ(backend->bindBufferBaseCalls, 0);
}

TEST_CASE("buffer_subdata_out_of_bounds_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 4, GL_STATIC_DRAW, nullptr);

    int v = 1;
    ctx.bufferSubData(GL_ARRAY_BUFFER, 2, 4, &v); // 2+4 > 4
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("buffer_subdata_no_bound_buffer_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int v = 1;
    ctx.bufferSubData(GL_ARRAY_BUFFER, 0, 4, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("buffer_storage_creates_immutable_store") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);

    int data[2] = {7, 8};
    ctx.bufferStorage(GL_ARRAY_BUFFER, 8, data,
                      GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* obj = ctx.getBuffer(buf);
    EXPECT_TRUE(obj->immutable);
    EXPECT_EQ(obj->size, 8);
    EXPECT_EQ(obj->immutableFlags,
              static_cast<uint32_t>(GL_MAP_WRITE_BIT | GL_MAP_READ_BIT));
    EXPECT_EQ(*reinterpret_cast<int*>(obj->store.data()), 7);

    // A second allocation must be rejected (immutable).
    ctx.bufferStorage(GL_ARRAY_BUFFER, 16, nullptr, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("buffer_storage_unsupported_reports_invalid_operation") {
    auto backend = makeBackend();
    backend->setCapability(Feature::ImmutableBufferStorage,
                           FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferStorage(GL_ARRAY_BUFFER, 8, nullptr, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("copy_buffer_subdata_copies_between_buffers") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint src = 0, dst = 0;
    ctx.genBuffers(1, &src);
    ctx.genBuffers(1, &dst);
    ctx.bindBuffer(GL_COPY_READ_BUFFER, src);
    ctx.bindBuffer(GL_COPY_WRITE_BUFFER, dst);
    ctx.bufferData(GL_COPY_READ_BUFFER, 8, GL_STATIC_DRAW, nullptr);
    ctx.bufferData(GL_COPY_WRITE_BUFFER, 8, GL_STATIC_DRAW, nullptr);
    int v[2] = {11, 22};
    ctx.bufferSubData(GL_COPY_READ_BUFFER, 0, 8, v);

    ctx.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 8);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* d = ctx.getBuffer(dst);
    EXPECT_EQ(*reinterpret_cast<int*>(d->store.data()), 11);
    EXPECT_EQ(*reinterpret_cast<int*>(d->store.data() + 4), 22);
}

TEST_CASE("copy_buffer_subdata_oob_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint src = 0, dst = 0;
    ctx.genBuffers(1, &src);
    ctx.genBuffers(1, &dst);
    ctx.bindBuffer(GL_COPY_READ_BUFFER, src);
    ctx.bindBuffer(GL_COPY_WRITE_BUFFER, dst);
    ctx.bufferData(GL_COPY_READ_BUFFER, 4, GL_STATIC_DRAW, nullptr);
    ctx.bufferData(GL_COPY_WRITE_BUFFER, 4, GL_STATIC_DRAW, nullptr);

    ctx.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 8);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("get_buffer_parameter_reports_size_usage_immutable") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 16, GL_DYNAMIC_DRAW, nullptr);

    GLint sz = -1, us = -1, imm = -1, mapped = -1;
    ctx.getBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &sz);
    ctx.getBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &us);
    ctx.getBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_IMMUTABLE_STORAGE, &imm);
    ctx.getBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(sz, 16);
    EXPECT_EQ(us, static_cast<GLint>(GL_DYNAMIC_DRAW));
    EXPECT_EQ(imm, GL_FALSE);
    EXPECT_EQ(mapped, GL_FALSE);

    // Unknown pname -> GL_INVALID_ENUM.
    GLint dummy = 0;
    ctx.getBufferParameteriv(GL_ARRAY_BUFFER, 0xDEAD, &dummy);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    setCurrentContext(nullptr);
}

TEST_CASE("map_unmap_buffer_returns_cpu_mirror") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 8, GL_STATIC_DRAW, nullptr);
    int v[2] = {0xAB, 0xCD};
    ctx.bufferSubData(GL_ARRAY_BUFFER, 0, 8, v);

    void* p = ctx.mapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* obj = ctx.getBuffer(buf);
    EXPECT_TRUE(obj->mapped);

    // Writes through the mapped pointer reach the frontend store.
    *reinterpret_cast<int*>(p) = 0x12345678;
    EXPECT_EQ(*reinterpret_cast<int*>(obj->store.data()), 0x12345678);

    EXPECT_TRUE(ctx.unmapBuffer(GL_ARRAY_BUFFER));
    EXPECT_FALSE(obj->mapped);
}

TEST_CASE("map_buffer_already_mapped_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 8, GL_STATIC_DRAW, nullptr);
    EXPECT_NE(ctx.mapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE), nullptr);
    EXPECT_EQ(ctx.mapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE), nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.unmapBuffer(GL_ARRAY_BUFFER);
}

TEST_CASE("unmap_buffer_not_mapped_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 8, GL_STATIC_DRAW, nullptr);
    EXPECT_FALSE(ctx.unmapBuffer(GL_ARRAY_BUFFER));
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("buffer_surface_entry_points_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);
    GLuint buf = 0;
    ctx.genBuffers(1, &buf);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.bufferData(GL_ARRAY_BUFFER, 8, GL_STATIC_DRAW, nullptr);

    int v = 5;
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(*reinterpret_cast<int*>(ctx.getBuffer(buf)->store.data()), 5);

    GLint sz = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &sz);
    EXPECT_EQ(sz, 8);

    void* p = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(glUnmapBuffer(GL_ARRAY_BUFFER));
    setCurrentContext(nullptr);
}
