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

// Captures the most recent debug callback invocation (single-message tests).
struct DebugCapt {
    int calls = 0;
    GLenum source = 0, type = 0, severity = 0;
    GLuint id = 0;
    std::string msg;
    const void* userParam = nullptr;
};
static DebugCapt g_capt;

static void debugCaptCb(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei length, const GLchar* message,
                        const void* userParam) {
    ++g_capt.calls;
    g_capt.source = source;
    g_capt.type = type;
    g_capt.id = id;
    g_capt.severity = severity;
    g_capt.msg = std::string(message, length > 0 ? static_cast<size_t>(length) : 0);
    g_capt.userParam = userParam;
}

// glDebugMessageInsert -> callback (SPEC §20.4) with passthrough args.
TEST_CASE("debug_message_callback_invoked") {
    auto backend = makeBackend();
    Context ctx(*backend);
    g_capt = DebugCapt{};
    ctx.debugMessageCallback(debugCaptCb, reinterpret_cast<void*>(0xDEAD));

    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 100u,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, "hello world");
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    EXPECT_EQ(g_capt.calls, 1);
    EXPECT_EQ(g_capt.source, GL_DEBUG_SOURCE_APPLICATION);
    EXPECT_EQ(g_capt.type, GL_DEBUG_TYPE_OTHER);
    EXPECT_EQ(g_capt.id, 100u);
    EXPECT_EQ(g_capt.severity, GL_DEBUG_SEVERITY_NOTIFICATION);
    EXPECT_EQ(g_capt.msg, std::string("hello world"));
    EXPECT_EQ(g_capt.userParam, reinterpret_cast<void*>(0xDEAD));
}

// glDebugMessageControl (SPEC §20.4) suppresses a disabled severity.
TEST_CASE("debug_message_control_filters_severity") {
    auto backend = makeBackend();
    Context ctx(*backend);
    g_capt = DebugCapt{};
    ctx.debugMessageCallback(debugCaptCb, nullptr);

    // Disable all APPLICATION/OTHER notifications.
    ctx.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                            GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 1u,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, "muted");
    EXPECT_EQ(g_capt.calls, 0); // filtered out, callback not invoked

    // Re-enable and confirm the same message now reaches the callback.
    ctx.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                            GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_TRUE);
    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 2u,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, "heard");
    EXPECT_EQ(g_capt.calls, 1);
    EXPECT_EQ(g_capt.msg, std::string("heard"));
}

// glDebugMessageInsert validation (SPEC §20.4).
TEST_CASE("debug_message_insert_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    // Source must be APPLICATION or THIRD_PARTY.
    ctx.debugMessageInsert(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, 1u,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, "x");
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    ctx.getError(); // clear

    // Buffer must not be null.
    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 1u,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

// glGetDebugMessageLog (SPEC §20.4) drains the FIFO ring.
TEST_CASE("debug_message_log_retrieves_inserted") {
    auto backend = makeBackend();
    Context ctx(*backend);
    g_capt = DebugCapt{};

    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 10u,
                           GL_DEBUG_SEVERITY_MEDIUM, -1, "perf-a");
    ctx.debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 11u,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, "note-b");
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    GLenum sources[4] = {}, types[4] = {}, sev[4] = {};
    GLuint ids[4] = {};
    GLsizei lengths[4] = {};
    char buf[256] = {};
    GLuint n = ctx.getDebugMessageLog(4, sizeof(buf), sources, types, ids,
                                      sev, lengths, buf);
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(sources[0], GL_DEBUG_SOURCE_APPLICATION);
    EXPECT_EQ(types[0], GL_DEBUG_TYPE_PERFORMANCE);
    EXPECT_EQ(ids[0], 10u);
    EXPECT_EQ(sev[0], GL_DEBUG_SEVERITY_MEDIUM);
    EXPECT_EQ(std::string(buf), std::string("perf-a"));

    // Ring is drained: a second call returns nothing.
    GLuint n2 = ctx.getDebugMessageLog(4, sizeof(buf), nullptr, nullptr, nullptr,
                                       nullptr, nullptr, nullptr);
    EXPECT_EQ(n2, 0u);
}

// glPushDebugGroup / glPopDebugGroup (SPEC §20.5) manage group depth.
TEST_CASE("debug_group_push_pop_depth") {
    auto backend = makeBackend();
    Context ctx(*backend);
    g_capt = DebugCapt{};
    ctx.debugMessageCallback(debugCaptCb, nullptr);

    EXPECT_EQ(ctx.debugGroupDepth(), 0u);

    ctx.pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 5u, -1, "outer");
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.debugGroupDepth(), 1u);

    ctx.pushDebugGroup(GL_DEBUG_SOURCE_OTHER, 6u, -1, "inner");
    EXPECT_EQ(ctx.debugGroupDepth(), 2u);

    ctx.popDebugGroup();
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.debugGroupDepth(), 1u);
    ctx.popDebugGroup();
    EXPECT_EQ(ctx.debugGroupDepth(), 0u);

    // Push source restriction: only APPLICATION/THIRD_PARTY/OTHER allowed.
    ctx.pushDebugGroup(GL_DEBUG_SOURCE_API, 7u, -1, "bad");
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    ctx.getError(); // clear

    // Pop with empty stack raises STACK_UNDERFLOW.
    ctx.popDebugGroup();
    EXPECT_EQ(ctx.getError(), GLError::StackUnderflow);

    // Each push/pop emits a PUSH_GROUP / POP_GROUP message.
    EXPECT_EQ(g_capt.calls, 4);
}

// Public dispatch reaches the frontend identically (SPEC §20.4/§20.5).
TEST_CASE("debug_message_via_public_dispatch") {
    auto backend = makeBackend();
    Context ctx(*backend);
    g_capt = DebugCapt{};
    glcompat::setCurrentContext(&ctx);
    glDebugMessageCallback(debugCaptCb, nullptr);

    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 42u,
                         GL_DEBUG_SEVERITY_NOTIFICATION, -1, "marker");
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(g_capt.calls, 1);
    EXPECT_EQ(g_capt.type, GL_DEBUG_TYPE_MARKER);
    EXPECT_EQ(g_capt.id, 42u);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1u, -1, "g");
    glPopDebugGroup();
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    glcompat::setCurrentContext(nullptr);
}
