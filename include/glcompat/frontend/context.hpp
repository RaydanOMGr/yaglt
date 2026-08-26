#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/frontend/error.hpp"
#include "glcompat/frontend/objects.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace glcompat {

// Frontend OpenGL context. Owns object identity, name allocation, binding
// state, and validation. Talks to the backend only through IGraphicsBackend,
// never to a native API. This is the foundation the OpenGL 4.6 API entry
// points will dispatch into (SPEC §2.1, §10, §11).
class Context {
public:
    explicit Context(IGraphicsBackend& backend) : backend_(backend) {}

    IGraphicsBackend& backend() { return backend_; }

    // --- Error (SPEC §19) ---
    GLError getError();          // returns and clears the pending error
    void setError(GLError e);    // records the first error since last getError

    // --- Buffers ---
    GLObjectName genBuffer();
    void bindBuffer(uint32_t target, GLObjectName name);
    GLObjectName boundBuffer(uint32_t target) const;
    void deleteBuffer(GLObjectName name);
    BufferObject* getBuffer(GLObjectName name);

    // --- Textures ---
    GLObjectName genTexture();
    void bindTexture(GLObjectName name);
    GLObjectName boundTexture() const;
    void deleteTexture(GLObjectName name);
    TextureObject* getTexture(GLObjectName name);

    // --- Renderbuffers ---
    GLObjectName genRenderbuffer();
    void bindRenderbuffer(GLObjectName name);
    GLObjectName boundRenderbuffer() const;
    void deleteRenderbuffer(GLObjectName name);
    RenderbufferObject* getRenderbuffer(GLObjectName name);

    // --- Framebuffers ---
    GLObjectName genFramebuffer();
    void bindFramebuffer(GLObjectName name);
    GLObjectName boundFramebuffer() const;
    void deleteFramebuffer(GLObjectName name);
    FramebufferObject* getFramebuffer(GLObjectName name);

    // --- Vertex arrays ---
    GLObjectName genVertexArray();
    void bindVertexArray(GLObjectName name);
    GLObjectName boundVertexArray() const;
    void deleteVertexArray(GLObjectName name);
    VertexArrayObject* getVertexArray(GLObjectName name);

private:
    GLObjectName nextName_ = 1;

    IGraphicsBackend& backend_;
    GLError error_ = GLError::NoError;

    std::unordered_map<GLObjectName, std::unique_ptr<BufferObject>> buffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<TextureObject>> textures_;
    std::unordered_map<GLObjectName, std::unique_ptr<RenderbufferObject>> renderbuffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<FramebufferObject>> framebuffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<VertexArrayObject>> vertexArrays_;

    std::unordered_map<uint32_t, GLObjectName> boundBuffers_;
    GLObjectName boundTexture_ = 0;
    GLObjectName boundRenderbuffer_ = 0;
    GLObjectName boundFramebuffer_ = 0;
    GLObjectName boundVertexArray_ = 0;
};

} // namespace glcompat
