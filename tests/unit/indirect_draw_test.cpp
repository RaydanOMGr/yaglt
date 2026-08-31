#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>

using namespace glcompat;

namespace {
constexpr GLenum GL_TRIANGLES = 0x0004;
} // namespace

TEST_CASE("draw_arrays_indirect_requires_program_and_indirect_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // IndirectDrawing is Native on the mock profile.
    EXPECT_TRUE(backend.capabilities().isSupported(Feature::IndirectDrawing));

    // No indirect buffer bound -> INVALID_OPERATION (SPEC §10).
    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawArraysIndirect(GL_TRIANGLES, nullptr);
    EXPECT_EQ(backend.drawArraysIndirectCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Bind an indirect buffer (frontend owns the name; mock records the bind).
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);

    // No program -> INVALID_OPERATION even with the buffer bound.
    glUseProgram(0);
    glDrawArraysIndirect(GL_TRIANGLES, nullptr);
    EXPECT_EQ(backend.drawArraysIndirectCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Both satisfied -> native indirect draw is issued with the byte offset.
    glUseProgram(_pg);
    glDrawArraysIndirect(GL_TRIANGLES,
                         reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(8)));
    EXPECT_EQ(backend.drawArraysIndirectCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastIndirect, reinterpret_cast<const void*>(static_cast<intptr_t>(8)));

    setCurrentContext(nullptr);
}

TEST_CASE("draw_elements_indirect_records_mode_type_and_offset") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);

    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT,
                           reinterpret_cast<const GLvoid*>(static_cast<intptr_t>(16)));
    EXPECT_EQ(backend.drawElementsIndirectCalls, 1);
    EXPECT_EQ(backend.lastDrawMode, GL_TRIANGLES);
    EXPECT_EQ(backend.lastDrawType, GL_UNSIGNED_INT);
    EXPECT_EQ(backend.lastIndirect,
              reinterpret_cast<const void*>(static_cast<intptr_t>(16)));

    setCurrentContext(nullptr);
}

TEST_CASE("draw_indirect_unbinds_buffer_blocks_draw") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    GLuint _pg; MAKE_VALID_PROGRAM(_pg);
    glUseProgram(_pg);
    glDrawArraysIndirect(GL_TRIANGLES, nullptr);
    EXPECT_EQ(backend.drawArraysIndirectCalls, 1);

    // Unbind the indirect buffer; subsequent indirect draw must be rejected.
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
    EXPECT_EQ(backend.drawElementsIndirectCalls, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
