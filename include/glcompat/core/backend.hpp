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

    virtual std::string describe() const = 0;
};

} // namespace glcompat
