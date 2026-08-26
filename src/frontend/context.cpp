#include "glcompat/frontend/context.hpp"
#include "glcompat/core/capabilities.hpp"
#include "glcompat/core/factory.hpp"

#include <cstdint>

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

namespace {
// Map an indexed buffer target to the capability that gates it.
Feature bufferTargetFeature(uint32_t target) {
    switch (target) {
    case GL_UNIFORM_BUFFER: return Feature::UniformBufferObjects;
    case GL_SHADER_STORAGE_BUFFER: return Feature::ShaderStorageBufferObjects;
    case GL_TRANSFORM_FEEDBACK_BUFFER: return Feature::TransformFeedback;
    default: return Feature::FeatureCount; // unknown -> unsupported
    }
}
} // namespace

void Context::bindBufferBase(uint32_t target, uint32_t index,
                             GLObjectName buffer) {
    Feature f = bufferTargetFeature(target);
    if (f == Feature::FeatureCount ||
        !backend_.capabilities().isSupported(f)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (GLStateSink* sink = backend_.stateSink()) {
        sink->bindBufferBase(target, index, buffer);
    }
}

void Context::bindBufferRange(uint32_t target, uint32_t index,
                              GLObjectName buffer, intptr_t offset,
                              intptr_t size) {
    Feature f = bufferTargetFeature(target);
    if (f == Feature::FeatureCount ||
        !backend_.capabilities().isSupported(f)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (GLStateSink* sink = backend_.stateSink()) {
        sink->bindBufferRange(target, index, buffer, offset, size);
    }
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
    if (obj->backend) backend_.bindNativeObject(name, obj->backend->nativeId());
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

void Context::flushState() {
    GLStateSink* sink = backend_.stateSink();
    if (sink) {
        state_.apply(*sink);
        if (vertexStateDirty_) {
            if (boundVertexArray_ != 0) {
                if (VertexArrayObject* vao = getVertexArray(boundVertexArray_)) {
                    sink->bindVertexArray(boundVertexArray_);
                    for (const auto& a : vao->attribs) {
                        if (a.enabled)
                            sink->enableVertexAttribArray(a.index);
                        else
                            sink->disableVertexAttribArray(a.index);
                        sink->vertexAttribPointer(a.index, a.size, a.type,
                                                  a.normalized, a.stride,
                                                  a.offset);
                    }
                }
            }
            vertexStateDirty_ = false;
        }
    }
}

void Context::drawArrays(uint32_t mode, int32_t first, int32_t count) {
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.drawArrays(mode, first, count);
}

void Context::drawElements(uint32_t mode, int32_t count, uint32_t type,
                           intptr_t indices) {
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.drawElements(mode, count, type, indices);
}

void Context::drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                                   int32_t primcount) {
    if (!backend_.capabilities().isSupported(Feature::InstancedRendering)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.drawArraysInstanced(mode, first, count, primcount);
}

void Context::drawElementsInstanced(uint32_t mode, int32_t count, uint32_t type,
                                     intptr_t indices, int32_t primcount) {
    if (!backend_.capabilities().isSupported(Feature::InstancedRendering)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.drawElementsInstanced(mode, count, type, indices, primcount);
}

// --- Shaders / programs (SPEC §8) ---

GLObjectName Context::createShader(uint32_t stage) {
    if (!backend_.capabilities().isSupported(Feature::ShaderObjects)) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<ShaderObject>(name, stage);
    obj->backend = backend_.resourceFactory().createShader(stage);
    shaders_.emplace(name, std::move(obj));
    return name;
}

void Context::shaderSource(GLObjectName shader, const std::string& src) {
    ShaderObject* s = getShader(shader);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    s->source = src;
}

void Context::compileShader(GLObjectName shader) {
    ShaderObject* s = getShader(shader);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    std::string out, err;
    // Translate desktop GLSL -> backend source when a translator is wired in.
    // On failure we report the translation error honestly (no fake success).
    if (!backend_.shaderCompiler().compile(s->source, s->stage, out, err)) {
        s->compiled = false;
        s->infoLog = err;
        return;
    }
    std::string log;
    bool ok = s->backend ? s->backend->compile(out, log) : false;
    s->compiled = ok;
    s->infoLog = log;
    if (!ok) setError(GLError::InvalidOperation);
}

bool Context::isShaderCompiled(GLObjectName shader) const {
    const ShaderObject* s = getShader(shader);
    return s != nullptr && s->compiled;
}

std::string Context::shaderInfoLog(GLObjectName shader) const {
    const ShaderObject* s = getShader(shader);
    return s ? s->infoLog : std::string();
}

void Context::deleteShader(GLObjectName shader) {
    auto it = shaders_.find(shader);
    if (it == shaders_.end()) return;
    shaders_.erase(it);
}

ShaderObject* Context::getShader(GLObjectName name) {
    auto it = shaders_.find(name);
    return it == shaders_.end() ? nullptr : it->second.get();
}

const ShaderObject* Context::getShader(GLObjectName name) const {
    auto it = shaders_.find(name);
    return it == shaders_.end() ? nullptr : it->second.get();
}

GLObjectName Context::createProgram() {
    if (!backend_.capabilities().isSupported(Feature::ProgramObjects)) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<ProgramObject>(name);
    obj->backend = backend_.resourceFactory().createProgram();
    programs_.emplace(name, std::move(obj));
    return name;
}

void Context::attachShader(GLObjectName program, GLObjectName shader) {
    ProgramObject* p = getProgram(program);
    ShaderObject* s = getShader(shader);
    if (p == nullptr || s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!s->compiled) {
        setError(GLError::InvalidOperation);
        return;
    }
    p->attachedShaders.push_back(shader);
}

void Context::linkProgram(GLObjectName program) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    for (GLObjectName sh : p->attachedShaders) {
        if (ShaderObject* s = getShader(sh)) {
            if (s->backend) p->backend->attach(*s->backend);
        }
    }
    std::string log;
    bool ok = p->backend ? p->backend->link(log) : false;
    p->linked = ok;
    p->infoLog = log;
    if (!ok) {
        setError(GLError::InvalidOperation);
        return;
    }
    // Register the name->native mapping so the backend can bind the program at
    // draw time (SPEC §3/§11).
    backend_.bindNativeObject(program, p->backend ? p->backend->nativeId() : 0);
}

bool Context::isProgramLinked(GLObjectName program) const {
    const ProgramObject* p = getProgram(program);
    return p != nullptr && p->linked;
}

std::string Context::programInfoLog(GLObjectName program) const {
    const ProgramObject* p = getProgram(program);
    return p ? p->infoLog : std::string();
}

int Context::getAttribLocation(GLObjectName program, const std::string& name) const {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) return -1;
    return p->backend->getAttribLocation(name);
}

void Context::deleteProgram(GLObjectName program) {
    auto it = programs_.find(program);
    if (it == programs_.end()) return;
    if (state_.activeProgram() == program) {
        state_.useProgram(0);
    }
    programs_.erase(it);
}

ProgramObject* Context::getProgram(GLObjectName name) {
    auto it = programs_.find(name);
    return it == programs_.end() ? nullptr : it->second.get();
}

const ProgramObject* Context::getProgram(GLObjectName name) const {
    auto it = programs_.find(name);
    return it == programs_.end() ? nullptr : it->second.get();
}

// --- Vertex attributes (SPEC §2.1) ---

void Context::enableVertexAttribArray(uint32_t index) {
    if (boundVertexArray_ == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    getVertexArray(boundVertexArray_)->attrib(index).enabled = true;
    vertexStateDirty_ = true;
}

void Context::disableVertexAttribArray(uint32_t index) {
    if (boundVertexArray_ == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    getVertexArray(boundVertexArray_)->attrib(index).enabled = false;
    vertexStateDirty_ = true;
}

void Context::vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                  bool normalized, int32_t stride, intptr_t offset) {
    if (boundVertexArray_ == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    a.size = size;
    a.type = type;
    a.normalized = normalized;
    a.stride = stride;
    a.offset = offset;
    a.enabled = true;
    vertexStateDirty_ = true;
}

} // namespace glcompat
