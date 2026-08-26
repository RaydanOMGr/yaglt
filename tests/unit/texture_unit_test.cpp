#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/state/gl_state.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// Records every activeTexture / bindTexture push so we can assert per-unit
// correctness (SPEC §2.1: each unit must end up bound to the right texture).
namespace {
struct UnitRecordingSink : GLStateSink {
    std::vector<uint32_t> activeTextures;
    struct Bind { uint32_t unit; uint32_t target; uint32_t texture; };
    std::vector<Bind> binds;
    uint32_t currentUnit = GL_TEXTURE0;
    void enable(GLenum) override {}
    void disable(GLenum) override {}
    void useProgram(uint32_t) override {}
    void blendFuncSeparate(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void blendEquationSeparate(uint32_t, uint32_t) override {}
    void blendColor(float, float, float, float) override {}
    void depthFunc(GLenum) override {}
    void depthMask(bool) override {}
    void depthRange(double, double) override {}
    void stencilFunc(GLenum, GLint, GLuint) override {}
    void stencilOp(GLenum, GLenum, GLenum) override {}
    void stencilMask(GLuint) override {}
    void cullFace(GLenum) override {}
    void frontFace(GLenum) override {}
    void pointSize(float) override {}
    void lineWidth(float) override {}
    void polygonOffset(float, float) override {}
    void pixelStorei(GLenum, GLint) override {}
    void setViewport(int32_t, int32_t, int32_t, int32_t) override {}
    void setScissor(int32_t, int32_t, int32_t, int32_t) override {}
    void clearColor(float, float, float, float) override {}
    void clearDepth(double) override {}
    void bindBufferBase(uint32_t, uint32_t, uint32_t) override {}
    void bindBufferRange(uint32_t, uint32_t, uint32_t, intptr_t, intptr_t) override {}
    void bindVertexArray(uint32_t) override {}
    void enableVertexAttribArray(uint32_t) override {}
    void disableVertexAttribArray(uint32_t) override {}
    void vertexAttribPointer(uint32_t, int32_t, uint32_t, bool, int32_t, intptr_t) override {}
    void activeTexture(uint32_t unit) override {
        activeTextures.push_back(unit);
        currentUnit = unit;
    }
    void bindTexture(uint32_t target, uint32_t texture) override {
        binds.push_back({currentUnit, target, texture});
    }
    void bindSampler(uint32_t, uint32_t) override {}
    void drawBuffers(int32_t, const uint32_t*) override {}
    void readBuffer(uint32_t) override {}
};
} // namespace

TEST_CASE("active_texture_out_of_range_is_invalid_enum") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.activeTexture(GL_TEXTURE0 + 9999);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
    ctx.activeTexture(0x1234); // not a GL_TEXTURE0 base
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("active_texture_selects_unit_and_query_reflects_it") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.activeTexture(GL_TEXTURE0 + 3);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int v = 0;
    ctx.getIntegerv(GL_ACTIVE_TEXTURE, &v);
    EXPECT_EQ(static_cast<GLenum>(v), GL_TEXTURE0 + 3);
}

TEST_CASE("max_combined_texture_image_units_query") {
    auto backend = makeBackend();
    Context ctx(*backend);
    int v = 0;
    ctx.getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &v);
    EXPECT_EQ(v, static_cast<int>(GLStateTracker::kMaxTextureUnits));
    ctx.getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &v);
    EXPECT_EQ(v, static_cast<int>(GLStateTracker::kMaxTextureUnits));
}

TEST_CASE("bind_texture_uses_active_unit") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.activeTexture(GL_TEXTURE0 + 2);
    ctx.bindTexture(GL_TEXTURE_2D, tex);

    // No backend push yet (lazy at flush). Flush and observe the sink.
    ctx.flushState();
    EXPECT_EQ(backend->activeTextureCalls, 1);
    EXPECT_EQ(backend->lastActiveTexture, GL_TEXTURE0 + 2);
    EXPECT_EQ(backend->bindTextureCalls, 1);
    EXPECT_EQ(backend->lastTexBindTarget, GL_TEXTURE_2D);
    EXPECT_EQ(backend->lastBindTexture, tex);
}

TEST_CASE("texture_binding_change_only_pushes_once") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.flushState();
    int afterFirst = backend->bindTextureCalls;
    EXPECT_EQ(afterFirst, 1);

    // Re-binding the same texture on the same unit must not push again.
    ctx.bindTexture(GL_TEXTURE_2D, tex);
    ctx.activeTexture(GL_TEXTURE0);
    ctx.flushState();
    EXPECT_EQ(backend->bindTextureCalls, afterFirst);
}

TEST_CASE("multiple_units_push_distinct_bindings") {
    UnitRecordingSink sink;
    GLStateTracker t;

    t.setActiveTexture(GL_TEXTURE0);
    t.setTextureBinding(GL_TEXTURE_2D, 10);
    t.setActiveTexture(GL_TEXTURE0 + 1);
    t.setTextureBinding(GL_TEXTURE_2D, 20);

    EXPECT_EQ(t.apply(sink), 1); // one applied category (texture units)

    // Unit 0 -> texture 10, unit 1 -> texture 20, each on its own unit.
    EXPECT_EQ(sink.binds.size(), 2u);
    EXPECT_EQ(sink.binds[0].unit, GL_TEXTURE0);
    EXPECT_EQ(sink.binds[0].texture, 10u);
    EXPECT_EQ(sink.binds[1].unit, GL_TEXTURE0 + 1);
    EXPECT_EQ(sink.binds[1].texture, 20u);

    // A second apply with no further changes must not push anything.
    EXPECT_EQ(t.apply(sink), 0);
    size_t bindsAfter = sink.binds.size();
    EXPECT_EQ(t.apply(sink), 0);
    EXPECT_EQ(sink.binds.size(), bindsAfter);
}

TEST_CASE("unbinding_texture_pushes_zero_binding") {
    UnitRecordingSink sink;
    GLStateTracker t;

    t.setActiveTexture(GL_TEXTURE0);
    t.setTextureBinding(GL_TEXTURE_2D, 10);
    EXPECT_EQ(t.apply(sink), 1);
    EXPECT_EQ(sink.binds.back().texture, 10u);

    // Remove the binding (as deleteTexture would): next apply must push a 0.
    t.clearTextureBinding(10);
    EXPECT_EQ(t.apply(sink), 1);
    EXPECT_EQ(sink.binds.back().unit, GL_TEXTURE0);
    EXPECT_EQ(sink.binds.back().texture, 0u);
}

TEST_CASE("gl_api_active_texture_and_bind_texture_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint t0 = 0, t1 = 0;
    glGenTextures(1, &t0);
    glGenTextures(1, &t1);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, t1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    int active = 0;
    ctx.getIntegerv(GL_ACTIVE_TEXTURE, &active);
    EXPECT_EQ(static_cast<GLenum>(active), GL_TEXTURE0 + 1);
    EXPECT_EQ(ctx.boundTextureForTarget(GL_TEXTURE_2D), t1);

    setCurrentContext(nullptr);
}
