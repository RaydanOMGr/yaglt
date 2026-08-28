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

// Build a normal linked program through the mock backend.
static GLObjectName makeLinkedProgram(Context& ctx) {
    GLObjectName vs = ctx.createShader(GL_VERTEX_SHADER);
    ctx.shaderSource(vs, "void main(){}");
    ctx.compileShader(vs);
    GLObjectName prog = ctx.createProgram();
    ctx.attachShader(prog, vs);
    ctx.linkProgram(prog);
    return prog;
}

// glProgramParameteri(prog, GL_PROGRAM_SEPARABLE, TRUE) before linking marks the
// program separable; the flag is queryable via glGetProgramiv (SPEC §7.3/§7.4.2).
TEST_CASE("pp_param_separable_before_link_sets_flag") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_PROGRAM_SEPARABLE), 0);

    ctx.programParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getProgram(prog)->separable, true);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_PROGRAM_SEPARABLE), 1);
}

// Setting GL_PROGRAM_SEPARABLE after the program is linked is an error (SPEC §7.3).
TEST_CASE("pp_param_separable_after_link_is_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = makeLinkedProgram(ctx);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_LINK_STATUS), 1);

    ctx.programParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    EXPECT_EQ(ctx.getProgram(prog)->separable, false);
}

// GL_PROGRAM_BINARY_RETRIEVABLE_HINT may be set before or after linking.
TEST_CASE("pp_param_binary_retrievable_hint_any_time") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    ctx.programParameteri(prog, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getProgram(prog)->binaryRetrievableHint, true);

    // Still settable after linking.
    GLObjectName linked = makeLinkedProgram(ctx);
    ctx.programParameteri(linked, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getProgram(linked)->binaryRetrievableHint, true);
}

// Unknown pname -> GL_INVALID_ENUM; non-program -> GL_INVALID_OPERATION.
TEST_CASE("pp_param_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName prog = ctx.createProgram();
    ctx.programParameteri(prog, 0xDEAD, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);

    ctx.programParameteri(999, GL_PROGRAM_SEPARABLE, GL_TRUE);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

// glProgramParameteri must be reachable through the public dispatch surface.
TEST_CASE("pp_param_gl_api_entry_point") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint prog = glCreateProgram();
    glProgramParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(glGetProgramiv(prog, GL_PROGRAM_SEPARABLE), 1);

    setCurrentContext(nullptr);
}
