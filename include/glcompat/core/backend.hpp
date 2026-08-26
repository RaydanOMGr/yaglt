#pragma once

#include "capabilities.hpp"
#include "platform.hpp"
#include <memory>
#include <string>

namespace glcompat {

class IResourceFactory;
class IShaderCompiler;

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

    virtual std::string describe() const = 0;
};

} // namespace glcompat
