#pragma once

#include "capabilities.hpp"
#include "platform.hpp"
#include <memory>
#include <string>

namespace glcompat {

class IResourceFactory;
class IShaderCompiler;
class GLStateSink;

// Graphics backend abstraction. The OpenGL frontend talks to this interface
// and never to a native graphics API directly.
class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;

    virtual BackendApi api() const = 0;
    virtual const ICapabilities& capabilities() const = 0;
    virtual const IPlatformCapabilities& platform() const = 0;

    virtual IResourceFactory& resourceFactory() = 0;
    virtual IShaderCompiler& shaderCompiler() = 0;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // The backend may implement GLStateSink and return itself here so the
    // frontend can push tracked state via GLStateTracker::apply() at draw /
    // flush time (SPEC §10). Returns nullptr when the backend does not consume
    // state pushes (e.g. record-only backends).
    virtual GLStateSink* stateSink() { return nullptr; }

    // Register a frontend object name -> backend-native id mapping. The frontend
    // calls this when it learns a backend-native id (e.g. after linking a
    // program or creating a VAO) so the backend can translate frontend names
    // back to native handles when flushing state (SPEC §3/§11). Default no-op
    // for backends that do not need the translation (e.g. the mock backend).
    virtual void bindNativeObject(uint32_t /*name*/, uint32_t /*nativeId*/) {}

    // Bind a framebuffer object on the driver (SPEC §9.4 / §15). `framebuffer` is
    // the native id; backends resolve the frontend name through the native map.
    // The frontend only records the bound framebuffer, so it must push the bind
    // to the backend here, otherwise draws/clears/readback target the wrong FBO.
    virtual void bindFramebuffer(uint32_t target, uint32_t framebuffer) = 0;

