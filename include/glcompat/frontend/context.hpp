#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/frontend/error.hpp"
#include "glcompat/frontend/objects.hpp"
#include "glcompat/state/gl_state.hpp"
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

    // Push tracked pipeline state to the backend (SPEC §10). Calls
    // GLStateTracker::apply() on the backend's GLStateSink; the backend then
    // issues only the native calls whose state actually changed. No-op when the
    // backend exposes no sink. The frontend calls this at draw / state-flush
    // time so redundant native calls are skipped.
    void flushState();

    // Centralized pipeline state (SPEC §10). glEnable/glDisable/glBlendFunc/
    // glUseProgram/etc. route through here so a backend can avoid redundant
    // native calls via GLStateTracker::apply().
    GLStateTracker& state() { return state_; }

    // --- Error (SPEC §19) ---
    GLError getError();          // returns and clears the pending error
    void setError(GLError e);    // records the first error since last getError

    // --- Buffers ---
    GLObjectName genBuffer();
    void genBuffers(uint32_t n, GLObjectName* names);
    void bindBuffer(uint32_t target, GLObjectName name);
    GLObjectName boundBuffer(uint32_t target) const;
    void deleteBuffer(GLObjectName name);
    void deleteBuffers(uint32_t n, const GLObjectName* names);
    void bufferData(uint32_t target, intptr_t size, uint32_t usage);
    BufferObject* getBuffer(GLObjectName name);

    // --- Textures ---
    GLObjectName genTexture();
    void genTextures(uint32_t n, GLObjectName* names);
    void bindTexture(GLObjectName name);
    GLObjectName boundTexture() const;
    void deleteTexture(GLObjectName name);
    void deleteTextures(uint32_t n, const GLObjectName* names);
    TextureObject* getTexture(GLObjectName name);

    // --- Renderbuffers ---
    GLObjectName genRenderbuffer();
    void genRenderbuffers(uint32_t n, GLObjectName* names);
    void bindRenderbuffer(GLObjectName name);
    GLObjectName boundRenderbuffer() const;
    void deleteRenderbuffer(GLObjectName name);
    void deleteRenderbuffers(uint32_t n, const GLObjectName* names);
    RenderbufferObject* getRenderbuffer(GLObjectName name);

    // --- Framebuffers ---
    GLObjectName genFramebuffer();
    void genFramebuffers(uint32_t n, GLObjectName* names);
    void bindFramebuffer(GLObjectName name);
    GLObjectName boundFramebuffer() const;
    void deleteFramebuffer(GLObjectName name);
    void deleteFramebuffers(uint32_t n, const GLObjectName* names);
    FramebufferObject* getFramebuffer(GLObjectName name);

    // --- Vertex arrays ---
    GLObjectName genVertexArray();
    void genVertexArrays(uint32_t n, GLObjectName* names);
    void bindVertexArray(GLObjectName name);
    GLObjectName boundVertexArray() const;
    void deleteVertexArray(GLObjectName name);
    void deleteVertexArrays(uint32_t n, const GLObjectName* names);
    VertexArrayObject* getVertexArray(GLObjectName name);

private:
    GLObjectName nextName_ = 1;

    IGraphicsBackend& backend_;
    GLError error_ = GLError::NoError;
    GLStateTracker state_;

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
