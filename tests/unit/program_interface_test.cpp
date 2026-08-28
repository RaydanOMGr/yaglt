#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <cstring>

using namespace glcompat;

namespace {
constexpr GLenum GL_BOGUS_PNAME = 0xDEAD;
constexpr GLenum GL_BOGUS_IFACE = 0xBADC0DE;
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";

GLuint makeLinkedProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, kVertSrc);
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}
} // namespace

TEST_CASE("getprograminterface_active_resources_uses_backend_count") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    ProgramObject* po = ctx.getProgram(prog);
    auto* mp = static_cast<MockProgram*>(po->backend.get());
    mp->interfaceActiveResources = 3;

    GLint out = -1;
    glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_ACTIVE_RESOURCES, &out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(out, 3);
    EXPECT_EQ(mp->programInterfaceCalls, 1);
    EXPECT_EQ(mp->lastInterface, GL_UNIFORM);
    EXPECT_EQ(mp->lastInterfacePname, GL_ACTIVE_RESOURCES);

    setCurrentContext(nullptr);
}

TEST_CASE("getprograminterface_max_name_length_default_zero") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    ProgramObject* po = ctx.getProgram(prog);
    auto* mp = static_cast<MockProgram*>(po->backend.get());

    // Honest default: backends without introspection report 0 for MAX_* pnames.
    GLint out = -1;
    glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_MAX_RESOURCE_NAME_LENGTH, &out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(out, 0);

    // Explicit override is surfaced.
    mp->interfaceCounts[GL_MAX_RESOURCE_NAME_LENGTH] = 7;
    glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_MAX_RESOURCE_NAME_LENGTH, &out);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(out, 7);

    setCurrentContext(nullptr);
}

TEST_CASE("getprograminterface_invalid_interface_rejected") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    ProgramObject* po = ctx.getProgram(prog);
    auto* mp = static_cast<MockProgram*>(po->backend.get());

    GLint out = -1;
    glGetProgramInterfaceiv(prog, GL_BOGUS_IFACE, GL_ACTIVE_RESOURCES, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_EQ(mp->programInterfaceCalls, 0);

    setCurrentContext(nullptr);
}

TEST_CASE("getprograminterface_invalid_pname_rejected") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram();
    ProgramObject* po = ctx.getProgram(prog);
    auto* mp = static_cast<MockProgram*>(po->backend.get());

    GLint out = -1;
    glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_BOGUS_PNAME, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_EQ(mp->programInterfaceCalls, 0);

    // Null params -> GL_INVALID_VALUE.
    glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_ACTIVE_RESOURCES, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("getprograminterface_requires_linked_program") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Never linked -> GL_INVALID_OPERATION.
    GLuint prog = glCreateProgram();
    GLint out = -1;
    glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_ACTIVE_RESOURCES, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Non-program object -> GL_INVALID_OPERATION.
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glGetProgramInterfaceiv(buf, GL_UNIFORM, GL_ACTIVE_RESOURCES, &out);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
