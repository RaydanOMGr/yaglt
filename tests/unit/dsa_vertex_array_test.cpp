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

// DSA vertex-array surface operates on a named VAO with no bound-VAO side
// effect; the recorded state is replayed through the legacy flush path.
TEST_CASE("dsa_create_vertex_arrays_generates_objects") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName vaos[3] = {0, 0, 0};
    ctx.createVertexArrays(3, vaos);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(vaos[0], 0u);
    EXPECT_NE(vaos[1], 0u);
    EXPECT_NE(vaos[2], 0u);
    EXPECT_NE(ctx.getVertexArray(vaos[0]), nullptr);
    EXPECT_NE(ctx.getVertexArray(vaos[1]), nullptr);
    EXPECT_NE(ctx.getVertexArray(vaos[2]), nullptr);
}

TEST_CASE("dsa_create_vertex_arrays_gated_by_direct_state_access") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);

    GLObjectName vao = 0;
    ctx.createVertexArrays(1, &vao);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(vao, 0u);
}

TEST_CASE("dsa_attrib_enable_disable_records_state") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();

    ctx.enableVertexArrayAttrib(vao, 2);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_TRUE(ctx.getVertexArray(vao)->attrib(2).enabled);

    ctx.disableVertexArrayAttrib(vao, 2);
    EXPECT_FALSE(ctx.getVertexArray(vao)->attrib(2).enabled);
}

TEST_CASE("dsa_ungenerated_vao_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    ctx.enableVertexArrayAttrib(999, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.vertexArrayElementBuffer(999, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.vertexArrayVertexBuffer(999, 0, 0, 0, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("dsa_vertex_buffer_and_attrib_format_flush_combines_offset") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();

    // attribute 0 sources binding 0; base offset 0, relative offset 12.
    ctx.vertexArrayVertexBuffer(vao, 0, buf, 0, 32);
    ctx.vertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, false, 12);
    ctx.enableVertexArrayAttrib(vao, 0);

    ctx.bindVertexArray(vao);
    ctx.flushState();

    // bindVertexArray pushed, then ARRAY_BUFFER bound, then the pointer with the
    // combined offset (binding.offset + attrib.relativeoffset = 0 + 12).
    EXPECT_TRUE(backend->bindVertexArrayCalls >= 1);
    EXPECT_EQ(backend->vertexAttribPtrs.size(), 1u);
    EXPECT_EQ(backend->vertexAttribPtrs[0].index, 0u);
    EXPECT_EQ(backend->vertexAttribPtrs[0].size, 3);
    EXPECT_EQ(backend->vertexAttribPtrs[0].type, static_cast<uint32_t>(GL_FLOAT));
    EXPECT_EQ(backend->vertexAttribPtrs[0].normalized, false);
    EXPECT_EQ(backend->vertexAttribPtrs[0].stride, 32);
    EXPECT_EQ(backend->vertexAttribPtrs[0].offset, 12);

    EXPECT_EQ(backend->bufferBinds.size(), 1u);
    EXPECT_EQ(backend->bufferBinds[0].target,
              static_cast<uint32_t>(GL_ARRAY_BUFFER));
    EXPECT_EQ(backend->bufferBinds[0].buffer, buf);
}

TEST_CASE("dsa_attrib_binding_to_distinct_vertex_buffer") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();

    // attribute 0 pulls from binding 1, which is a different buffer/stride.
    ctx.vertexArrayVertexBuffer(vao, 1, buf, 4, 16);
    ctx.vertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, false, 0);
    ctx.vertexArrayAttribBinding(vao, 0, 1);
    ctx.enableVertexArrayAttrib(vao, 0);

    ctx.bindVertexArray(vao);
    ctx.flushState();

    EXPECT_EQ(backend->vertexAttribPtrs.size(), 1u);
    EXPECT_EQ(backend->vertexAttribPtrs[0].index, 0u);
    EXPECT_EQ(backend->vertexAttribPtrs[0].stride, 16);
    EXPECT_EQ(backend->vertexAttribPtrs[0].offset, 4);
    EXPECT_EQ(backend->bufferBinds.back().buffer, buf);
}

