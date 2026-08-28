#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <string>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
constexpr char kFragSrc[] =
    "#version 330 core\nout vec4 outColor;\nvoid main(){outColor=vec4(1.0);}";

// Build a linked program and register `outputs` as active fragment outputs on the
// mock backend program (the mock has no shader introspection, so the test seeds
// the locations it would otherwise learn from the driver).
GLuint buildLinkedProgram(Context& ctx, MockBackend& backend,
                          const std::vector<std::pair<std::string, int>>& outputs) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, kVertSrc);
    glShaderSource(fs, kFragSrc);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    ProgramObject* po = ctx.getProgram(prog);
    auto* mp = dynamic_cast<MockProgram*>(po->backend.get());
    for (const auto& kv : outputs) mp->fragDataLocations[kv.first] = kv.second;
    return prog;
}
} // namespace

TEST_CASE("getfragdatalocation_returns_assigned_location") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = buildLinkedProgram(ctx, backend, {{"outColor", 0}});
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Registered output returns its assigned location.
    EXPECT_EQ(glGetFragDataLocation(prog, "outColor"), 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Unknown / inactive output -> -1 (no error).
    EXPECT_EQ(glGetFragDataLocation(prog, "doesNotExist"), -1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("getfragdataindex_returns_assigned_index") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = buildLinkedProgram(ctx, backend, {{"outColor", 0}});
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Mock returns the dual-source index (here 1) for the registered output.
    MockProgram* mp = dynamic_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());
    mp->fragDataIndices["outColor"] = 1;
    EXPECT_EQ(glGetFragDataIndex(prog, "outColor"), 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    EXPECT_EQ(glGetFragDataIndex(prog, "missing"), -1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("getfragdatalocation_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Non-program object -> GL_INVALID_OPERATION.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    EXPECT_EQ(glGetFragDataLocation(vs, "outColor"), -1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(glGetFragDataIndex(vs, "outColor"), -1);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}
