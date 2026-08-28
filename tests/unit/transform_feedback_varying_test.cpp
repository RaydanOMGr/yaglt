#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstring>
#include <string>

using namespace glcompat;

namespace {
constexpr char kVertSrc[] =
    "#version 330 core\nvoid main(){gl_Position=vec4(0.0);}";
constexpr char kFragSrc[] =
    "#version 330 core\nout vec4 outColor;\nvoid main(){outColor=vec4(1.0);}";

// Build a linked program and seed `varyings` as the program's transform feedback
// capture list on the mock backend (the mock has no shader introspection, so the
// test supplies the list glTransformFeedbackVaryings would otherwise produce).
GLuint buildLinkedProgram(Context& ctx, MockBackend&,
                          const std::vector<MockProgram::MockTfVarying>& varyings) {
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
    mp->tfVaryings = varyings;
    return prog;
}
} // namespace

TEST_CASE("gettransformfeedbackvarying_returns_capture_info") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    MockProgram::MockTfVarying v;
    v.name = "vPosition";
    v.size = 1;
    v.type = 0x1406; // GL_FLOAT
    GLuint prog = buildLinkedProgram(ctx, backend, {v});
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    char name[64];
    GLsizei length = 0, size = 0;
    GLenum type = 0;
    glGetTransformFeedbackVarying(prog, 0, sizeof(name), &length, &size, &type, name);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(std::string(name), std::string("vPosition"));
    EXPECT_EQ(length, static_cast<GLsizei>(std::strlen("vPosition")));
    EXPECT_EQ(size, 1);
    EXPECT_EQ(type, static_cast<GLenum>(0x1406));

    setCurrentContext(nullptr);
}

TEST_CASE("gettransformfeedbackvarying_out_of_range_is_invalid_value") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint prog = buildLinkedProgram(ctx, backend, {}); // no varyings captured
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    char name[64];
    glGetTransformFeedbackVarying(prog, 0, sizeof(name), nullptr, nullptr, nullptr,
                                  name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}

TEST_CASE("gettransformfeedbackvarying_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Non-program object -> GL_INVALID_OPERATION.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    char name[64];
    glGetTransformFeedbackVarying(vs, 0, sizeof(name), nullptr, nullptr, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Negative bufSize -> GL_INVALID_VALUE.
    GLuint prog = buildLinkedProgram(ctx, backend, {});
    glGetTransformFeedbackVarying(prog, 0, -1, nullptr, nullptr, nullptr, name);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    setCurrentContext(nullptr);
}
