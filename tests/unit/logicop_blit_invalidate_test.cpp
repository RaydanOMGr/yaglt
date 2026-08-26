#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

TEST_CASE("glLogicOp_records_pushed_mode_and_skips_redundant_push") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Default logic op is GL_COPY; a different mode pushes on flush (SPEC §10).
    glLogicOp(GL_XOR);
    glFlushState();
    EXPECT_EQ(backend.logicOpCalls, 1);
    EXPECT_EQ(backend.lastLogicOp, static_cast<uint32_t>(GL_XOR));

    // Same mode again must not re-push to the backend (SPEC §10).
    glLogicOp(GL_XOR);
    glFlushState();
    EXPECT_EQ(backend.logicOpCalls, 1);

    // A different mode pushes again.
    glLogicOp(GL_COPY);
    glFlushState();
    EXPECT_EQ(backend.logicOpCalls, 2);
    EXPECT_EQ(backend.lastLogicOp, static_cast<uint32_t>(GL_COPY));

    setCurrentContext(nullptr);
}

TEST_CASE("glLogicOp_unsupported_capability_is_invalid_operation") {
    // LogicOp is Native in the mock profile, so temporarily clear it.
    MockBackend backend;
    backend.setCapability(Feature::LogicOp, FeatureSupport::Unsupported);
    Context ctx(backend);
    setCurrentContext(&ctx);

    glLogicOp(GL_COPY);
    EXPECT_EQ(backend.logicOpCalls, 0);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    setCurrentContext(nullptr);
}

TEST_CASE("glBlitFramebuffer_forwards_to_backend_and_validates_mask") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    // Valid mask (color only) forwards after a state flush.
    glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    EXPECT_EQ(backend.blitFramebufferCalls, 1);
    EXPECT_EQ(backend.lastBlitMask, static_cast<uint32_t>(GL_COLOR_BUFFER_BIT));
    EXPECT_EQ(backend.lastBlitFilter, static_cast<uint32_t>(GL_NEAREST));
    EXPECT_EQ(backend.lastBlitSrcX1, 1);
    EXPECT_EQ(backend.lastBlitDstY1, 1);

    // Mask with an undefined bit is invalid (SPEC §15).
    glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1,
                      GL_COLOR_BUFFER_BIT | 0x00000008u, GL_NEAREST);
    EXPECT_EQ(backend.blitFramebufferCalls, 1);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}

TEST_CASE("glInvalidateFramebuffer_forms_and_validation") {
    MockBackend backend;
    Context ctx(backend);
    setCurrentContext(&ctx);

    GLenum full[] = {GL_COLOR_ATTACHMENT0};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, full);
    EXPECT_EQ(backend.invalidateFramebufferCalls, 1);
    EXPECT_EQ(backend.lastInvalidateSub, false);
    EXPECT_EQ(backend.lastInvalidateTarget,
              static_cast<uint32_t>(GL_FRAMEBUFFER));
    EXPECT_EQ(backend.lastInvalidateNum, 1);
    EXPECT_EQ(backend.lastInvalidateAttachments.size(), 1u);
    EXPECT_EQ(backend.lastInvalidateAttachments[0],
              static_cast<uint32_t>(GL_COLOR_ATTACHMENT0));

    GLenum sub[] = {GL_DEPTH_ATTACHMENT};
    glInvalidateSubFramebuffer(GL_FRAMEBUFFER, 1, sub, 0, 0, 4, 4);
    EXPECT_EQ(backend.invalidateFramebufferCalls, 2);
    EXPECT_EQ(backend.lastInvalidateSub, true);
    EXPECT_EQ(backend.lastInvalidateW, 4);
    EXPECT_EQ(backend.lastInvalidateH, 4);

    // Null attachments with a non-zero count is invalid (SPEC §16).
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, nullptr);
    EXPECT_EQ(backend.invalidateFramebufferCalls, 2);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    setCurrentContext(nullptr);
}
