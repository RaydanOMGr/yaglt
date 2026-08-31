#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

// Non-DSA separate attribute format (SPEC §10.3.2/§10.3.4,
// ARB_vertex_attrib_binding): glBindVertexBuffer(s), glVertexAttrib*Format,
// glVertexAttribBinding and glVertexBindingDivisor all act on the VAO bound to
// GL_VERTEX_ARRAY_BINDING.

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

TEST_CASE("bind_vertex_buffer_records_binding_on_bound_vao") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();
    ctx.bindVertexArray(vao);

    ctx.bindVertexBuffer(1, buf, 16, 32);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto& b = ctx.getVertexArray(vao)->bindings[1];
    EXPECT_EQ(b.buffer, buf);
    EXPECT_EQ(b.offset, 16);
    EXPECT_EQ(b.stride, 32);
}

TEST_CASE("bind_vertex_buffer_with_default_vao_succeeds") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName buf = ctx.genBuffer();
    // Default VAO (name 0) is always present in the compat profile (SPEC §10.3.2).
    ctx.bindVertexBuffer(0, buf, 0, 12);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
}

TEST_CASE("bind_vertex_buffer_zero_detaches_buffer") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();
    ctx.bindVertexArray(vao);
    ctx.bindVertexBuffer(0, buf, 0, 12);
    ctx.bindVertexBuffer(0, 0, 0, 12);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getVertexArray(vao)->bindings[0].buffer, 0u);
}

TEST_CASE("bind_vertex_buffer_validates_index_offset_and_stride") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();
    ctx.bindVertexArray(vao);

    // bindingindex >= MAX_VERTEX_ATTRIB_BINDINGS -> GL_INVALID_VALUE.
    ctx.bindVertexBuffer(16, buf, 0, 12);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // Negative offset / stride -> GL_INVALID_VALUE.
    ctx.bindVertexBuffer(0, buf, -4, 12);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.bindVertexBuffer(0, buf, 0, -1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // stride > MAX_VERTEX_ATTRIB_STRIDE -> GL_INVALID_VALUE.
    ctx.bindVertexBuffer(0, buf, 0, 4096);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    // The rejected calls left the binding point untouched.
    EXPECT_EQ(ctx.getVertexArray(vao)->bindings[0].buffer, 0u);
}

TEST_CASE("bind_vertex_buffer_ungenerated_buffer_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindVertexArray(ctx.genVertexArray());
    ctx.bindVertexBuffer(0, 7777, 0, 12);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("bind_vertex_buffers_binds_consecutive_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);
    GLObjectName bufs[2] = {ctx.genBuffer(), ctx.genBuffer()};
    intptr_t offsets[2] = {0, 8};
    int32_t strides[2] = {12, 24};

    ctx.bindVertexBuffers(0, 2, bufs, offsets, strides);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* v = ctx.getVertexArray(vao);
    EXPECT_EQ(v->bindings[0].buffer, bufs[0]);
    EXPECT_EQ(v->bindings[0].stride, 12);
    EXPECT_EQ(v->bindings[1].buffer, bufs[1]);
    EXPECT_EQ(v->bindings[1].offset, 8);
    EXPECT_EQ(v->bindings[1].stride, 24);
}

TEST_CASE("bind_vertex_buffers_null_array_resets_range_to_defaults") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);
    GLObjectName bufs[2] = {ctx.genBuffer(), ctx.genBuffer()};
    intptr_t offsets[2] = {4, 8};
    int32_t strides[2] = {12, 24};
    ctx.bindVertexBuffers(0, 2, bufs, offsets, strides);

    // SPEC §10.3.2: null `buffers` resets the range (offset 0, stride 16).
    ctx.bindVertexBuffers(0, 2, nullptr, nullptr, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto* v = ctx.getVertexArray(vao);
    EXPECT_EQ(v->bindings[0].buffer, 0u);
    EXPECT_EQ(v->bindings[0].offset, 0);
    EXPECT_EQ(v->bindings[0].stride, 16);
    EXPECT_EQ(v->bindings[1].buffer, 0u);
    EXPECT_EQ(v->bindings[1].stride, 16);
}

