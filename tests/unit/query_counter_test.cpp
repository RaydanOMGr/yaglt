#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_factory.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
// glQueryCounter (SPEC §4.2.1): records a timestamp when all prior GL commands
// have completed. Only the GL_TIMESTAMP target is valid; the id must name a
// generated, non-active query object.
} // namespace

TEST_CASE("query_counter_records_timestamp_via_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glCreateQueries(GL_SAMPLES_PASSED, 1, &q);
    EXPECT_NE(q, 0u);

    glQueryCounter(q, GL_TIMESTAMP);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    auto& factory = static_cast<MockResourceFactory&>(backend.resourceFactory());
    EXPECT_EQ(factory.lastCreatedQuery->queryCounterCalls, 1);
    EXPECT_EQ(factory.lastCreatedQuery->lastCounterTarget, GL_TIMESTAMP);

    setCurrentContext(nullptr);
}

TEST_CASE("query_counter_bad_target_is_enum_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glCreateQueries(GL_SAMPLES_PASSED, 1, &q);

    glQueryCounter(q, GL_SAMPLES_PASSED); // not GL_TIMESTAMP
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("query_counter_ungenerated_id_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glQueryCounter(999u, GL_TIMESTAMP); // never created
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("query_counter_active_query_is_operation_error") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glCreateQueries(GL_SAMPLES_PASSED, 1, &q);

    glBeginQuery(GL_ANY_SAMPLES_PASSED, q);
    glQueryCounter(q, GL_TIMESTAMP); // already active
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glEndQuery(GL_ANY_SAMPLES_PASSED);

    setCurrentContext(nullptr);
}

TEST_CASE("query_counter_public_dispatch_surface") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glCreateQueries(GL_SAMPLES_PASSED, 1, &q);
    glQueryCounter(q, GL_TIMESTAMP);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}
