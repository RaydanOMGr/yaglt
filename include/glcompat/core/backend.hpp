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

    // Clear the bound framebuffer (SPEC §2.1). The frontend pushes the tracked
    // clear color/depth values through GLStateSink before calling this, so the
    // backend issues the native clear with the current clear values.
    virtual void clear(uint32_t mask) = 0;

    // Command stream flush / finish (SPEC §2.1). glFlush empties the GL command
    // buffer; glFinish blocks until all issued commands complete.
    virtual void flush() = 0;
    virtual void finish() = 0;

    // Read back pixels from the bound framebuffer (SPEC §2.1). The frontend
    // flushes tracked state first so the backend reads the current framebuffer.
    virtual void readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                             uint32_t format, uint32_t type, void* pixels) = 0;

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
};

} // namespace glcompat
