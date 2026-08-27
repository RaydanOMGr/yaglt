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