TEST_CASE("dsa_binding_divisor_flushes_vertex_attrib_divisor") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();

    ctx.vertexArrayVertexBuffer(vao, 1, buf, 0, 24);
    ctx.vertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, false, 0);
    ctx.vertexArrayAttribBinding(vao, 0, 1);
    ctx.vertexArrayBindingDivisor(vao, 1, 3);
    ctx.enableVertexArrayAttrib(vao, 0);

    ctx.bindVertexArray(vao);
    ctx.flushState();

    EXPECT_EQ(backend->attribDivisors.size(), 1u);
    EXPECT_EQ(backend->attribDivisors[0].index, 0u);
    EXPECT_EQ(backend->attribDivisors[0].divisor, 3u);
}

TEST_CASE("dsa_element_buffer_binds_on_flush") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName ebo = ctx.genBuffer();

    ctx.vertexArrayElementBuffer(vao, ebo);
    EXPECT_EQ(ctx.getVertexArray(vao)->elementBuffer, ebo);

    ctx.bindVertexArray(vao);
    ctx.flushState();

    bool found = false;
    for (const auto& b : backend->bufferBinds) {
        if (b.target == static_cast<uint32_t>(GL_ELEMENT_ARRAY_BUFFER) &&
            b.buffer == ebo) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_CASE("dsa_attrib_format_validation_and_integer_variant") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();

    ctx.vertexArrayAttribFormat(vao, 0, 5, GL_FLOAT, false, 0); // size > 4
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    ctx.vertexArrayAttribIFormat(vao, 1, 1, 0x1404 /*GL_INT*/, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto& a = ctx.getVertexArray(vao)->attrib(1);
    EXPECT_EQ(a.size, 1);
    EXPECT_EQ(a.normalized, false); // integer attributes never normalized
    EXPECT_EQ(a.type, 0x1404u);     // GL_INT
}

TEST_CASE("dsa_vertex_buffers_array_variant") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName bufs[2] = {ctx.genBuffer(), ctx.genBuffer()};
    intptr_t offsets[2] = {0, 8};
    int32_t strides[2] = {12, 20};

    ctx.vertexArrayVertexBuffers(vao, 0, 2, bufs, offsets, strides);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    auto& b0 = ctx.getVertexArray(vao)->bindings[0];
    auto& b1 = ctx.getVertexArray(vao)->bindings[1];
    EXPECT_EQ(b0.buffer, bufs[0]);
    EXPECT_EQ(b0.offset, 0);
    EXPECT_EQ(b0.stride, 12);
    EXPECT_EQ(b1.buffer, bufs[1]);
    EXPECT_EQ(b1.offset, 8);
    EXPECT_EQ(b1.stride, 20);

    // SPEC §10.3.2: a null `buffers` array is legal and resets every touched
    // binding point to no buffer, offset 0 and stride 16 (offsets/strides are
    // ignored). This previously reported GL_INVALID_VALUE, which the spec does
    // not list as an error for this command.
    ctx.vertexArrayVertexBuffers(vao, 0, 2, nullptr, offsets, strides);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(b0.buffer, 0u);
    EXPECT_EQ(b0.offset, 0);
    EXPECT_EQ(b0.stride, 16);
    EXPECT_EQ(b1.buffer, 0u);
    EXPECT_EQ(b1.offset, 0);
    EXPECT_EQ(b1.stride, 16);
}

// Regression: the unified flush path must still replay the legacy
// gl*VertexAttribPointer correctly after the binding-map refactor.
TEST_CASE("legacy_vertex_attrib_pointer_flush_unchanged") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName buf = ctx.genBuffer();

    ctx.bindVertexArray(vao);
    ctx.bindBuffer(GL_ARRAY_BUFFER, buf);
    ctx.vertexAttribPointer(0, 3, GL_FLOAT, false, 24, 8);
    ctx.enableVertexAttribArray(0);
    ctx.flushState();

    EXPECT_EQ(backend->vertexAttribPtrs.size(), 1u);
    EXPECT_EQ(backend->vertexAttribPtrs[0].index, 0u);
    EXPECT_EQ(backend->vertexAttribPtrs[0].size, 3);
    EXPECT_EQ(backend->vertexAttribPtrs[0].stride, 24);
    EXPECT_EQ(backend->vertexAttribPtrs[0].offset, 8);
    EXPECT_EQ(backend->bufferBinds.back().buffer, buf);
}

