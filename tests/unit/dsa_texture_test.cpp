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
// correctness (SPEC §2.1: each unit must end up bound to the right texture,
// including DSA bindings that do not touch the active-texture selector).
namespace {
struct UnitRecordingSink : GLStateSink {
    std::vector<uint32_t> activeTextures;
    struct Bind { uint32_t unit; uint32_t target; uint32_t texture; };
    std::vector<Bind> binds;
    uint32_t currentUnit = GL_TEXTURE0;
    void enable(GLenum) override {}
    void disable(GLenum) override {}
    void enableIndexed(uint32_t, uint32_t) override {}
    void disableIndexed(uint32_t, uint32_t) override {}
    void useProgram(uint32_t) override {}
    void bindProgramPipeline(uint32_t) override {}
    void blendFuncSeparate(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void blendEquationSeparate(uint32_t, uint32_t) override {}
    void blendFuncSeparatei(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void blendEquationSeparatei(uint32_t, uint32_t, uint32_t) override {}
    void blendColor(float, float, float, float) override {}
    void depthFunc(GLenum) override {}
    void depthMask(bool) override {}
    void depthRange(double, double) override {}
    void depthRangeIndexed(uint32_t, double, double) override {}
    void stencilFunc(GLenum, GLint, GLuint) override {}
    void stencilOp(GLenum, GLenum, GLenum) override {}
    void stencilMask(GLuint) override {}
    void stencilFuncSeparate(GLenum, GLenum, GLint, GLuint) override {}
    void stencilOpSeparate(GLenum, GLenum, GLenum, GLenum) override {}
    void stencilMaskSeparate(GLenum, GLuint) override {}
    void colorMask(bool, bool, bool, bool) override {}
    void colorMaski(uint32_t, bool, bool, bool, bool) override {}
    void sampleCoverage(float, bool) override {}
    void primitiveRestart(uint32_t) override {}
    void hint(uint32_t, uint32_t) override {}
    void beginConditionalRender(uint32_t, uint32_t) override {}
    void endConditionalRender() override {}
    void cullFace(GLenum) override {}
    void frontFace(GLenum) override {}
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
    void pixelStorei(GLenum, GLint) override {}
    void setViewport(int32_t, int32_t, int32_t, int32_t) override {}
    void setScissor(int32_t, int32_t, int32_t, int32_t) override {}
    void setViewportIndexed(uint32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void setScissorIndexed(uint32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void clearColor(float, float, float, float) override {}
    void clearDepth(double) override {}
    void clearStencil(int) override {}
    void bindBufferBase(uint32_t, uint32_t, uint32_t) override {}
    void bindBufferRange(uint32_t, uint32_t, uint32_t, intptr_t, intptr_t) override {}
    void bindVertexArray(uint32_t) override {}
    void enableVertexAttribArray(uint32_t) override {}
    void disableVertexAttribArray(uint32_t) override {}
    void vertexAttribPointer(uint32_t, int32_t, uint32_t, bool, int32_t, intptr_t) override {}
    void bindBuffer(uint32_t, uint32_t) override {}
    void activeTexture(uint32_t unit) override {
        activeTextures.push_back(unit);
        currentUnit = unit;
    }
    void bindTexture(uint32_t target, uint32_t texture) override {
        binds.push_back({currentUnit, target, texture});
    }
    void bindSampler(uint32_t, uint32_t) override {}
    void vertexAttribDivisor(uint32_t, uint32_t) override {}
    void drawBuffers(int32_t, const uint32_t*) override {}
    void readBuffer(uint32_t) override {}
    void logicOp(uint32_t) override {}
};
} // namespace

TEST_CASE("bind_texture_unit_binds_to_specific_unit") {
    UnitRecordingSink sink;
    GLStateTracker t;
    t.setTextureUnitBinding(3, GL_TEXTURE_2D, 10);
    EXPECT_EQ(t.apply(sink), 1);
    EXPECT_TRUE(sink.binds.size() == 1u);
    EXPECT_EQ(sink.binds[0].unit, GL_TEXTURE0 + 3);
    EXPECT_EQ(sink.binds[0].target, GL_TEXTURE_2D);
    EXPECT_EQ(sink.binds[0].texture, 10u);
    // The active-texture selector must have been switched to unit 3.
    EXPECT_EQ(sink.activeTextures.size(), 1u);
    EXPECT_EQ(sink.activeTextures[0], GL_TEXTURE0 + 3);
}

TEST_CASE("bind_texture_unit_unbind_clears_unit") {
    UnitRecordingSink sink;
    GLStateTracker t;
    t.setTextureUnitBinding(1, GL_TEXTURE_2D, 10);
    EXPECT_EQ(t.apply(sink), 1);
    // Unbind by binding 0 to the unit.
    t.setTextureUnitBinding(1, GL_TEXTURE_2D, 0);
    EXPECT_EQ(t.apply(sink), 1);
    EXPECT_TRUE(sink.binds.size() == 2u);
    EXPECT_EQ(sink.binds[1].unit, GL_TEXTURE0 + 1);
    EXPECT_EQ(sink.binds[1].texture, 0u);
}

TEST_CASE("bind_textures_binds_consecutive_units") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName t0 = ctx.genTexture();
    GLObjectName t1 = ctx.genTexture();
    GLObjectName t2 = ctx.genTexture();
    // Each texture is bound to the target it was created with (SPEC §8.1): bind
    // t1 to GL_TEXTURE_3D first so the multi-bind must honor that target.
    ctx.bindTexture(GL_TEXTURE_3D, t1);
    GLObjectName arr[3] = {t0, t1, t2};
    ctx.bindTextures(0, 3, arr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(0, GL_TEXTURE_2D), t0);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(1, GL_TEXTURE_3D), t1);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(1, GL_TEXTURE_2D), 0u);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(2, GL_TEXTURE_2D), t2);
}

