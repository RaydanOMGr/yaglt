#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

// Current generic vertex attribute values (SPEC §10.2). Recorded on the bound
// VAO and returned by glGetVertexAttribfv/iv for GL_CURRENT_VERTEX_ATTRIB.
TEST_CASE("vertexAttrib4f_sets_current_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glVertexAttrib4f(2, 0.5f, 1.5f, 2.5f, 3.5f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(2, GL_CURRENT_VERTEX_ATTRIB, fv);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(fv[0], 0.5f);
    EXPECT_EQ(fv[1], 1.5f);
    EXPECT_EQ(fv[2], 2.5f);
    EXPECT_EQ(fv[3], 3.5f);
}

TEST_CASE("vertexAttrib3f_defaults_w_to_one") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    glVertexAttrib3f(0, 1.0f, 2.0f, 3.0f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, fv);
    EXPECT_EQ(fv[0], 1.0f);
    EXPECT_EQ(fv[1], 2.0f);
    EXPECT_EQ(fv[2], 3.0f);
    EXPECT_EQ(fv[3], 1.0f);
}

TEST_CASE("vertexAttrib_vector_forms_match_scalar") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    const float v[2] = {7.0f, 9.0f};
    glVertexAttrib2fv(1, v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(1, GL_CURRENT_VERTEX_ATTRIB, fv);
    EXPECT_EQ(fv[0], 7.0f);
    EXPECT_EQ(fv[1], 9.0f);
    EXPECT_EQ(fv[2], 0.0f);
    EXPECT_EQ(fv[3], 1.0f);

    // Null pointer is GL_INVALID_VALUE.
    glVertexAttrib4fv(1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("vertexAttribI4i_stores_integral_current_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    glVertexAttribI4i(3, 10, -20, 30, 40);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t iv[4] = {0, 0, 0, 0};
    glGetVertexAttribiv(3, GL_CURRENT_VERTEX_ATTRIB, iv);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(iv[0], 10);
    EXPECT_EQ(iv[1], -20);
    EXPECT_EQ(iv[2], 30);
    EXPECT_EQ(iv[3], 40);

    // The float query returns the same values as floats for the integer family.
    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(3, GL_CURRENT_VERTEX_ATTRIB, fv);
    EXPECT_EQ(fv[0], 10.0f);
    EXPECT_EQ(fv[1], -20.0f);
    EXPECT_EQ(fv[2], 30.0f);
    EXPECT_EQ(fv[3], 40.0f);
}

TEST_CASE("vertexAttribI4uiv_stores_unsigned_current_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    const uint32_t v[4] = {1u, 2u, 3u, 4u};
    glVertexAttribI4uiv(0, v);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t iv[4] = {0, 0, 0, 0};
    glGetVertexAttribiv(0, GL_CURRENT_VERTEX_ATTRIB, iv);
    EXPECT_EQ(iv[0], 1);
    EXPECT_EQ(iv[1], 2);
    EXPECT_EQ(iv[2], 3);
    EXPECT_EQ(iv[3], 4);
}

TEST_CASE("vertexAttrib_without_bound_vao_is_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glVertexAttrib4f(0, 1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, fv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("vertexAttrib_out_of_range_index_is_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    glVertexAttrib4f(1024, 1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(1024, GL_CURRENT_VERTEX_ATTRIB, fv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("getVertexAttrib_unknown_pname_is_invalid_enum") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    float fv[4] = {0, 0, 0, 0};
    glGetVertexAttribfv(0, 0xDEAD, fv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

// The integer/bool pname family (SPEC §10.4) returns array state.
TEST_CASE("getVertexAttribiv_reads_array_enabled_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    glEnableVertexAttribArray(0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t enabled = 0;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(enabled, 1);

    // A never-enabled attribute reports 0.
    int32_t disabled = 1;
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &disabled);
    EXPECT_EQ(disabled, 0);
}

TEST_CASE("getVertexAttribiv_reads_pointer_attributes") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);
    GLObjectName buf = ctx.genBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    const intptr_t ptr = 0x1234;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12,
                          reinterpret_cast<const void*>(ptr));
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t size = 0, type = 0, stride = 0, normalized = 0, bufBinding = 0;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bufBinding);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(size, 3);
    EXPECT_EQ(type, static_cast<int32_t>(GL_FLOAT));
    EXPECT_EQ(stride, 12);
    EXPECT_EQ(normalized, 0);
    EXPECT_EQ(bufBinding, static_cast<int32_t>(buf));

    // Divisor reads back the per-attribute value (default 0).
    int32_t divisor = 7;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
    EXPECT_EQ(divisor, 0);

    // Pointer query returns the address passed to glVertexAttribPointer.
    void* got = nullptr;
    glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &got);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(reinterpret_cast<intptr_t>(got), ptr);
}

