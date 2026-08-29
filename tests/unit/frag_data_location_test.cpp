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

namespace {
// Simple linked program for exercising the bind path (mock compiles any
// non-empty source and links once a shader is attached).
GLuint makeBindProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, std::string("void main(){}"));
    glCompileShader(vs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    return prog;
}
} // namespace

TEST_CASE("bindfragdatalocation_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // A shader-object name must report GL_INVALID_OPERATION (SPEC §15.1.2).
    GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
    glBindFragDataLocation(sh, 0, "outColor");
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // An unknown (ungenerated) name must report GL_INVALID_VALUE.
    glBindFragDataLocation(9999, 0, "outColor");
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // A reserved "gl_" prefix reports GL_INVALID_OPERATION.
    GLuint prog = makeBindProgram();
    glBindFragDataLocation(prog, 0, "gl_FragColor");
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // colorNumber >= MAX_DRAW_BUFFERS (8) reports GL_INVALID_VALUE.
    glBindFragDataLocation(prog, 8, "outColor");
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // The indexed form rejects index > 1 with GL_INVALID_VALUE.
    glBindFragDataLocationIndexed(prog, 0, 2, "outColor");
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("bindfragdatalocation_records_on_program_object") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeBindProgram();
    glBindFragDataLocation(prog, 3, "outColor");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    ProgramObject* p = ctx.getProgram(prog);
    EXPECT_NE(p, nullptr);
    if (p == nullptr) return;
    auto it = p->fragDataBindings.find("outColor");
    EXPECT_NE(it, p->fragDataBindings.end());
    if (it != p->fragDataBindings.end()) EXPECT_EQ(it->second, 3);

    setCurrentContext(nullptr);
}

TEST_CASE("bindfragdatalocation_no_effect_before_link") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Bind AFTER link: the binding is only consulted at the next link
    // (SPEC §7.3.7), so getFragDataLocation still reports an unknown output.
    GLuint prog = makeBindProgram();
    glBindFragDataLocation(prog, 2, "outColor");
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(glGetFragDataLocation(prog, "outColor"), -1);

    setCurrentContext(nullptr);
}

TEST_CASE("bindfragdatalocation_applied_before_link_to_backend") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Bind BEFORE link, then link: the backend program records the binding and
    // getFragDataLocation returns the bound color number (SPEC §7.3.7).
    GLuint prog = makeBindProgram();
    glBindFragDataLocation(prog, 4, "outColor");
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);
    EXPECT_EQ(glGetFragDataLocation(prog, "outColor"), 4);

    MockProgram* mp =
        dynamic_cast<MockProgram*>(ctx.getProgram(prog)->backend.get());
    EXPECT_NE(mp, nullptr);
    if (mp == nullptr) return;
    auto it = mp->fragDataLocations.find("outColor");
    EXPECT_NE(it, mp->fragDataLocations.end());
    if (it != mp->fragDataLocations.end()) EXPECT_EQ(it->second, 4);

    setCurrentContext(nullptr);
}

TEST_CASE("bindfragdatalocationindexed_sets_dual_source_index") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeBindProgram();
    glBindFragDataLocationIndexed(prog, 0, 1, "outColor");
    glLinkProgram(prog);
    EXPECT_EQ(glGetProgramiv(prog, GL_LINK_STATUS), GL_TRUE);

    // Non-indexed bind is equivalent to index 0 for the color number.
    EXPECT_EQ(glGetFragDataLocation(prog, "outColor"), 0);
    // The indexed form records the dual-source index for getFragDataIndex.
    EXPECT_EQ(glGetFragDataIndex(prog, "outColor"), 1);

    setCurrentContext(nullptr);
}

TEST_CASE("bindfragdatalocation_rebind_after_relink") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = makeBindProgram();
    glBindFragDataLocation(prog, 1, "outColor");
    glLinkProgram(prog);
    EXPECT_EQ(glGetFragDataLocation(prog, "outColor"), 1);

    // Re-bind and re-link: the new color number takes effect.
    glBindFragDataLocation(prog, 6, "outColor");
    glLinkProgram(prog);
    EXPECT_EQ(glGetFragDataLocation(prog, "outColor"), 6);

    setCurrentContext(nullptr);
}
