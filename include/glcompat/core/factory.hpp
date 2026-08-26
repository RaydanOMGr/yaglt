#pragma once

#include "backend_resources.hpp"
#include <memory>
#include <string>

namespace glcompat {

// Creates backend-native resources behind opaque pointers. Returns unique_ptr
// so ownership/lifetime stays with the frontend object model.
class IResourceFactory {
public:
    virtual ~IResourceFactory() = default;

    virtual std::unique_ptr<BackendBuffer> createBuffer() = 0;
    virtual std::unique_ptr<BackendTexture> createTexture() = 0;
    virtual std::unique_ptr<BackendRenderbuffer> createRenderbuffer() = 0;
    virtual std::unique_ptr<BackendFramebuffer> createFramebuffer() = 0;
    virtual std::unique_ptr<BackendVertexArray> createVertexArray() = 0;
    virtual std::unique_ptr<BackendSampler> createSampler() = 0;
    virtual std::unique_ptr<BackendTransformFeedback> createTransformFeedback() = 0;
    virtual std::unique_ptr<BackendQuery> createQuery() = 0;
    virtual std::unique_ptr<BackendShader> createShader(uint32_t stage) = 0;
    virtual std::unique_ptr<BackendProgram> createProgram() = 0;
};

// Shader translation/compilation entry point. The actual GLSL transformation
// pipeline plugs in behind this interface.
class IShaderCompiler {
public:
    virtual ~IShaderCompiler() = default;

    // Translate/compile `source` for the active backend. `stage` is the GL
    // shader stage (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...). On success
    // returns true and fills `output`. On failure returns false and fills
    // `error` with a diagnostic.
    virtual bool compile(const std::string& source,
                         uint32_t stage,
                         std::string& output,
                         std::string& error) = 0;
};

} // namespace glcompat
