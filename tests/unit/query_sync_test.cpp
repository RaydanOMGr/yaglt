#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"
#include "src/backend/mock/mock_resources.hpp"

#include <cstdint>

using namespace glcompat;

TEST_CASE("query_gen_delete_is_validates_and_records") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Queries is Native in the mock profile.
    GLuint q = glGenQuery();
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(q, 0u);
    EXPECT_TRUE(glIsQuery(q));
    EXPECT_FALSE(glIsQuery(9999));

    glDeleteQuery(q);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_FALSE(glIsQuery(q));

    // genQueries bulk form.
    GLuint qs[3] = {0, 0, 0};
    glGenQueries(3, qs);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(qs[0], 0u);
    EXPECT_NE(qs[1], 0u);
    EXPECT_NE(qs[2], 0u);
    EXPECT_TRUE(glIsQuery(qs[2]));
    glDeleteQueries(3, qs);
    EXPECT_FALSE(glIsQuery(qs[0]));

    setCurrentContext(nullptr);
}

TEST_CASE("query_begin_end_forwards_to_backend_and_validates") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    glBeginQuery(GL_SAMPLES_PASSED, q);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    MockQuery* mq =
        static_cast<MockQuery*>(ctx.getQuery(q)->backend.get());
    EXPECT_EQ(mq->beginCalls, 1);
    EXPECT_EQ(mq->lastBeginTarget, static_cast<uint32_t>(GL_SAMPLES_PASSED));

    // Begin while already active is an error.
    glBeginQuery(GL_SAMPLES_PASSED, q);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // End targets the active query.
    glEndQuery(GL_SAMPLES_PASSED);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(mq->endCalls, 1);

    // End with no active query is an error.
    glEndQuery(GL_SAMPLES_PASSED);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    // Begin with an ungenerated id is an error.
    glBeginQuery(GL_SAMPLES_PASSED, 9999);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("query_indexed_requires_counter_target") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    // Occlusion queries cannot be indexed (only primitive/tf-written counters).
    glBeginQueryIndexed(GL_SAMPLES_PASSED, 0, q);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_FALSE(ctx.getQuery(q)->active);

    // The two counter targets are allowed.
    glBeginQueryIndexed(GL_PRIMITIVES_GENERATED, 0, q);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_TRUE(ctx.getQuery(q)->active);
    glEndQueryIndexed(GL_PRIMITIVES_GENERATED, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("query_parameter_queries_read_frontend_state") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    GLint cur = -1;
    glGetQueryiv(GL_SAMPLES_PASSED, GL_CURRENT_QUERY, &cur);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(cur, 0); // none active yet

    glBeginQuery(GL_SAMPLES_PASSED, q);
    glGetQueryiv(GL_SAMPLES_PASSED, GL_CURRENT_QUERY, &cur);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(cur, static_cast<GLint>(q));

    GLint bits = -1;
    glGetQueryiv(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &bits);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(bits, 0); // boolean-style query: no counter bits

    glEndQuery(GL_SAMPLES_PASSED);

    // null params -> INVALID_VALUE; unknown pname -> INVALID_ENUM.
    glGetQueryiv(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetQueryiv(GL_SAMPLES_PASSED, 0xDEAD, &bits);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("query_object_queries_read_cached_result") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    MockQuery* mq =
        static_cast<MockQuery*>(ctx.getQuery(q)->backend.get());
    mq->resultValue = 42;
    mq->hasResult = true;

    GLint ri = -1;
    glGetQueryObjectiv(q, GL_QUERY_RESULT, &ri);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ri, 42);

    GLuint ru = 0;
    glGetQueryObjectuiv(q, GL_QUERY_RESULT, &ru);
    EXPECT_EQ(ru, 42u);

    GLint64 ri64 = -1;
    glGetQueryObjecti64v(q, GL_QUERY_RESULT, &ri64);
    EXPECT_EQ(ri64, 42);

    GLuint64 ru64 = 0;
    glGetQueryObjectui64v(q, GL_QUERY_RESULT, &ru64);
    EXPECT_EQ(ru64, 42u);

    GLint avail = -1;
    glGetQueryObjectiv(q, GL_QUERY_RESULT_AVAILABLE, &avail);
    EXPECT_EQ(avail, static_cast<GLint>(GL_TRUE));

    mq->hasResult = false;
    glGetQueryObjectiv(q, GL_QUERY_RESULT_AVAILABLE, &avail);
    EXPECT_EQ(avail, static_cast<GLint>(GL_FALSE));

    // null params / unknown id / unknown pname error paths.
    glGetQueryObjectiv(q, GL_QUERY_RESULT, nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);
    glGetQueryObjectiv(9999, GL_QUERY_RESULT, &ri);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glGetQueryObjectiv(q, 0xDEAD, &ri);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);

    setCurrentContext(nullptr);
}

TEST_CASE("query_unsupported_reported_honestly") {
    MockBackend backend;
    backend.setCapability(Feature::Queries, FeatureSupport::Unsupported);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLuint q = glGenQuery();
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(q, 0u);
    glBeginQuery(GL_SAMPLES_PASSED, q);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);

    setCurrentContext(nullptr);
}

TEST_CASE("sync_fence_clientwait_getsync_lifecycle") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // SyncObjects is Native in the mock profile.
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_NE(sync, nullptr);
    EXPECT_TRUE(glIsSync(sync));

    // Unknown condition is rejected.
    GLsync bad = glFenceSync(0x1234, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    EXPECT_EQ(bad, nullptr);

    // Status starts unsignaled.
    GLint len = 0, val = 0;
    glGetSynciv(sync, GL_SYNC_STATUS, 1, &len, &val);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(len, 1);
    EXPECT_EQ(val, static_cast<GLint>(GL_UNSIGNALED));

    // Condition + flags round-trip.
    glGetSynciv(sync, GL_SYNC_CONDITION, 1, &len, &val);
    EXPECT_EQ(val, static_cast<GLint>(GL_SYNC_GPU_COMMANDS_COMPLETE));
    glGetSynciv(sync, GL_SYNC_FLAGS, 1, &len, &val);
    EXPECT_EQ(val, 0);

    // Client wait satisfies the fence.
    GLenum r = glClientWaitSync(sync, 0, 0);
    EXPECT_EQ(r, static_cast<GLenum>(GL_ALREADY_SIGNALED));

    glGetSynciv(sync, GL_SYNC_STATUS, 1, &len, &val);
    EXPECT_EQ(val, static_cast<GLint>(GL_SIGNALED));

    // Non-sync getSynciv / waitSync error paths.
    glGetSynciv(nullptr, GL_SYNC_STATUS, 1, &len, &val);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    glWaitSync(nullptr, 0, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_VALUE);

    // Delete removes it.
    glDeleteSync(sync);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_FALSE(glIsSync(sync));
    // Deleting a non-sync is a silent no-op.
    glDeleteSync(nullptr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    setCurrentContext(nullptr);
}

TEST_CASE("sync_unsupported_reported_honestly") {
    MockBackend backend;
    backend.setCapability(Feature::SyncObjects, FeatureSupport::Unsupported);
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    EXPECT_EQ(glGetError(), GL_INVALID_OPERATION);
    EXPECT_EQ(sync, nullptr);

    setCurrentContext(nullptr);
}
