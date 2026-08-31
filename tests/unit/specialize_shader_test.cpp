#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// glSpecializeShader (SPEC §7.4) specializes a SPIR-V shader's constants and
// compiles it. The backend records the entry point, constant count, indices and
// values; the frontend reports COMPILE_STATUS true on success.
TEST_CASE("specialize_shader_records_and_compiles") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName vs = ctx.createShader(GL_VERTEX_SHADER);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    const uint32_t idx[] = {0, 3};
    const uint32_t val[] = {7, 11};
    ctx.specializeShader(vs, "main", 2, idx, val);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* ms = dynamic_cast<MockShader*>(ctx.getShader(vs)->backend.get());
    EXPECT_NE(ms, nullptr);
    EXPECT_EQ(ms->specializeCalls, 1);
    EXPECT_EQ(ms->lastEntryPoint, std::string("main"));
    EXPECT_EQ(ms->lastNumConstants, static_cast<uint32_t>(2));
    EXPECT_TRUE((ms->lastConstantIndex == std::vector<uint32_t>{0, 3}));
    EXPECT_TRUE((ms->lastConstantValue == std::vector<uint32_t>{7, 11}));

    // A successful specialize marks the shader compiled.
    EXPECT_EQ(ctx.getShader(vs)->compiled, true);
    EXPECT_EQ(ctx.getShaderiv(vs, GL_COMPILE_STATUS), 1);
}

// Unknown shader name must raise INVALID_OPERATION and not touch backend state.
TEST_CASE("specialize_shader_unknown_object_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    const uint32_t idx[] = {0};
    const uint32_t val[] = {1};
    ctx.specializeShader(999u, "main", 1, idx, val);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// The public glSpecializeShader dispatch reaches the backend identically.
TEST_CASE("specialize_shader_via_public_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_NE(vs, 0u);

    const uint32_t idx[] = {1};
    const uint32_t val[] = {42};
    glSpecializeShader(vs, "vs_main", 1, idx, val);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    auto* ms = dynamic_cast<MockShader*>(ctx.getShader(vs)->backend.get());
    EXPECT_NE(ms, nullptr);
    EXPECT_EQ(ms->specializeCalls, 1);
    EXPECT_EQ(ms->lastEntryPoint, std::string("vs_main"));
    EXPECT_TRUE((ms->lastConstantValue == std::vector<uint32_t>{42}));
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), 1);

    glcompat::setCurrentContext(nullptr);
}

// glReleaseShaderCompiler (SPEC §7.1) is a no-op hint: it never produces a GL
// error and leaves the context usable (a shader can still be compiled after it).
TEST_CASE("release_shader_compiler_is_noop_and_keeps_context_usable") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glReleaseShaderCompiler();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // The (Context) method form also never errors.
    ctx.releaseShaderCompiler();
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    // Compiler remains usable: a fresh shader can still be created and compiled.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_NE(vs, 0u);
    const char* src = "#version 110\nvoid main() { gl_Position = vec4(0.0); }";
    glShaderSource(vs, 1, &src, nullptr);
    glCompileShader(vs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetShaderiv(vs, GL_COMPILE_STATUS), 1);

    setCurrentContext(nullptr);
}
