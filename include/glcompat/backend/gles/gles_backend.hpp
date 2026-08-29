#pragma once

#include "glcompat/backend/gles/gles_capabilities.hpp"
#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/backend.hpp"
#include "glcompat/core/capabilities_table.hpp"
#include "glcompat/core/factory.hpp"
#include "glcompat/core/platform.hpp"
#include "glcompat/state/gl_state_sink.hpp"
#include "src/backend/gles/gles_factory.hpp"
#include "src/backend/gles/gles_shader_compiler.hpp"
#include "src/platform/linux/linux_capabilities.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace glcompat {

// Real OpenGL ES backend. Uses a headless, surfaceless EGL context so it works
// without a window system (Linux/Mesa, Android). Symbols are resolved at
// runtime via GLESLib, so this compiles even where libGLESv2 dev libs are
// absent. initialize() returns false honestly when no driver is available.
class GLESBackend : public IGraphicsBackend, public GLStateSink {
public:
    GLESBackend();
    ~GLESBackend() override;

    BackendApi api() const override { return BackendApi::GLES; }
    const ICapabilities& capabilities() const override { return caps_; }
    const IPlatformCapabilities& platform() const override { return platform_; }

    IResourceFactory& resourceFactory() override { return factory_; }
    IShaderCompiler& shaderCompiler() override { return *compiler_; }

    bool initialize() override;
    void shutdown() override;

    GLStateSink* stateSink() override { return this; }

    std::string describe() const override;