TEST_CASE("getVertexAttribdv_returns_current_value_as_double") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    glVertexAttrib4f(1, 1.5f, 2.25f, 3.125f, 4.0f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    double dv[4] = {0, 0, 0, 0};
    glGetVertexAttribdv(1, GL_CURRENT_VERTEX_ATTRIB, dv);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(dv[0], 1.5);
    EXPECT_EQ(dv[1], 2.25);
    EXPECT_EQ(dv[2], 3.125);
    EXPECT_EQ(dv[3], 4.0);
}

TEST_CASE("getVertexAttribIiv_Iuiv_return_integer_current_and_flag") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    // Float family: ARRAY_INTEGER flag is 0.
    glVertexAttrib4f(0, 1.0f, 2.0f, 3.0f, 4.0f);
    int32_t isIntF = 2;
    glGetVertexAttribIiv(0, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &isIntF);
    EXPECT_EQ(isIntF, 0);
    uint32_t isIntU = 2;
    glGetVertexAttribIuiv(0, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &isIntU);
    EXPECT_EQ(isIntU, 0u);

    // Integer family: flag is 1 and current value round-trips as int/uint.
    glVertexAttribI4i(1, 10, -20, 30, 40);
    int32_t isInt2 = 0;
    glGetVertexAttribIiv(1, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &isInt2);
    EXPECT_EQ(isInt2, 1);
    int32_t iv[4] = {0, 0, 0, 0};
    glGetVertexAttribIiv(1, GL_CURRENT_VERTEX_ATTRIB, iv);
    EXPECT_EQ(iv[0], 10);
    EXPECT_EQ(iv[1], -20);
    EXPECT_EQ(iv[2], 30);
    EXPECT_EQ(iv[3], 40);
    uint32_t uv[4] = {0, 0, 0, 0};
    glGetVertexAttribIuiv(1, GL_CURRENT_VERTEX_ATTRIB, uv);
    EXPECT_EQ(uv[0], 10u);
    EXPECT_EQ(uv[1], static_cast<uint32_t>(-20));
    EXPECT_EQ(uv[2], 30u);
    EXPECT_EQ(uv[3], 40u);
}

TEST_CASE("getVertexAttrib_validation_errors") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    // Null params -> INVALID_VALUE.
    int32_t out = 0;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
    (void)out;

    // Out-of-range index -> INVALID_VALUE.
    int32_t oob = 0;
    glGetVertexAttribiv(1024, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &oob);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Unknown pname -> INVALID_ENUM (all four entry points).
    int32_t iv = 0;
    glGetVertexAttribiv(0, 0xDEAD, &iv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    glGetVertexAttribIiv(0, 0xDEAD, &iv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    uint32_t uv = 0;
    glGetVertexAttribIuiv(0, 0xDEAD, &uv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    void* pv = nullptr;
    glGetVertexAttribPointerv(0, 0xDEAD, &pv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("getVertexAttrib_without_bound_vao_is_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    int32_t iv = 0;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &iv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    void* pv = nullptr;
    glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// glGetVertexAttribLdv (SPEC §10.3) reads the same double-precision current
// attribute value as glGetVertexAttribdv (GLdouble) for CURRENT_VERTEX_ATTRIB.
TEST_CASE("getVertexAttribLdv_matches_current_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLObjectName vao = ctx.genVertexArray();
    ctx.bindVertexArray(vao);

    glVertexAttrib4f(1, 0.25f, -1.75f, 2.0f, 9.0f);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    double dv[4] = {0, 0, 0, 0};
    glGetVertexAttribLdv(1, GL_CURRENT_VERTEX_ATTRIB, dv);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(dv[0], 0.25);
    EXPECT_EQ(dv[1], -1.75);
    EXPECT_EQ(dv[2], 2.0);
    EXPECT_EQ(dv[3], 9.0);

    // The dv and Ldv variants agree on the recorded double value.
    double dv2[4] = {0, 0, 0, 0};
    glGetVertexAttribdv(1, GL_CURRENT_VERTEX_ATTRIB, dv2);
    EXPECT_EQ(dv2[0], dv[0]);
    EXPECT_EQ(dv2[1], dv[1]);
    EXPECT_EQ(dv2[2], dv[2]);
    EXPECT_EQ(dv2[3], dv[3]);

    // A non-current-attribute pname is GL_INVALID_ENUM.
    glGetVertexAttribLdv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, dv);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    // Null params -> GL_INVALID_VALUE.
    glGetVertexAttribLdv(1, GL_CURRENT_VERTEX_ATTRIB, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}