    // Draw commands (SPEC §2.1). The frontend flushes tracked pipeline state
    // (via GLStateSink) immediately before issuing these so the backend never
    // receives stale state. Backends translate them to native draw calls.
    virtual void drawArrays(uint32_t mode, int32_t first, int32_t count) = 0;
    virtual void drawElements(uint32_t mode, int32_t count, uint32_t type,
                              intptr_t indices) = 0;
    virtual void drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                                      int32_t primcount) = 0;
    virtual void drawElementsInstanced(uint32_t mode, int32_t count,
                                        uint32_t type, intptr_t indices,
                                        int32_t primcount) = 0;
    // Draw expansion (SPEC §10). Multi-draw issues several draw ranges/element
    // lists in one call; range-elements bounds the accessible index range;
    // base-vertex adds `basevertex` to each fetched index. The frontend flushes
    // tracked state before each (consistent with the single-draw calls).
    virtual void multiDrawArrays(uint32_t mode, const int32_t* firsts,
                                 const int32_t* counts, int32_t drawcount) = 0;
    virtual void multiDrawElements(uint32_t mode, const int32_t* counts,
                                   uint32_t type, const intptr_t* indices,
                                   int32_t drawcount) = 0;
    virtual void drawRangeElements(uint32_t mode, uint32_t start, uint32_t end,
                                   int32_t count, uint32_t type,
                                   intptr_t indices) = 0;
    virtual void drawElementsBaseVertex(uint32_t mode, int32_t count,
                                        uint32_t type, intptr_t indices,
                                        int32_t basevertex) = 0;
    // Base-instance draws (SPEC §10, ARB_base_instance / GL 4.2). These extend the
    // instanced draws with a `baseinstance` that offsets the per-instance attribute
    // index (the native glDraw*InstancedBaseInstance entry points, ES 3.2+). The
    // frontend flushes tracked state and checks the active program before calling.
    virtual void drawArraysInstancedBaseInstance(uint32_t mode, int32_t first,
                                                 int32_t count, int32_t primcount,
                                                 uint32_t baseinstance) = 0;
    virtual void drawElementsInstancedBaseInstance(uint32_t mode, int32_t count,
                                                   uint32_t type, intptr_t indices,
                                                   int32_t primcount,
                                                   uint32_t baseinstance) = 0;
    virtual void drawElementsInstancedBaseVertexBaseInstance(
        uint32_t mode, int32_t count, uint32_t type, intptr_t indices,
        int32_t primcount, int32_t basevertex, uint32_t baseinstance) = 0;
    // Indirect draw (SPEC §10). The frontend validates that an indirect buffer is
    // bound to GL_DRAW_INDIRECT_BUFFER and that a program is active before calling
    // these. `indirect` is the offset into that bound buffer (the frontend marshals
    // it as a client-side pointer for backends that read CPU-side; the GLES backend
    // forwards the offset to the driver, which reads from the bound indirect buffer).
    virtual void drawArraysIndirect(uint32_t mode, const void* indirect) = 0;
    virtual void drawElementsIndirect(uint32_t mode, uint32_t type,
                                      const void* indirect) = 0;
    // Multi-draw indirect (SPEC §10, ARB_multi_draw_indirect). Issues `drawcount`
    // indirect draw commands from a bound GL_DRAW_INDIRECT_BUFFER at `indirect`
    // (byte offset); `stride` is the per-command spacing in bytes (0 = tightly
    // packed). The frontend checks Feature::IndirectDrawing, an active program, and
    // the bound indirect buffer before calling.
    virtual void multiDrawArraysIndirect(uint32_t mode, const void* indirect,
                                         int32_t drawcount, int32_t stride) = 0;
    virtual void multiDrawElementsIndirect(uint32_t mode, uint32_t type,
                                           const void* indirect, int32_t drawcount,
                                           int32_t stride) = 0;
    // Transform-feedback draws (SPEC §13.3.3). `tfId` is the native transform-
    // feedback object handle (0 when none); `count` is the captured vertex count
    // resolved by the frontend via BackendTransformFeedback::getCapturedVertexCount.
    // The GLES backend forwards (mode, tfId, [stream, ]count) to the native driver,
    // which uses its own captured count; the mock backend records the call.
    virtual void drawTransformFeedback(uint32_t mode, uint32_t tfId,
                                      int32_t count) = 0;
    virtual void drawTransformFeedbackInstanced(uint32_t mode, uint32_t tfId,
                                                int32_t count,
                                                int32_t primcount) = 0;
    virtual void drawTransformFeedbackStream(uint32_t mode, uint32_t tfId,
                                             uint32_t stream, int32_t count) = 0;
    virtual void drawTransformFeedbackStreamInstanced(uint32_t mode, uint32_t tfId,
                                                      uint32_t stream,
                                                      int32_t count,
                                                      int32_t primcount) = 0;

    // Compute dispatch (SPEC §7.4). glDispatchCompute issues a 3D work-group
    // grid directly; glDispatchComputeIndirect reads the work-group counts from
    // the bound GL_DISPATCH_INDIRECT_BUFFER at the given byte offset.
    virtual void dispatchCompute(uint32_t x, uint32_t y, uint32_t z) = 0;
    virtual void dispatchComputeIndirect(uintptr_t offset) = 0;

    // Clear the bound framebuffer (SPEC §2.1). The frontend pushes the tracked
    // clear color/depth values through GLStateSink before calling this, so the
    // backend issues the native clear with the current clear values.
    virtual void clear(uint32_t mask) = 0;

    // Texture clearing (SPEC §8.10). DSA-style: operates on the named texture
    // directly without a bind. The frontend validates that the texture exists and
    // has storage; the backend resolves the frontend name to the native driver id
    // and issues glClearTexImage / glClearTexSubImage. `data` is normally null.
    virtual void clearTexImage(uint32_t texture, int level, uint32_t format,
                              uint32_t type, const void* data) = 0;
    virtual void clearTexSubImage(uint32_t texture, int level, int x, int y, int z,
                                 int w, int h, int d, uint32_t format,
                                 uint32_t type, const void* data) = 0;

    // Copy a sub-region of texels between two image objects (SPEC §8.21).
    // `srcName`/`dstName` are frontend object names (texture or renderbuffer);
    // the backend resolves each to the native driver id and issues
    // glCopyImageSubData. The frontend validates targets, object existence,
    // level ranges, dimension signs, and sub-region bounds.
    virtual void copyImageSubData(uint32_t srcName, uint32_t srcTarget, int srcLevel,
                                  int srcX, int srcY, int srcZ,
                                  uint32_t dstName, uint32_t dstTarget, int dstLevel,
                                  int dstX, int dstY, int dstZ,
                                  int srcWidth, int srcHeight, int srcDepth) = 0;

    // Command stream flush / finish (SPEC §2.1). glFlush empties the GL command
    // buffer; glFinish blocks until all issued commands complete.
    virtual void flush() = 0;
    virtual void finish() = 0;

    // Memory barriers (SPEC §7.13.2). glMemoryBarrier orders prior memory
    // transactions (shader writes to images, SSBOs, atomic counters, buffer
    // updates, etc.) so subsequent operations observe them; glMemoryBarrier-
    // ByRegion is the framebuffer-region-scoped variant. Defaults are no-ops,
    // since a backend without separate shader/CPU memory domains needs no
    // ordering (the mock backend records the call for observability).
    virtual void memoryBarrier(uint32_t barriers) { (void)barriers; }
    virtual void memoryBarrierByRegion(uint32_t barriers) { (void)barriers; }

    // Read back pixels from the bound framebuffer (SPEC §2.1). The frontend
    // flushes tracked state first so the backend reads the current framebuffer.
    virtual void readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                            uint32_t format, uint32_t type, void* pixels) = 0;

    // Internal format queries (SPEC §22.3, glGetInternalformativ /
    // glGetInternalformati64v). The frontend validates the call (null params ->
    // INVALID_VALUE, negative bufSize -> INVALID_VALUE, unknown pname ->
    // INVALID_ENUM) and forwards here. `bufSize` is the capacity of `params` in
    // elements; backends fill at most bufSize values. The answer is driver/
    // implementation-dependent, so each backend decides what to return.
    virtual void getInternalformativ(uint32_t target, uint32_t internalformat,
                                     uint32_t pname, int32_t bufSize,
                                     int32_t* params) = 0;
    virtual void getInternalformati64v(uint32_t target, uint32_t internalformat,
                                       uint32_t pname, int32_t bufSize,
                                        int64_t* params) = 0;

    // Multisample sample-position queries (SPEC §14.3.1, glGetMultisamplefv).
    // `getMultisampleSampleCount` returns the number of samples available for
    // position queries (the SAMPLES of the bound framebuffer); the frontend uses
    // it to validate `index` against GL_INVALID_VALUE. `getMultisamplefv` writes
    // the (x, y) sample location for the given pname/index; the frontend has
    // already validated that pname == SAMPLE_POSITION and index is in range, so
    // backends may assume those preconditions.
    virtual uint32_t getMultisampleSampleCount() = 0;
    virtual void getMultisamplefv(uint32_t pname, uint32_t index, float* val) = 0;

    // Whole-framebuffer copy (SPEC §15, glBlitFramebuffer). Copies a rectangle of
    // the bound read framebuffer into the bound draw framebuffer; `mask` selects
    // color/depth/stencil, `filter` is the scaling filter for drawable buffers.
    virtual void blitFramebuffer(int32_t srcX0, int32_t srcY0, int32_t srcX1,
                                 int32_t srcY1, int32_t dstX0, int32_t dstY0,
                                 int32_t dstX1, int32_t dstY1, uint32_t mask,
                                 uint32_t filter) = 0;

    // Invalidate framebuffer attachments (SPEC §16, glInvalidateFramebuffer /
    // glInvalidateSubFramebuffer). `target` is GL_FRAMEBUFFER / READ / DRAW;
    // `attachments` lists the attachment points to discard; the sub-rectangle form
    // passes x/y/width/height (the non-sub form passes 0 for all four).
    virtual void invalidateFramebuffer(uint32_t target, int32_t numAttachments,
                                       const uint32_t* attachments, int32_t x,
                                       int32_t y, int32_t width,
                                       int32_t height) = 0;

    virtual std::string describe() const = 0;

    // Return a driver-level string (GL_VENDOR, GL_RENDERER, GL_VERSION or
    // GL_SHADING_LANGUAGE_VERSION) straight from the backing native context,
    // bypassing any frontend synthetic value. Returns nullptr when the backend
    // has no native context or the name is unsupported by the driver.
    virtual const char* getBackingGlString(uint32_t name) = 0;
};

} // namespace glcompat