TEST_CASE("bind_textures_null_array_resets_range") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName arr[2] = {ctx.genTexture(), ctx.genTexture()};
    ctx.bindTextures(0, 2, arr);
    // A null array resets every target of each touched unit to its default.
    ctx.bindTextures(0, 2, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(0, GL_TEXTURE_2D), 0u);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(1, GL_TEXTURE_2D), 0u);
}

TEST_CASE("bind_texture_unit_out_of_range_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTextureUnit(ctx.state().maxCombinedTextureUnits(), tex);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("bind_texture_unit_ungenerated_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    ctx.bindTextureUnit(0, 12345);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("bind_textures_out_of_range_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName arr[5] = {0};
    uint32_t max = ctx.state().maxCombinedTextureUnits();
    // SPEC §8.1: first + count past the unit count is GL_INVALID_OPERATION.
    ctx.bindTextures(max - 2, 5, arr); // (max-2)+5 > max
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("bind_textures_negative_count_invalid_value") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName arr[1] = {ctx.genTexture()};
    ctx.bindTextures(0, -1, arr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);
}

TEST_CASE("bind_textures_ungenerated_invalid_operation") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName good = ctx.genTexture();
    GLObjectName arr[2] = {good, 4242}; // second is ungenerated
    ctx.bindTextures(0, 2, arr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    // SPEC §8.1: the valid entry still binds, the invalid unit is unchanged.
    EXPECT_EQ(ctx.boundTextureForUnitTarget(0, GL_TEXTURE_2D), good);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(1, GL_TEXTURE_2D), 0u);
}

TEST_CASE("dsa_unsupported_reports_invalid_operation") {
    auto backend = makeBackend();
    // DirectStateAccess is Emulated in the mock; flip it to Unsupported and the
    // DSA entry points must refuse honestly.
    backend->setCapability(Feature::DirectStateAccess, FeatureSupport::Unsupported);
    Context ctx(*backend);
    GLObjectName tex = ctx.genTexture();
    ctx.bindTextureUnit(0, tex);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    GLObjectName arr[1] = {tex};
    ctx.bindTextures(0, 1, arr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("gl_api_bind_texture_unit_and_bind_textures_surface") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint t0 = 0, t1 = 0, t2 = 0;
    glGenTextures(1, &t0);
    glGenTextures(1, &t1);
    glGenTextures(1, &t2);

    glBindTextureUnit(2, t1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(2, GL_TEXTURE_2D), t1);

    GLuint arr[3] = {t0, t1, t2};
    glBindTextures(0, 3, arr);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(0, GL_TEXTURE_2D), t0);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(1, GL_TEXTURE_2D), t1);
    EXPECT_EQ(ctx.boundTextureForUnitTarget(2, GL_TEXTURE_2D), t2);

    setCurrentContext(nullptr);
}

TEST_CASE("dsa_binding_flushes_to_mock_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);
    GLObjectName t0 = ctx.genTexture();
    ctx.bindTextureUnit(1, t0);
    ctx.flushState();
    EXPECT_EQ(backend->activeTextureCalls, 1);
    EXPECT_EQ(backend->lastActiveTexture, GL_TEXTURE0 + 1);
    EXPECT_EQ(backend->bindTextureCalls, 1);
    EXPECT_EQ(backend->lastTexBindTarget, GL_TEXTURE_2D);
    EXPECT_EQ(backend->lastBindTexture, t0);
}
