#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

// glGetQueryIndexediv (SPEC §4 / §19): reports the active query for an indexed
// counter target (PRIMITIVES_GENERATED / TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN).
TEST_CASE("glGetQueryIndexediv reads active indexed counter query") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQueryIndexed(GL_PRIMITIVES_GENERATED, 0, q);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Active query at index 0 is reported.
    GLint cur = -1;
    glGetQueryIndexediv(GL_PRIMITIVES_GENERATED, 0, GL_CURRENT_QUERY, &cur);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(cur, static_cast<GLint>(q));

    // No query active at index 1 -> 0.
    GLint none = -1;
    glGetQueryIndexediv(GL_PRIMITIVES_GENERATED, 1, GL_CURRENT_QUERY, &none);
    EXPECT_EQ(none, 0);

    // TF-written counter target is reported independently per index.
    GLuint q2 = 0;
    glGenQueries(1, &q2);
    glBeginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 2, q2);
    GLint tf = -1;
    glGetQueryIndexediv(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 2, GL_CURRENT_QUERY, &tf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(tf, static_cast<GLint>(q2));

    // Ending the query clears the active entry.
    glEndQueryIndexed(GL_PRIMITIVES_GENERATED, 0);
    GLint after = -1;
    glGetQueryIndexediv(GL_PRIMITIVES_GENERATED, 0, GL_CURRENT_QUERY, &after);
    EXPECT_EQ(after, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("glGetQueryIndexediv validates target pname and params") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLint v = 0;

    // Non-counter target -> INVALID_ENUM.
    glGetQueryIndexediv(GL_SAMPLES_PASSED, 0, GL_CURRENT_QUERY, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Non-indexed target (TIMESTAMP) -> INVALID_ENUM.
    glGetQueryIndexediv(GL_TIMESTAMP, 0, GL_CURRENT_QUERY, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Bad pname -> INVALID_ENUM.
    glGetQueryIndexediv(GL_PRIMITIVES_GENERATED, 0, 0xDEAD, &v);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Null params -> INVALID_VALUE.
    glGetQueryIndexediv(GL_PRIMITIVES_GENERATED, 0, GL_CURRENT_QUERY, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
