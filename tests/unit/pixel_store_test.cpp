#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {

// Pixel store (SPEC §8.4): tracks every pack/unpack parameter, pushes only
// changed values to the backend, validates pnames, and answers glGet*.

TEST_CASE("pixel_store_query_defaults") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint v = -1;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &v);
    EXPECT_EQ(v, 4);
    glGetIntegerv(GL_PACK_ALIGNMENT, &v);
    EXPECT_EQ(v, 4);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &v);
    EXPECT_EQ(v, 0);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &v);
    EXPECT_EQ(v, 0);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &v);
    EXPECT_EQ(v, 0);
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &v);
    EXPECT_EQ(v, 0);
    glGetIntegerv(GL_UNPACK_COMPRESSED_BLOCK_WIDTH, &v);
    EXPECT_EQ(v, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_set_and_query_round_trip") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 16);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 3);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint v = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &v);
    EXPECT_EQ(v, 1);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &v);
    EXPECT_EQ(v, 16);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &v);
    EXPECT_EQ(v, 3);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_change_skipping_suppresses_redundant_push") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    int before = backend.pixelStoreiCalls;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 10);
    EXPECT_EQ(backend.pixelStoreiCalls, before + 1);
    // same value again -> no push
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 10);
    EXPECT_EQ(backend.pixelStoreiCalls, before + 1);
    // different value -> one more push
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 20);
    EXPECT_EQ(backend.pixelStoreiCalls, before + 2);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_unknown_pname_is_enum_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(0xDEAD, 1);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_bad_alignment_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 3);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glPixelStorei(GL_PACK_ALIGNMENT, 16);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    // valid alignment values accepted
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_negative_nonnegative_is_value_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, -1);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glPixelStorei(GL_PACK_SKIP_PIXELS, -5);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_boolean_pname_accepts_any_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    GLint v = 0;
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &v);
    EXPECT_EQ(v, 1);
    glGetBooleanv(GL_UNPACK_LSB_FIRST,
                  reinterpret_cast<unsigned char*>(&v));
    EXPECT_EQ(v, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_f_path_round_trip") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStoref(GL_UNPACK_SKIP_PIXELS, 7.0f);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    GLfloat fv = -1.0f;
    glGetFloatv(GL_UNPACK_SKIP_PIXELS, &fv);
    EXPECT_EQ(fv, 7.0f);
    // float alignment out of the {1,2,4,8} set is a value error
    glPixelStoref(GL_UNPACK_ALIGNMENT, 3.0f);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_compressed_block_params") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(GL_UNPACK_COMPRESSED_BLOCK_WIDTH, 4);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    GLint v = 0;
    glGetIntegerv(GL_UNPACK_COMPRESSED_BLOCK_WIDTH, &v);
    EXPECT_EQ(v, 4);
    glPixelStorei(GL_PACK_COMPRESSED_BLOCK_SIZE, -1);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("pixel_store_public_dispatch_via_gl_api") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    GLint v = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &v);
    EXPECT_EQ(v, 1);

    setCurrentContext(nullptr);
}

} // namespace
