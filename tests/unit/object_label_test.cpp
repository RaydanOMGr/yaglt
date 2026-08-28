#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cstring>
#include <string>

using namespace glcompat;

namespace {
constexpr GLenum GL_BOGUS_IDENT = 0xDEAD;
constexpr int kMaxLabel = kMaxObjectLabelLength;
} // namespace

TEST_CASE("objectlabel_roundtrip_buffer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Assign and read back a label.
    glObjectLabel(GL_BUFFER, buf, -1, "my-buffer");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    char out[64] = {0};
    GLsizei len = 0;
    glGetObjectLabel(GL_BUFFER, buf, sizeof(out), &len, out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(static_cast<size_t>(len), std::strlen("my-buffer"));
    EXPECT_EQ(std::string(out), std::string("my-buffer"));

    // Explicit length (no nul terminator in source) is honored.
    glObjectLabel(GL_BUFFER, buf, 3, "abcdef");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glGetObjectLabel(GL_BUFFER, buf, sizeof(out), &len, out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(std::string(out), std::string("abc"));

    // Clearing via a null label removes it; query-only mode reports length+1.
    glObjectLabel(GL_BUFFER, buf, -1, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glGetObjectLabel(GL_BUFFER, buf, sizeof(out), &len, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(len, 1); // empty label: 0 chars + 1 nul terminator

    setCurrentContext(nullptr);
}

TEST_CASE("objectlabel_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);

    // Unknown identifier -> GL_INVALID_ENUM.
    glObjectLabel(GL_BOGUS_IDENT, buf, -1, "x");
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    // Live object required -> GL_INVALID_OPERATION for an unused name.
    glObjectLabel(GL_BUFFER, 9999, -1, "x");
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Label longer than the limit -> GL_INVALID_VALUE.
    std::string big(kMaxLabel + 1, 'a');
    glObjectLabel(GL_BUFFER, buf, static_cast<GLsizei>(big.size()), big.c_str());
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // glGetObjectLabel mirrors the identifier / liveness validation.
    char out[8] = {0};
    GLsizei len = 0;
    glGetObjectLabel(GL_BOGUS_IDENT, buf, sizeof(out), &len, out);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetObjectLabel(GL_BUFFER, 9999, sizeof(out), &len, out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glGetObjectLabel(GL_BUFFER, buf, -1, &len, out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("objectlabel_works_across_namespaces") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0, tex = 0, prog = 0;
    glGenBuffers(1, &buf);
    glGenTextures(1, &tex);
    prog = glCreateProgram();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glObjectLabel(GL_BUFFER, buf, -1, "B");
    glObjectLabel(GL_TEXTURE, tex, -1, "T");
    glObjectLabel(GL_PROGRAM, prog, -1, "P");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    char out[16] = {0};
    GLsizei len = 0;
    glGetObjectLabel(GL_TEXTURE, tex, sizeof(out), &len, out);
    EXPECT_EQ(std::string(out), std::string("T"));
    glGetObjectLabel(GL_PROGRAM, prog, sizeof(out), &len, out);
    EXPECT_EQ(std::string(out), std::string("P"));

    setCurrentContext(nullptr);
}

TEST_CASE("objectptrlabel_roundtrip") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    int sentinel = 0;
    const void* ptr = &sentinel;

    glObjectPtrLabel(ptr, -1, "sync-label");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    char out[64] = {0};
    GLsizei len = 0;
    glGetObjectPtrLabel(ptr, sizeof(out), &len, out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(std::string(out), std::string("sync-label"));

    // Null pointer -> GL_INVALID_VALUE.
    glObjectPtrLabel(nullptr, -1, "x");
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetObjectPtrLabel(nullptr, sizeof(out), &len, out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Clearing.
    glObjectPtrLabel(ptr, -1, nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glGetObjectPtrLabel(ptr, sizeof(out), &len, nullptr);
    EXPECT_EQ(len, 1); // empty label: 0 chars + 1 nul terminator

    setCurrentContext(nullptr);
}
