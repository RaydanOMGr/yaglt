#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>
#include <string>

using namespace glcompat;

namespace {

// Builds a linked program behind the public API (mock compiles any non-empty
// source and links once a shader is attached).
GLuint makeLinkedProgram(Context& ctx) {
    (void)ctx;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}

} // namespace

TEST_CASE("bind_attrib_location_unknown_program_is_invalid_operation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    glBindAttribLocation(9999, 0, "a_position");
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("bind_attrib_location_records_on_program_object") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeLinkedProgram(ctx);
    glBindAttribLocation(prog, 3, "a_position");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    ProgramObject* p = ctx.getProgram(prog);
    EXPECT_NE(p, nullptr);
    if (p == nullptr) return;
    auto it = p->attribBindings.find("a_position");
    EXPECT_NE(it, p->attribBindings.end());
    if (it != p->attribBindings.end()) EXPECT_EQ(it->second, 3);

    setCurrentContext(nullptr);
}

TEST_CASE("bind_attrib_location_applied_before_link_to_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Bind BEFORE link, then link: the backend program sees the binding and the
    // bound location is authoritative for getAttribLocation (SPEC §7.3.7).
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glBindAttribLocation(prog, 5, "a_position");
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    MockProgram* mp =
        static_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());
    EXPECT_NE(mp, nullptr);
    if (mp == nullptr) return;
    auto it = mp->boundAttribLocations.find("a_position");
    EXPECT_NE(it, mp->boundAttribLocations.end());
    if (it != mp->boundAttribLocations.end()) EXPECT_EQ(it->second, 5);

    // The bound index must win over the mock's default auto-assigned location.
    EXPECT_EQ(ctx.getAttribLocation(prog, "a_position"), 5);

    setCurrentContext(nullptr);
}

TEST_CASE("bind_attrib_location_rebind_after_relink") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);

    glBindAttribLocation(prog, 2, "a_position");
    glLinkProgram(prog);
    EXPECT_EQ(ctx.getAttribLocation(prog, "a_position"), 2);

    // Re-bind and re-link: the new index takes effect.
    glBindAttribLocation(prog, 7, "a_position");
    glLinkProgram(prog);
    EXPECT_EQ(ctx.getAttribLocation(prog, "a_position"), 7);

    setCurrentContext(nullptr);
}
