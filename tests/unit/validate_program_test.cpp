#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <string>

using namespace glcompat;

namespace {

GLuint makeLinkedProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}

} // namespace

TEST_CASE("validate_program_records_backend_and_sets_status") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    EXPECT_EQ(glGetProgramiv(prog, GL_VALIDATE_STATUS), GL_FALSE);
    glValidateProgram(prog);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetProgramiv(prog, GL_VALIDATE_STATUS), GL_TRUE);
    auto* mp = dynamic_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());
    EXPECT_NE(mp, nullptr);
    EXPECT_EQ(mp->validateCalls, 1);

    setCurrentContext(nullptr);
}

TEST_CASE("validate_program_unknown_object_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glValidateProgram(4242);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(glGetProgramiv(4242, GL_VALIDATE_STATUS), 0);

    setCurrentContext(nullptr);
}

TEST_CASE("validate_program_via_context_method") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    ctx.validateProgram(prog);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_VALIDATE_STATUS), 1);

    setCurrentContext(nullptr);
}
