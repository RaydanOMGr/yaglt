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

// glIsVertexArray (SPEC §10.3.2) reports whether a name is a generated VAO.
// The default VAO (name 0) is never a queried object.
TEST_CASE("is_vertex_array_false_for_ungenerated") {
    auto backend = makeBackend();
    Context ctx(*backend);

    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_FALSE(ctx.isVertexArray(999));
    EXPECT_FALSE(ctx.isVertexArray(0));
}

TEST_CASE("is_vertex_array_true_for_generated") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName vao = ctx.genVertexArray();
    EXPECT_NE(vao, 0u);
    EXPECT_TRUE(ctx.isVertexArray(vao));
}

TEST_CASE("is_vertex_array_false_after_delete") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName vao = ctx.genVertexArray();
    ctx.deleteVertexArray(vao);
    EXPECT_FALSE(ctx.isVertexArray(vao));
}

TEST_CASE("glIsVertexArray_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLObjectName vao = 0;
    ctx.genVertexArrays(1, &vao);
    EXPECT_EQ(glIsVertexArray(vao), GL_TRUE);
    EXPECT_EQ(glIsVertexArray(0), GL_FALSE);

    setCurrentContext(nullptr);
}
