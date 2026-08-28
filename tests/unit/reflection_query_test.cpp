#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

#include <cstdint>
#include <cstring>
#include <string>

using namespace glcompat;

namespace {
constexpr GLenum GL_BOGUS_PNAME = 0xDEAD;
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
} // namespace

TEST_CASE("getshadersource_returns_concatenated_source") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), GL_TRUE);

    // Read the source back through glGetShaderSource.
    char buf[128];
    GLsizei len = 0;
    glGetShaderSource(vs, sizeof(buf), &len, buf);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(static_cast<size_t>(len), std::strlen(kVertSrc));
    EXPECT_EQ(std::string(buf), std::string(kVertSrc));

    // Negative bufSize -> GL_INVALID_VALUE; source untouched.
    char out[16] = {0};
    glGetShaderSource(vs, -1, &len, out);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Non-shader object -> GL_INVALID_OPERATION.
    GLuint prog = glCreateProgram();
    glGetShaderSource(prog, sizeof(buf), &len, buf);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("getattachedshaders_reports_attached_names") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, kVertSrc);
    glShaderSource(fs, kVertSrc);
    glCompileShader(vs);
    glCompileShader(fs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);

    GLsizei count = 0;
    GLuint shaders[4] = {0, 0, 0, 0};
    glGetAttachedShaders(prog, 4, &count, shaders);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(shaders[0], vs);
    EXPECT_EQ(shaders[1], fs);

    // maxCount clamps the written count; count still reports the true total.
    GLuint one[1] = {0};
    glGetAttachedShaders(prog, 1, &count, one);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(one[0], vs);

    // Negative maxCount -> GL_INVALID_VALUE.
    glGetAttachedShaders(prog, -1, &count, shaders);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Non-program object -> GL_INVALID_OPERATION.
    glGetAttachedShaders(vs, 4, &count, shaders);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("getbufferpointerv_returns_mapped_pointer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Unmapped buffer: pointer is nullptr, no error.
    void* ptr = reinterpret_cast<void*>(0x1);
    glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BUFFER_MAP_POINTER, &ptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ptr, nullptr);

    // Map the buffer and read back the pointer.
    void* mapBase = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    EXPECT_NE(mapBase, nullptr);
    glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BUFFER_MAP_POINTER, &ptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ptr, mapBase);

    // Unknown pname -> GL_INVALID_ENUM; null params -> GL_INVALID_VALUE.
    glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BOGUS_PNAME, &ptr);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BUFFER_MAP_POINTER, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Invalid target -> GL_INVALID_ENUM.
    glGetBufferPointerv(GL_BOGUS_PNAME, GL_BUFFER_MAP_POINTER, &ptr);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("getnamedbufferpointerv_dsa_returns_mapped_pointer") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    void* mapBase = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    EXPECT_NE(mapBase, nullptr);

    // DSA query (capability-gated by DirectStateAccess, supported in the mock).
    void* ptr = nullptr;
    glGetNamedBufferPointerv(buf, GL_BUFFER_MAP_POINTER, &ptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ptr, mapBase);

    // Ungenerated name -> GL_INVALID_OPERATION.
    glGetNamedBufferPointerv(9999, GL_BUFFER_MAP_POINTER, &ptr);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Unknown pname -> GL_INVALID_ENUM.
    glGetNamedBufferPointerv(buf, GL_BOGUS_PNAME, &ptr);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}
