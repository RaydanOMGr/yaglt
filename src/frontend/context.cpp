#include "glcompat/frontend/context.hpp"
#include "glcompat/core/factory.hpp"

namespace glcompat {

GLError Context::getError() {
    GLError e = error_;
    error_ = GLError::NoError;
    return e;
}

void Context::setError(GLError e) {
    if (error_ == GLError::NoError) {
        error_ = e;
    }
}

GLObjectName Context::genBuffer() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<BufferObject>(name);
    obj->backend = backend_.resourceFactory().createBuffer();
    buffers_.emplace(name, std::move(obj));
    return name;
}

void Context::bindBuffer(uint32_t target, GLObjectName name) {
    if (name != 0 && buffers_.find(name) == buffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    boundBuffers_[target] = name;
    if (name != 0) {
        buffers_[name]->target = target;
    }
}

GLObjectName Context::boundBuffer(uint32_t target) const {
    auto it = boundBuffers_.find(target);
    return it == boundBuffers_.end() ? 0 : it->second;
}

void Context::deleteBuffer(GLObjectName name) {
    auto it = buffers_.find(name);
    if (it == buffers_.end()) {
        return; // OpenGL: deleting an unused name is a no-op
    }
    // Reset binding if this buffer was bound to any target.
    for (auto& b : boundBuffers_) {
        if (b.second == name) {
            b.second = 0;
        }
    }
    buffers_.erase(it);
}

BufferObject* Context::getBuffer(GLObjectName name) {
    auto it = buffers_.find(name);
    return it == buffers_.end() ? nullptr : it->second.get();
}

void Context::genBuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) {
        names[i] = genBuffer();
    }
}

void Context::deleteBuffers(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) {
        deleteBuffer(names[i]);
    }
}

void Context::bufferData(uint32_t target, intptr_t size, uint32_t usage) {
    GLObjectName bound = boundBuffer(target);
    if (bound == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    BufferObject* obj = getBuffer(bound);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    obj->size = size;
    obj->usage = usage;
}

GLObjectName Context::genTexture() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<TextureObject>(name);
    obj->backend = backend_.resourceFactory().createTexture();
    textures_.emplace(name, std::move(obj));
    return name;
}

void Context::bindTexture(GLObjectName name) {
    if (name != 0 && textures_.find(name) == textures_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    boundTexture_ = name;
}

GLObjectName Context::boundTexture() const { return boundTexture_; }

void Context::deleteTexture(GLObjectName name) {
    auto it = textures_.find(name);
    if (it == textures_.end()) return;
    if (boundTexture_ == name) boundTexture_ = 0;
    textures_.erase(it);
}

TextureObject* Context::getTexture(GLObjectName name) {
    auto it = textures_.find(name);
    return it == textures_.end() ? nullptr : it->second.get();
}

void Context::genTextures(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genTexture();
}
void Context::deleteTextures(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteTexture(names[i]);
}

GLObjectName Context::genRenderbuffer() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<RenderbufferObject>(name);
    obj->backend = backend_.resourceFactory().createRenderbuffer();
    renderbuffers_.emplace(name, std::move(obj));
    return name;
}

void Context::bindRenderbuffer(GLObjectName name) {
    if (name != 0 && renderbuffers_.find(name) == renderbuffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    boundRenderbuffer_ = name;
}

GLObjectName Context::boundRenderbuffer() const { return boundRenderbuffer_; }

void Context::deleteRenderbuffer(GLObjectName name) {
    auto it = renderbuffers_.find(name);
    if (it == renderbuffers_.end()) return;
    if (boundRenderbuffer_ == name) boundRenderbuffer_ = 0;
    renderbuffers_.erase(it);
}

RenderbufferObject* Context::getRenderbuffer(GLObjectName name) {
    auto it = renderbuffers_.find(name);
    return it == renderbuffers_.end() ? nullptr : it->second.get();
}

void Context::genRenderbuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genRenderbuffer();
}
void Context::deleteRenderbuffers(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteRenderbuffer(names[i]);
}

GLObjectName Context::genFramebuffer() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<FramebufferObject>(name);
    obj->backend = backend_.resourceFactory().createFramebuffer();
    framebuffers_.emplace(name, std::move(obj));
    return name;
}

void Context::bindFramebuffer(GLObjectName name) {
    if (name != 0 && framebuffers_.find(name) == framebuffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    boundFramebuffer_ = name;
}

GLObjectName Context::boundFramebuffer() const { return boundFramebuffer_; }

void Context::deleteFramebuffer(GLObjectName name) {
    auto it = framebuffers_.find(name);
    if (it == framebuffers_.end()) return;
    if (boundFramebuffer_ == name) boundFramebuffer_ = 0;
    framebuffers_.erase(it);
}

FramebufferObject* Context::getFramebuffer(GLObjectName name) {
    auto it = framebuffers_.find(name);
    return it == framebuffers_.end() ? nullptr : it->second.get();
}

void Context::genFramebuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genFramebuffer();
}
void Context::deleteFramebuffers(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteFramebuffer(names[i]);
}

GLObjectName Context::genVertexArray() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<VertexArrayObject>(name);
    obj->backend = backend_.resourceFactory().createVertexArray();
    vertexArrays_.emplace(name, std::move(obj));
    return name;
}

void Context::bindVertexArray(GLObjectName name) {
    if (name != 0 && vertexArrays_.find(name) == vertexArrays_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    boundVertexArray_ = name;
}

GLObjectName Context::boundVertexArray() const { return boundVertexArray_; }

void Context::deleteVertexArray(GLObjectName name) {
    auto it = vertexArrays_.find(name);
    if (it == vertexArrays_.end()) return;
    if (boundVertexArray_ == name) boundVertexArray_ = 0;
    vertexArrays_.erase(it);
}

VertexArrayObject* Context::getVertexArray(GLObjectName name) {
    auto it = vertexArrays_.find(name);
    return it == vertexArrays_.end() ? nullptr : it->second.get();
}

void Context::genVertexArrays(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genVertexArray();
}
void Context::deleteVertexArrays(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteVertexArray(names[i]);
}

} // namespace glcompat