// Exercise the new gl_api entry points (SPEC §10.3.1) through the global API.
TEST_CASE("gl_api_dsa_vertex_array_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint vao = 0, buf = 0;
    glCreateVertexArrays(1, &vao);
    glGenBuffers(1, &buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glEnableVertexArrayAttrib(vao, 0);
    glBindVertexArray(vao); // bind so flush replays
    glFlushState();         // global flush entry point
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_TRUE(backend->enableVertexAttribOrder.size() >= 1u);
    EXPECT_EQ(backend->enableVertexAttribOrder.back(), 0u);

    glcompat::setCurrentContext(nullptr);
}

// --- DSA vertex-array queries (SPEC §10.3.1) ---

TEST_CASE("get_vertex_array_iv_element_buffer_binding") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();
    GLObjectName ebo = ctx.genBuffer();

    ctx.vertexArrayElementBuffer(vao, ebo);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t v = -1;
    ctx.getVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(v, static_cast<int32_t>(ebo));
}

TEST_CASE("get_vertex_array_indexed_iv_per_attrib_state") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();

    ctx.vertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, false, 0);
    ctx.enableVertexArrayAttrib(vao, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t enabled = 0, size = 0, type = 0, normalized = 0, bufBinding = 0;
    ctx.getVertexArrayIndexediv(vao, 0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    ctx.getVertexArrayIndexediv(vao, 0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
    ctx.getVertexArrayIndexediv(vao, 0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
    ctx.getVertexArrayIndexediv(vao, 0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
    ctx.getVertexArrayIndexediv(vao, 0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bufBinding);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(enabled, 1);
    EXPECT_EQ(size, 3);
    EXPECT_EQ(type, static_cast<int32_t>(GL_FLOAT));
    EXPECT_EQ(normalized, 0);
    EXPECT_EQ(bufBinding, 0); // no buffer bound to this attribute yet

    // A disabled attribute reports 0.
    int32_t disabled = 1;
    ctx.getVertexArrayIndexediv(vao, 1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &disabled);
    EXPECT_EQ(disabled, 0);
}

TEST_CASE("get_vertex_array_indexed_64v_binding_and_relative_offset") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();

    ctx.vertexArrayVertexBuffer(vao, 1, ctx.genBuffer(), 4, 16);
    ctx.vertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, false, 12);
    ctx.vertexArrayAttribBinding(vao, 0, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int64_t binding = -1, relOffset = -1;
    ctx.getVertexArrayIndexed64v(vao, 0, GL_VERTEX_ATTRIB_BINDING, &binding);
    ctx.getVertexArrayIndexed64v(vao, 0, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &relOffset);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(binding, 1);
    EXPECT_EQ(relOffset, 12);
}

TEST_CASE("get_vertex_array_indexed_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();

    // Unknown pname -> INVALID_ENUM.
    int32_t v = 0;
    ctx.getVertexArrayIndexediv(vao, 0, 0xDEAD, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    int64_t v64 = 0;
    ctx.getVertexArrayIndexed64v(vao, 0, 0xDEAD, &v64);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    // Out-of-range index -> INVALID_VALUE.
    int32_t oob = 0;
    ctx.getVertexArrayIndexediv(vao, 1024, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &oob);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Null params -> INVALID_VALUE.
    ctx.getVertexArrayIndexediv(vao, 0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("get_vertex_array_ungenerated_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    int32_t v = 0;
    ctx.getVertexArrayiv(999, GL_ELEMENT_ARRAY_BUFFER_BINDING, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.getVertexArrayIndexediv(999, 0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("get_vertex_array_gated_by_direct_state_access") {
    auto backend = makeBackend();
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName vao = ctx.genVertexArray();

    int32_t v = 0;
    ctx.getVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &v);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// Exercise the public gl_api entry points (SPEC §10.3.1).
TEST_CASE("gl_api_get_vertex_array_queries") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint vao = 0, ebo = 0;
    glCreateVertexArrays(1, &vao);
    glGenBuffers(1, &ebo);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glVertexArrayElementBuffer(vao, ebo);
    GLint v = -1;
    glGetVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &v);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(v, static_cast<GLint>(ebo));

    glcompat::setCurrentContext(nullptr);
}
