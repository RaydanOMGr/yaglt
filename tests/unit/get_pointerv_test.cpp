#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

namespace {
// A trivial debug callback so glGetPointerv can report a non-null function.
void testDebugCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*,
                      const void*) {}

constexpr GLenum kGL_FLOAT = 0x1406;
} // namespace

// glGetPointerv(DEBUG_CALLBACK_FUNCTION) returns the installed callback pointer.
TEST_CASE("get_pointerv_debug_callback_function") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glDebugMessageCallback(testDebugCallback, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    void* fn = nullptr;
    glGetPointerv(GL_DEBUG_CALLBACK_FUNCTION, &fn);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(fn, reinterpret_cast<void*>(testDebugCallback));

    setCurrentContext(nullptr);
}

// glGetPointerv(DEBUG_CALLBACK_USER_PARAM) returns the installed user parameter.
TEST_CASE("get_pointerv_debug_callback_user_param") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    int sentinel = 0xCAFE;
    glDebugMessageCallback(testDebugCallback, &sentinel);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    void* up = nullptr;
    glGetPointerv(GL_DEBUG_CALLBACK_USER_PARAM, &up);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(up, &sentinel);

    setCurrentContext(nullptr);
}

// With no callback installed the debug pointers query as null.
TEST_CASE("get_pointerv_debug_callback_absent_is_null") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    void* fn = reinterpret_cast<void*>(0x1);
    glGetPointerv(GL_DEBUG_CALLBACK_FUNCTION, &fn);
    EXPECT_EQ(fn, nullptr);

    setCurrentContext(nullptr);
}

// An unknown pname is rejected with GL_INVALID_ENUM.
TEST_CASE("get_pointerv_rejects_unknown_pname") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    void* p = nullptr;
    glGetPointerv(GL_BLEND, &p);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

// A null params pointer is rejected with GL_INVALID_VALUE.
TEST_CASE("get_pointerv_rejects_null_params") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glGetPointerv(GL_DEBUG_CALLBACK_FUNCTION, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

// SELECTION_BUFFER_POINTER / FEEDBACK_BUFFER_POINTER are unimplemented (null).
TEST_CASE("get_pointerv_selection_feedback_buffers_null") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    void* sel = reinterpret_cast<void*>(0x1);
    void* fb = reinterpret_cast<void*>(0x1);
    glGetPointerv(GL_SELECTION_BUFFER_POINTER, &sel);
    glGetPointerv(GL_FEEDBACK_BUFFER_POINTER, &fb);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(sel, nullptr);
    EXPECT_EQ(fb, nullptr);

    setCurrentContext(nullptr);
}

// VERTEX_ARRAY_POINTER resolves to the bound VAO's generic-attribute client
// pointer (legacy index 0).
TEST_CASE("get_pointerv_vertex_array_pointer_from_vao") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, kGL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<const void*>(static_cast<intptr_t>(0x40)));
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    void* ptr = nullptr;
    glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ptr, reinterpret_cast<void*>(static_cast<intptr_t>(0x40)));

    glBindVertexArray(0);
    setCurrentContext(nullptr);
}