    // GLStateSink: push tracked state to the native driver. The frontend calls
    // these via GLStateTracker::apply() at draw / flush time (SPEC §10).
    void enable(uint32_t cap) override;
    void disable(uint32_t cap) override;
    void enableIndexed(uint32_t cap, uint32_t index) override;
    void disableIndexed(uint32_t cap, uint32_t index) override;
    void hint(uint32_t target, uint32_t mode) override;
    void beginConditionalRender(uint32_t id, uint32_t mode) override;
    void endConditionalRender() override;
    void useProgram(uint32_t prog) override;
    // Program pipeline (SPEC §7.4). GLES uses a single linked program per draw,
    // so a separable pipeline cannot be consumed for rendering; we record the
    // binding only (the frontend still models the object and its queries). The
    // ProgramPipelines capability reports Emulated only where separable programs
    // are genuinely available (ES 3.1-class drivers / EXT_separate_shader_
    // objects); where that is missing it reports Unsupported honestly.
    void bindProgramPipeline(uint32_t pipeline) override;
    void blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB, uint32_t srcAlpha,
                            uint32_t dstAlpha) override;
    void blendEquationSeparate(uint32_t modeRGB, uint32_t modeAlpha) override;
    void blendFuncSeparatei(uint32_t buf, uint32_t srcRGB, uint32_t dstRGB,
                            uint32_t srcAlpha, uint32_t dstAlpha) override;
    void blendEquationSeparatei(uint32_t buf, uint32_t modeRGB,
                                uint32_t modeAlpha) override;
    void blendColor(float r, float g, float b, float a) override;
    void depthFunc(uint32_t func) override;
    void depthMask(bool enabled) override;
    void depthRange(double nearVal, double farVal) override;
    void depthRangeIndexed(uint32_t index, double nearVal, double farVal) override;
    void stencilFunc(uint32_t func, int32_t ref, uint32_t mask) override;
    void stencilOp(uint32_t sfail, uint32_t dpfail, uint32_t dppass) override;
    void stencilMask(uint32_t mask) override;
    void stencilFuncSeparate(uint32_t face, uint32_t func, int32_t ref,
                             uint32_t mask) override;
    void stencilOpSeparate(uint32_t face, uint32_t sfail, uint32_t dpfail,
                           uint32_t dppass) override;
    void stencilMaskSeparate(uint32_t face, uint32_t mask) override;
    // Color write mask (SPEC §17.3.6, glColorMask / glColorMaski).
    void colorMask(bool r, bool g, bool b, bool a) override;
    void colorMaski(uint32_t buf, bool r, bool g, bool b, bool a) override;
    // Sample coverage (SPEC §17.3.6 multisample, glSampleCoverage).
    void sampleCoverage(float value, bool invert) override;
    // Primitive restart index (SPEC §10.4, glPrimitiveRestartIndex).
    void primitiveRestart(uint32_t index) override;
    void cullFace(uint32_t mode) override;
    void frontFace(uint32_t mode) override;
    void pointSize(float size) override;
    void lineWidth(float width) override;
    void polygonOffset(float factor, float units, float clamp) override;
    // GLES has no point parameters (glPointParameter*); recorded but not
    // forwarded to the driver (honest "Unsupported"), matching the desktop-only
    // point-sprite state.
    void pointParameters(float sizeMin, float sizeMax, float fadeThreshold,
                         GLenum spriteCoordOrigin) override;
    // Patch parameters (SPEC §10.6). glPatchParameteri is forwarded on GLES 3.2;
    // glPatchParameterfv has no GLES equivalent (default levels are set in-shader)
    // so it is recorded but not forwarded (honest "Unsupported").
    void patchParameteri(uint32_t pname, int value) override;
    void patchParameterfv(uint32_t pname, const float* values) override;
    // GLES has no glClipControl; recorded but not forwarded to the driver
    // (honest "Unsupported"), matching the desktop-only clip-volume state.
    void clipControl(GLenum origin, GLenum depth) override;
    // GLES has no polygon mode / sample mask / min sample shading; recorded but
    // not forwarded to the driver (honest "Unsupported").
    void polygonMode(uint32_t front, uint32_t back) override;
    void sampleMaski(uint32_t maskNumber, uint32_t mask) override;
    void minSampleShading(float value) override;
    void provokingVertex(uint32_t mode) override;
    void clampColor(uint32_t target, uint32_t mode) override;
    void pixelStorei(uint32_t pname, int32_t param) override;
    void setViewport(int32_t x, int32_t y, int32_t w, int32_t h) override;
    void setScissor(int32_t x, int32_t y, int32_t w, int32_t h) override;
    void setViewportIndexed(uint32_t index, int32_t x, int32_t y, int32_t w,
                            int32_t h) override;
    void setScissorIndexed(uint32_t index, int32_t x, int32_t y, int32_t w,
                           int32_t h) override;
    void clearColor(float r, float g, float b, float a) override;
    void clearDepth(double d) override;
    void clearStencil(int s) override;
    void drawBuffers(int32_t n, const uint32_t* bufs) override;
    void readBuffer(uint32_t buf) override;
    void bindBufferBase(uint32_t target, uint32_t index, uint32_t buffer) override;
    void bindBufferRange(uint32_t target, uint32_t index, uint32_t buffer,
                          intptr_t offset, intptr_t size) override;

    // Texture units (SPEC §2.1). The frontend name is resolved to the native
    // driver id via the registered name map, mirroring useProgram/bindVertexArray.
    void activeTexture(uint32_t unit) override;
    void bindTexture(uint32_t target, uint32_t texture) override;

    // Sampler objects (SPEC §8.2). `sampler` is the frontend name resolved to the
    // native driver sampler id via the registered name map; `unit` is the
    // zero-based texture unit index.
    void bindSampler(uint32_t unit, uint32_t sampler) override;

    // Vertex array + attribute setup (SPEC §2.1).
    void bindVertexArray(uint32_t vao) override;
    void enableVertexAttribArray(uint32_t index) override;
    void disableVertexAttribArray(uint32_t index) override;
    void bindBuffer(uint32_t target, uint32_t buffer) override;
    void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                             bool normalized, int32_t stride,
                             intptr_t offset) override;
    // Vertex attribute divisor (SPEC §10, glVertexAttribDivisor).
    void vertexAttribDivisor(uint32_t index, uint32_t divisor) override;

    // Color logic op (SPEC §17.3.4, glLogicOp). Pushed only when the mode changes
    // (SPEC §10); the driver applies it only while GL_COLOR_LOGIC_OP is enabled.
    void logicOp(uint32_t mode) override;

    // Frontend name -> native id mapping so useProgram/bindVertexArray can bind
    // the real driver objects (SPEC §3/§11).
    void bindNativeObject(uint32_t name, uint32_t nativeId) override;

    // Bind a framebuffer object on the driver (SPEC §9.4 / §15).
    void bindFramebuffer(uint32_t target, uint32_t framebuffer) override;

    // Test/debug access to the registered name -> native id map.
    const std::unordered_map<uint32_t, uint32_t>& nativeMap() const {
        return nativeMap_;
    }

    // Draw commands (SPEC §2.1). The frontend flushes tracked pipeline state
    // before calling these, so the driver already sees current GL state.
    void drawArrays(uint32_t mode, int32_t first, int32_t count) override;
    void drawElements(uint32_t mode, int32_t count, uint32_t type,
                      intptr_t indices) override;
    void drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                             int32_t primcount) override;
    void drawElementsInstanced(uint32_t mode, int32_t count, uint32_t type,
                                intptr_t indices, int32_t primcount) override;

    // Draw expansion (SPEC §10).
    void multiDrawArrays(uint32_t mode, const int32_t* firsts,
                        const int32_t* counts, int32_t drawcount) override;
    void multiDrawElements(uint32_t mode, const int32_t* counts, uint32_t type,
                          const intptr_t* indices, int32_t drawcount) override;
    void drawRangeElements(uint32_t mode, uint32_t start, uint32_t end,
                          int32_t count, uint32_t type, intptr_t indices) override;
    void drawElementsBaseVertex(uint32_t mode, int32_t count, uint32_t type,
                               intptr_t indices, int32_t basevertex) override;
    // Indirect draw (SPEC §10, ES 3.1+; forwarded when the driver supports it).
    void drawArraysIndirect(uint32_t mode, const void* indirect) override;
    void drawElementsIndirect(uint32_t mode, uint32_t type,
                              const void* indirect) override;

    // Compute dispatch (SPEC §7.4, ES 3.1+; forwarded when the driver supports it).
    void dispatchCompute(uint32_t x, uint32_t y, uint32_t z) override;
    void dispatchComputeIndirect(uintptr_t offset) override;

    // Clear the bound framebuffer (SPEC §2.1). The frontend pushes the tracked
    // clear color/depth through GLStateSink first, so this issues the native
    // clear with the current values.
    void clear(uint32_t mask) override;

    // Command stream flush / finish (SPEC §2.1).
    void flush() override;
    void finish() override;

    // Read back pixels from the bound framebuffer (SPEC §2.1).
    void readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                    uint32_t format, uint32_t type, void* pixels) override;

    // Whole-framebuffer copy (SPEC §15, glBlitFramebuffer).
    void blitFramebuffer(int32_t srcX0, int32_t srcY0, int32_t srcX1, int32_t srcY1,
                         int32_t dstX0, int32_t dstY0, int32_t dstX1, int32_t dstY1,
                         uint32_t mask, uint32_t filter) override;

    // Invalidate framebuffer attachments (SPEC §16). The sub-rectangle form passes
    // x/y/width/height; the full form passes width/height == 0.
    void invalidateFramebuffer(uint32_t target, int32_t numAttachments,
                              const uint32_t* attachments, int32_t x, int32_t y,
                              int32_t width, int32_t height) override;

    // Internal format queries (SPEC §22.3). Forwards to the GLES driver; GLES has
    // no glGetInternalformati64v, so the i64v variant widens from the iv call
    // (valid for the pnames GLES supports: NUM_SAMPLE_COUNTS, SAMPLES).
    void getInternalformativ(uint32_t target, uint32_t internalformat,
                            uint32_t pname, int32_t bufSize,
                            int32_t* params) override;
    void getInternalformati64v(uint32_t target, uint32_t internalformat,
                               uint32_t pname, int32_t bufSize,
                               int64_t* params) override;

    // Multisample sample-position queries (SPEC §14.3.1 glGetMultisamplefv). The
    // sample count is the SAMPLES of the bound draw framebuffer (read via
    // glGetFramebufferParameteriv; 0 if unsupported or no MSAA framebuffer). The
    // position query forwards to the GLES driver's glGetMultisamplefv.
    uint32_t getMultisampleSampleCount() override;
    void getMultisamplefv(uint32_t pname, uint32_t index, float* val) override;

    // Driver-level string query, bypassing the frontend synthetic strings.
    const char* getBackingGlString(uint32_t name) override;

 private:
    GLESLibPtr lib_;
    LinuxCapabilities platform_;
    CapabilityTable caps_;
    GLESResourceFactory factory_;
    std::unique_ptr<IShaderCompiler> compiler_;

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    bool initialized_ = false;

    // When set, the backend attaches to an externally-owned EGL context (e.g. one
    // created by a libEGL drop-in shim on behalf of the application) instead of
    // creating its own surfaceless context. The shim calls setAdopt() before
    // initialize() and is responsible for making the adopted context current.
    bool adopt_ = false;
    EGLDisplay adoptDisplay_ = EGL_NO_DISPLAY;
    EGLContext adoptContext_ = EGL_NO_CONTEXT;

    // Frontend object name -> backend-native id (only entries that the frontend
    // has registered via bindNativeObject).
    std::unordered_map<uint32_t, uint32_t> nativeMap_;

    bool createContext();
    void queryVersion();

public:
    // Attach to an externally-owned EGL display/context. The caller must make
    // `ctx` current before calling initialize(); the backend will then issue
    // native GL on that context rather than creating its own. shutdown() will
    // not destroy an adopted context/display (the owner owns their lifetime).
    void setAdopt(EGLDisplay dpy, EGLContext ctx) {
        adopt_ = true;
        adoptDisplay_ = dpy;
        adoptContext_ = ctx;
    }
    bool isAdopted() const { return adopt_; }
};

} // namespace glcompat
