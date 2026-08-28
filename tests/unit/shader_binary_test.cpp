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

// glProgramBinary loads a precompiled blob and marks the program linked. The
// authoritative frontend binary mirror round-trips through glGetProgramBinary
// and is queryable via glGetProgramiv(GL_PROGRAM_BINARY_LENGTH) (SPEC §7.3/§19.1).
TEST_CASE("program_binary_round_trip") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    const uint8_t blob[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
    ctx.programBinary(prog, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, sizeof(blob));
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    // A binary fully defines the program: it is now linked.
    EXPECT_EQ(ctx.getProgramiv(prog, GL_LINK_STATUS), 1);
    EXPECT_EQ(ctx.getProgram(prog)->linked, true);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_PROGRAM_BINARY_LENGTH),
              static_cast<GLint>(sizeof(blob)));

    // Retrieve via glGetProgramBinary (whole blob).
    GLsizei len = 0;
    GLenum fmt = 0;
    uint8_t out[16] = {0};
    ctx.getProgramBinary(prog, sizeof(out), &len, &fmt, out);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(len, static_cast<GLsizei>(sizeof(blob)));
    EXPECT_EQ(fmt, static_cast<GLenum>(GL_SHADER_BINARY_FORMAT_SPIR_V));
    EXPECT_EQ(std::memcmp(out, blob, sizeof(blob)), 0);
}

// glGetProgramBinary can report the length/format without copying (binary == null).
TEST_CASE("program_binary_query_only") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    const uint8_t blob[] = {1, 2, 3};
    ctx.programBinary(prog, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, sizeof(blob));

    GLsizei len = 0;
    GLenum fmt = 0;
    ctx.getProgramBinary(prog, 0, &len, &fmt, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(len, static_cast<GLsizei>(sizeof(blob)));
    EXPECT_EQ(fmt, static_cast<GLenum>(GL_SHADER_BINARY_FORMAT_SPIR_V));
}

// Retrieving a binary from a program that never received one is an error.
TEST_CASE("program_binary_get_without_load_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    GLsizei len = 0;
    ctx.getProgramBinary(prog, 0, &len, nullptr, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// bufSize smaller than the binary with a non-null destination is an error.
TEST_CASE("program_binary_buffer_too_small_is_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    const uint8_t blob[] = {1, 2, 3, 4};
    ctx.programBinary(prog, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, sizeof(blob));

    GLsizei len = 0;
    uint8_t out[2] = {0};
    ctx.getProgramBinary(prog, 2, &len, nullptr, out);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// glProgramBinary validation: unknown program / negative length / zero format.
TEST_CASE("program_binary_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    const uint8_t blob[] = {1, 2};
    ctx.programBinary(999, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, 2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    GLObjectName prog = ctx.createProgram();
    ctx.programBinary(prog, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, -1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    ctx.programBinary(prog, 0, blob, 2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

// glShaderBinary loads a binary into each named shader and marks them compiled.
TEST_CASE("shader_binary_marks_compiled") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName vs = ctx.createShader(GL_VERTEX_SHADER);
    GLObjectName fs = ctx.createShader(GL_FRAGMENT_SHADER);
    EXPECT_EQ(ctx.getShader(vs)->compiled, false);

    GLuint shaders[] = {vs, fs};
    const uint8_t blob[] = {0xCA, 0xFE};
    ctx.shaderBinary(2, shaders, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, sizeof(blob));
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    EXPECT_EQ(ctx.getShader(vs)->compiled, true);
    EXPECT_EQ(ctx.getShader(fs)->compiled, true);
    EXPECT_EQ(ctx.getShader(vs)->binary.size(), static_cast<size_t>(sizeof(blob)));
    EXPECT_EQ(ctx.getShader(vs)->binaryFormat,
              static_cast<uint32_t>(GL_SHADER_BINARY_FORMAT_SPIR_V));
}

// glShaderBinary validation: negative count / zero format / unknown shader.
TEST_CASE("shader_binary_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    const uint8_t blob[] = {1, 2};
    ctx.shaderBinary(-1, nullptr, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, 2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    ctx.shaderBinary(0, nullptr, 0, blob, 2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    GLuint bad[] = {999};
    ctx.shaderBinary(1, bad, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, 2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// Public dispatch surface reaches the new entry points.
TEST_CASE("shader_binary_gl_api_entry_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint prog = glCreateProgram();
    const uint8_t blob[] = {9, 8, 7};
    glProgramBinary(prog, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, sizeof(blob));
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(glGetProgramiv(prog, GL_PROGRAM_BINARY_LENGTH),
              static_cast<GLint>(sizeof(blob)));

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint shaders[] = {vs};
    glShaderBinary(1, shaders, GL_SHADER_BINARY_FORMAT_SPIR_V, blob, sizeof(blob));
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getShader(vs)->compiled, true);

    setCurrentContext(nullptr);
}
