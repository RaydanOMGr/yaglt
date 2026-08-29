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

namespace {
struct Capture {
    GLenum source = 0;
    GLenum type = 0;
    GLuint id = 0;
    GLenum severity = 0;
    std::string message;
    int calls = 0;
};
Capture g_capture;

void captureCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                     GLsizei length, const GLchar* message, const void* userParam) {
    ++g_capture.calls;
    g_capture.source = source;
    g_capture.type = type;
    g_capture.id = id;
    g_capture.severity = severity;
    g_capture.message = std::string(message, length > 0 ? static_cast<size_t>(length) : 0);
    (void)userParam;
}
} // namespace

// glDebugMessageInsert delivers to a registered callback for enabled messages.
TEST_CASE("debug_message_insert_invokes_callback") {
    g_capture = Capture{};
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.debugMessageCallback(&captureCallback, nullptr);

    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 42,
                          GL_DEBUG_SEVERITY_HIGH, -1, "boom");
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(g_capture.calls, 1);
    EXPECT_EQ(g_capture.source, static_cast<GLenum>(GL_DEBUG_SOURCE_APPLICATION));
    EXPECT_EQ(g_capture.type, static_cast<GLenum>(GL_DEBUG_TYPE_ERROR));
    EXPECT_EQ(g_capture.id, static_cast<GLuint>(42));
    EXPECT_EQ(g_capture.severity, static_cast<GLenum>(GL_DEBUG_SEVERITY_HIGH));
    EXPECT_EQ(g_capture.message, std::string("boom"));

    // The message is also retained for glGetDebugMessageLog.
    GLenum src = 0, ty = 0, sev = 0;
    GLuint id = 0;
    GLsizei len = 0;
    GLchar buf[16] = {0};
    GLuint n = ctx.getDebugMessageLog(1, sizeof(buf), &src, &ty, &id, &sev, &len, buf);
    EXPECT_EQ(n, static_cast<GLuint>(1));
    EXPECT_EQ(std::string(buf), std::string("boom"));
}

// glDebugMessageControl can silence a (type, severity) so the callback is skipped.
TEST_CASE("debug_message_control_silences_callback") {
    g_capture = Capture{};
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.debugMessageCallback(&captureCallback, nullptr);

    ctx.debugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0,
                           nullptr, GL_FALSE);
    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1,
                          GL_DEBUG_SEVERITY_HIGH, -1, "quiet");
    EXPECT_EQ(g_capture.calls, 0);

    // Re-enable errors and confirm delivery resumes.
    ctx.debugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0,
                           nullptr, GL_TRUE);
    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1,
                          GL_DEBUG_SEVERITY_HIGH, -1, "loud");
    EXPECT_EQ(g_capture.calls, 1);
}

// glDebugMessageInsert rejects non-application/third-party sources with INVALID_ENUM.
TEST_CASE("debug_message_insert_invalid_source") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.debugMessageCallback(&captureCallback, nullptr);

    ctx.debugMessageInsert(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, 1,
                          GL_DEBUG_SEVERITY_HIGH, -1, "nope");
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

// Push/pop debug groups maintain a stack depth and emit group messages.
TEST_CASE("debug_group_push_pop_round_trip") {
    g_capture = Capture{};
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.debugMessageCallback(&captureCallback, nullptr);

    EXPECT_EQ(ctx.debugGroupDepth(), static_cast<size_t>(0));
    ctx.pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 7, -1, "scope");
    EXPECT_EQ(ctx.debugGroupDepth(), static_cast<size_t>(1));
    EXPECT_EQ(g_capture.type, static_cast<GLenum>(GL_DEBUG_TYPE_PUSH_GROUP));

    ctx.popDebugGroup();
    EXPECT_EQ(ctx.debugGroupDepth(), static_cast<size_t>(0));
    EXPECT_EQ(g_capture.type, static_cast<GLenum>(GL_DEBUG_TYPE_POP_GROUP));

    // Pop on an empty stack is a STACK_UNDERFLOW.
    ctx.popDebugGroup();
    EXPECT_EQ(ctx.getError(), GLError::StackUnderflow);
}

// The public dispatch reaches the same paths.
TEST_CASE("debug_api_via_public_dispatch") {
    g_capture = Capture{};
    auto backend = makeBackend();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    glDebugMessageCallback(&captureCallback, nullptr);
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 9,
                        GL_DEBUG_SEVERITY_LOW, -1, "perf");
    EXPECT_EQ(g_capture.calls, 1);
    EXPECT_EQ(g_capture.type, static_cast<GLenum>(GL_DEBUG_TYPE_PERFORMANCE));

    glPopDebugGroup(); // empty stack
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_STACK_UNDERFLOW));

    glcompat::setCurrentContext(nullptr);
}
