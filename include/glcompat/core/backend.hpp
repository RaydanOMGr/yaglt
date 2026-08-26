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