TEST_CASE("bind_vertex_buffers_negative_count_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindVertexArray(ctx.genVertexArray());
    ctx.bindVertexBuffers(0, -1, nullptr, nullptr, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("bind_vertex_buffers_range_overflow_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindVertexArray(ctx.genVertexArray());
    // SPEC §10.3.2: first + count > MAX_VERTEX_ATTRIB_BINDINGS.
    ctx.bindVertexBuffers(14, 4, nullptr, nullptr, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("bind_vertex_buffers_validates_entries_per_binding_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);
    GLObjectName good = ctx.genBuffer();
    GLObjectName bufs[3] = {good, 8888 /* ungenerated */, good};
    intptr_t offsets[3] = {0, 0, -8 /* negative */};
    int32_t strides[3] = {12, 12, 12};

    ctx.bindVertexBuffers(0, 3, bufs, offsets, strides);
    // SPEC §10.3.2: invalid entries report an error per binding point...
    EXPECT_NE(ctx.getError(), GLError::NoError);
    auto* v = ctx.getVertexArray(vao);
    // ...but only the valid entry is applied; points 1 and 2 stay unchanged.
    EXPECT_EQ(v->bindings[0].buffer, good);
    EXPECT_EQ(v->bindings[1].buffer, 0u);
    EXPECT_EQ(v->bindings[2].buffer, 0u);
}

TEST_CASE("vertex_attrib_format_records_format_on_bound_vao") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    ctx.vertexAttribFormat(2, 3, GL_FLOAT, true, 8);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto& a = ctx.getVertexArray(vao)->attrib(2);
    EXPECT_EQ(a.size, 3);
    EXPECT_EQ(a.type, GL_FLOAT);
    EXPECT_TRUE(a.normalized);
    EXPECT_EQ(a.relativeoffset, 8);
}

TEST_CASE("vertex_attrib_iformat_and_lformat_are_never_normalized") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    ctx.vertexAttribFormat(0, 4, GL_FLOAT, true, 0); // normalized first
    ctx.vertexAttribIFormat(0, 2, GL_INT, 4);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_FALSE(ctx.getVertexArray(vao)->attrib(0).normalized);
    EXPECT_EQ(ctx.getVertexArray(vao)->attrib(0).type, GL_INT);

    ctx.vertexAttribFormat(1, 4, GL_FLOAT, true, 0);
    ctx.vertexAttribLFormat(1, 1, GL_FLOAT, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_FALSE(ctx.getVertexArray(vao)->attrib(1).normalized);
}

TEST_CASE("vertex_attrib_format_validates_index_and_size") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindVertexArray(ctx.genVertexArray());

    ctx.vertexAttribFormat(16, 4, GL_FLOAT, false, 0); // index >= MAX
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexAttribFormat(0, 5, GL_FLOAT, false, 0); // size > 4
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexAttribIFormat(0, 0, GL_INT, 0); // size < 1
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexAttribLFormat(16, 1, GL_FLOAT, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("vertex_attrib_format_with_default_vao_succeeds") {
    auto backend = makeBackend();
    Context ctx(*backend);
    // Default VAO (name 0) is always present in the compat profile.
    ctx.vertexAttribFormat(0, 4, GL_FLOAT, false, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
}

TEST_CASE("vertex_attrib_binding_maps_attribute_to_binding_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    ctx.vertexAttribBinding(3, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getVertexArray(vao)->attrib(3).binding, 1u);

    // attribindex / bindingindex limits (SPEC §10.3.2).
    ctx.vertexAttribBinding(16, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexAttribBinding(0, 16);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("vertex_binding_divisor_records_and_validates") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    ctx.vertexBindingDivisor(2, 3);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getVertexArray(vao)->bindings[2].divisor, 3u);

    ctx.vertexBindingDivisor(16, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("dsa_vertex_buffer_shares_validation_with_non_dsa") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();

    // The DSA spelling now enforces the same SPEC §10.3.2 limits.
    ctx.vertexArrayVertexBuffer(vao, 16, buf, 0, 12);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexArrayVertexBuffer(vao, 0, buf, -1, 12);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexArrayAttribBinding(vao, 0, 16);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    ctx.vertexArrayBindingDivisor(vao, 16, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("gl_api_vertex_attrib_binding_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint vao = 0, buf = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &buf);
    glBindVertexArray(vao);

    glBindVertexBuffer(0, buf, 0, 20);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribBinding(0, 0);
    glVertexBindingDivisor(0, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    auto* v = ctx.getVertexArray(vao);
    EXPECT_EQ(v->bindings[0].buffer, buf);
    EXPECT_EQ(v->bindings[0].stride, 20);
    EXPECT_EQ(v->bindings[0].divisor, 1u);
    EXPECT_EQ(v->attrib(0).size, 3);
    EXPECT_EQ(v->attrib(0).binding, 0u);

    GLuint bufs[2] = {buf, buf};
    GLintptr offsets[2] = {0, 4};
    GLsizei strides[2] = {20, 24};
    glBindVertexBuffers(0, 2, bufs, offsets, strides);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(v->bindings[1].offset, 4);
    EXPECT_EQ(v->bindings[1].stride, 24);

    setCurrentContext(nullptr);
}
