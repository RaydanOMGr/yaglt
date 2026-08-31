#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

// glMultiDrawArraysBaseInstance records the call and the first draw's base
// instance (SPEC §10, GL 4.6).
TEST_CASE("multi_draw_arrays_base_instance_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    int32_t firsts[2] = {0, 3};
    int32_t counts[2] = {3, 4};
    int32_t instances[2] = {1, 2};
    uint32_t bases[2] = {7, 11};
    glMultiDrawArraysBaseInstance(GL_TRIANGLES, firsts, counts, instances, bases, 2);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.multiDrawArraysBaseInstanceCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastMultiDrawCount, 2);
    EXPECT_EQ(backend.lastMultiDrawBaseInstance, 7u);

    setCurrentContext(nullptr);
}

// glMultiDrawElementsBaseInstance records the call and the first draw's base
// instance (SPEC §10, GL 4.6).
TEST_CASE("multi_draw_elements_base_instance_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    int32_t counts[2] = {6, 6};
    intptr_t indices[2] = {0, 12};
    uint32_t bases[2] = {3, 5};
    glMultiDrawElementsBaseInstance(GL_TRIANGLES, counts, GL_UNSIGNED_INT,
                                   reinterpret_cast<const GLvoid* const*>(indices),
                                   2, bases);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.multiDrawElementsBaseInstanceCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastMultiDrawCount, 2);
    EXPECT_EQ(backend.lastMultiDrawBaseInstance, 3u);

    setCurrentContext(nullptr);
}

// A null instanceCounts is accepted (every draw is a single instance).
TEST_CASE("multi_draw_arrays_base_instance_null_instances_ok") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    int32_t firsts[1] = {0};
    int32_t counts[1] = {3};
    uint32_t bases[1] = {2};
    glMultiDrawArraysBaseInstance(GL_TRIANGLES, firsts, counts, nullptr, bases, 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(backend.multiDrawArraysBaseInstanceCalls, 1);

    setCurrentContext(nullptr);
}

// Negative drawcount is rejected (GL_INVALID_VALUE) and a missing program is
// rejected (GL_INVALID_OPERATION) (SPEC §10).
TEST_CASE("multi_draw_base_instance_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    int32_t firsts[1] = {0};
    int32_t counts[1] = {3};
    uint32_t bases[1] = {0};
    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glMultiDrawArraysBaseInstance(GL_TRIANGLES, firsts, counts, nullptr, bases, -1);
    EXPECT_EQ(backend.multiDrawArraysBaseInstanceCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    glUseProgram(0);
    glMultiDrawArraysBaseInstance(GL_TRIANGLES, firsts, counts, nullptr, bases, 1);
    EXPECT_EQ(backend.multiDrawArraysBaseInstanceCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

// Without Feature::BaseInstance the call is rejected (GL_INVALID_OPERATION)
// (SPEC §10, ARB_base_instance / GL 4.2).
TEST_CASE("multi_draw_base_instance_requires_base_instance_capability") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    backend.setCapability(Feature::BaseInstance, FeatureSupport::Unsupported);
    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    int32_t firsts[1] = {0};
    int32_t counts[1] = {3};
    uint32_t bases[1] = {0};
    glMultiDrawArraysBaseInstance(GL_TRIANGLES, firsts, counts, nullptr, bases, 1);
    EXPECT_EQ(backend.multiDrawArraysBaseInstanceCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
