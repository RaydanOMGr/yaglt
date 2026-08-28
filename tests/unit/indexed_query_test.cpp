#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

// The typed indexed getters report the same per-slot enable state as the int /
// boolean variants (SPEC §22.3). Only BLEND / SCISSOR_TEST are indexable.
TEST_CASE("indexed_query_typed_variants_report_enable_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glEnablei(GL_BLEND, 0);
    glEnablei(GL_BLEND, 2);
    glEnablei(GL_SCISSOR_TEST, 1);

    GLint i = 0;
    glGetIntegeri_v(GL_BLEND, 0, &i);
    EXPECT_EQ(i, 1);
    glGetIntegeri_v(GL_BLEND, 1, &i);
    EXPECT_EQ(i, 0); // not enabled here

    GLboolean b = 0;
    glGetBooleani_v(GL_SCISSOR_TEST, 1, &b);
    EXPECT_EQ(b, static_cast<GLboolean>(0x01));

    GLfloat f = 0.0f;
    glGetFloati_v(GL_BLEND, 0, &f);
    EXPECT_EQ(f, 1.0f);
    glGetFloati_v(GL_BLEND, 2, &f);
    EXPECT_EQ(f, 1.0f);
    glGetFloati_v(GL_BLEND, 3, &f);
    EXPECT_EQ(f, 0.0f);

    GLdouble d = 0.0;
    glGetDoublei_v(GL_SCISSOR_TEST, 1, &d);
    EXPECT_EQ(d, 1.0);

    GLint64 i64 = 0;
    glGetInteger64i_v(GL_BLEND, 2, &i64);
    EXPECT_EQ(i64, 1);

    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("indexed_query_typed_variants_validate") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLfloat f = 0.0f;
    GLdouble d = 0.0;
    GLint64 i64 = 0;

    // Unknown pname -> INVALID_ENUM.
    glGetFloati_v(0xDEAD, 0, &f);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetDoublei_v(0xDEAD, 0, &d);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetInteger64i_v(0xDEAD, 0, &i64);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Out-of-range index -> INVALID_VALUE.
    glGetFloati_v(GL_BLEND, 16, &f);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetDoublei_v(GL_BLEND, 99, &d);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetInteger64i_v(GL_BLEND, 16, &i64);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Null params -> INVALID_VALUE.
    glGetFloati_v(GL_BLEND, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetDoublei_v(GL_BLEND, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetInteger64i_v(GL_BLEND, 0, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
