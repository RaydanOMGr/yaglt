#include "test_framework.hpp"

#include "glcompat/state/gl_state.hpp"

#include <cstdint>
#include <vector>

using namespace glcompat;

namespace {
// Records every state push so we can assert the tracker only emits changed
// state (SPEC §10: avoid redundant backend calls).
struct RecordingSink : GLStateSink {
    std::vector<std::pair<GLenum, bool>> caps;
    std::vector<GLObjectName> programs;
    int blendFuncCalls = 0;
    int blendEqCalls = 0;
    int depthFuncCalls = 0;
    int depthMaskCalls = 0;
    int stencilCalls = 0;
    int cullCalls = 0;
    int frontCalls = 0;
    int pixelCalls = 0;

    void enable(GLenum c) override { caps.emplace_back(c, true); }
    void disable(GLenum c) override { caps.emplace_back(c, false); }
    void enableIndexed(uint32_t, uint32_t) override {}
    void disableIndexed(uint32_t, uint32_t) override {}
    void useProgram(GLObjectName p) override { programs.push_back(p); }
    void bindProgramPipeline(uint32_t) override {}
    void blendFuncSeparate(uint32_t, uint32_t, uint32_t, uint32_t) override {
        ++blendFuncCalls;
    }
    void blendEquationSeparate(uint32_t, uint32_t) override { ++blendEqCalls; }
    void blendFuncSeparatei(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void blendEquationSeparatei(uint32_t, uint32_t, uint32_t) override {}
    void blendColor(float, float, float, float) override {}
    void depthFunc(GLenum) override { ++depthFuncCalls; }
    void depthMask(bool) override { ++depthMaskCalls; }
    void depthRange(double, double) override {}
    void stencilFunc(GLenum, GLint, GLuint) override { ++stencilCalls; }
    void stencilOp(GLenum, GLenum, GLenum) override { ++stencilCalls; }
    void stencilMask(GLuint) override { ++stencilCalls; }
    void stencilFuncSeparate(GLenum, GLenum, GLint, GLuint) override { ++stencilCalls; }
    void stencilOpSeparate(GLenum, GLenum, GLenum, GLenum) override { ++stencilCalls; }
    void stencilMaskSeparate(GLenum, GLuint) override { ++stencilCalls; }
    void colorMask(bool, bool, bool, bool) override {}
    void sampleCoverage(float, bool) override {}
    void primitiveRestart(uint32_t) override {}
    void hint(uint32_t, uint32_t) override {}
    void beginConditionalRender(uint32_t, uint32_t) override {}
    void endConditionalRender() override {}
    void cullFace(GLenum) override { ++cullCalls; }
    void frontFace(GLenum) override { ++frontCalls; }
    void pointSize(float) override {}
    void lineWidth(float) override {}
    void polygonOffset(float, float) override {}
    void polygonMode(uint32_t, uint32_t) override {}
    void sampleMaski(uint32_t, uint32_t) override {}
    void minSampleShading(float) override {}
    void provokingVertex(uint32_t) override {}
    void pointParameters(float, float, float, GLenum) override {}
    void clipControl(GLenum, GLenum) override {}
    void clampColor(uint32_t, uint32_t) override {}
    void pixelStorei(GLenum, GLint) override { ++pixelCalls; }
    void setViewport(int32_t, int32_t, int32_t, int32_t) override {}
    void setScissor(int32_t, int32_t, int32_t, int32_t) override {}
    void setViewportIndexed(uint32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void setScissorIndexed(uint32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void clearColor(float, float, float, float) override {}
    void clearDepth(double) override {}
    void bindBufferBase(uint32_t, uint32_t, uint32_t) override {}
    void bindBufferRange(uint32_t, uint32_t, uint32_t, intptr_t, intptr_t) override {}
    void bindVertexArray(uint32_t) override {}
    void enableVertexAttribArray(uint32_t) override {}
    void disableVertexAttribArray(uint32_t) override {}
    void vertexAttribPointer(uint32_t, int32_t, uint32_t, bool, int32_t, intptr_t) override {}
    void vertexAttribDivisor(uint32_t, uint32_t) override {}
    void bindBuffer(uint32_t, uint32_t) override {}
    void activeTexture(uint32_t) override {}
    void bindTexture(uint32_t, uint32_t) override {}
    void bindSampler(uint32_t, uint32_t) override {}
    void drawBuffers(int32_t, const uint32_t*) override {}
    void readBuffer(uint32_t) override {}
    void logicOp(uint32_t) override {}
};

} // namespace

TEST_CASE("state_tracker_initial_apply_is_noop") {
    GLStateTracker t;
    RecordingSink s;
    EXPECT_EQ(t.apply(s), 0);
    EXPECT_EQ(s.caps.size(), 0u);
    EXPECT_EQ(s.programs.size(), 0u);
    EXPECT_EQ(s.blendFuncCalls, 0);
}

TEST_CASE("state_tracker_emits_changed_caps_once") {
    GLStateTracker t;
    RecordingSink s;
    EXPECT_TRUE(t.setCapability(GL_BLEND, true));
    EXPECT_EQ(t.apply(s), 1);
    EXPECT_EQ(s.caps.size(), 1u);
    EXPECT_EQ(s.caps[0].first, GL_BLEND);
    EXPECT_TRUE(s.caps[0].second);

    // No change on repeat; apply emits nothing.
    EXPECT_FALSE(t.setCapability(GL_BLEND, true));
    EXPECT_EQ(t.apply(s), 0);
    EXPECT_EQ(s.caps.size(), 1u);

    // Disable flips it; apply emits a single disable.
    EXPECT_TRUE(t.setCapability(GL_BLEND, false));
    EXPECT_EQ(t.apply(s), 1);
    EXPECT_EQ(s.caps.size(), 2u);
    EXPECT_FALSE(s.caps[1].second);
}

TEST_CASE("state_tracker_blend_uses_defaults_without_change") {
    GLStateTracker t;
    RecordingSink s;
    // GL_ONE / GL_ZERO are the default blend factors: no change.
    EXPECT_FALSE(t.setBlendFunc(1 /*GL_ONE*/, 0 /*GL_ZERO*/));
    EXPECT_EQ(t.apply(s), 0);
    EXPECT_EQ(s.blendFuncCalls, 0);

    EXPECT_TRUE(t.setBlendFunc(0x0302 /*GL_SRC_ALPHA*/,
                               0x0303 /*GL_ONE_MINUS_SRC_ALPHA*/));
    EXPECT_EQ(t.apply(s), 1);
    EXPECT_EQ(s.blendFuncCalls, 1);
    EXPECT_EQ(s.blendEqCalls, 1);

    // Re-applying identical state is a no-op.
    EXPECT_EQ(t.apply(s), 0);
    EXPECT_EQ(s.blendFuncCalls, 1);
}

TEST_CASE("state_tracker_use_program_tracks_active_program") {
    GLStateTracker t;
    RecordingSink s;
    EXPECT_TRUE(t.useProgram(7));
    EXPECT_EQ(t.activeProgram(), 7u);
    EXPECT_EQ(t.apply(s), 1);
    EXPECT_EQ(s.programs.size(), 1u);
    EXPECT_EQ(s.programs[0], 7u);

    EXPECT_FALSE(t.useProgram(7)); // same -> no change
    EXPECT_EQ(t.apply(s), 0);
    EXPECT_EQ(s.programs.size(), 1u);
}

TEST_CASE("state_tracker_depth_state_pushed_together") {
    GLStateTracker t;
    RecordingSink s;
    EXPECT_TRUE(t.setDepthFunc(0x0203 /*GL_LEQUAL*/));
    EXPECT_TRUE(t.setDepthMask(false));
    EXPECT_EQ(t.apply(s), 1); // one "categories applied" for depth
    EXPECT_EQ(s.depthFuncCalls, 1);
    EXPECT_EQ(s.depthMaskCalls, 1);
    EXPECT_EQ(t.apply(s), 0);
}

TEST_CASE("state_tracker_reset_clears_everything") {
    GLStateTracker t;
    RecordingSink s;
    t.setCapability(GL_DEPTH_TEST, true);
    t.useProgram(3);
    t.setCullFace(0x0404 /*GL_FRONT*/);
    t.apply(s);

    t.reset();
    EXPECT_FALSE(t.isCapabilityEnabled(GL_DEPTH_TEST));
    EXPECT_EQ(t.activeProgram(), 0u);
    // After reset, applied state equals defaults, so nothing is re-pushed.
    RecordingSink s2;
    EXPECT_EQ(t.apply(s2), 0);
}
