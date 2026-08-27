#include "glcompat/frontend/context.hpp"
#include "glcompat/core/capabilities.hpp"
#include "glcompat/core/factory.hpp"
#include "glcompat/core/log.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>

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

const GLubyte* Context::getString(GLenum name) {
    // YAGLT identifies itself as the vendor and renderer. The VERSION string
    // follows the spec format "major.minor.release" (4.6.0) with an
    // implementation-dependent compatibility-profile suffix.
    static constexpr char kVendor[] = "YAGLT";
    static constexpr char kRenderer[] = "YAGLT";
    static constexpr char kVersion[] = "4.6.0 Compatibility Profile YAGLT";
    static constexpr char kShadingLanguageVersion[] = "4.60";
    static constexpr char kExtensions[] = "";
    switch (name) {
    case GL_VENDOR: return reinterpret_cast<const GLubyte*>(kVendor);
    case GL_RENDERER: return reinterpret_cast<const GLubyte*>(kRenderer);
    case GL_VERSION: return reinterpret_cast<const GLubyte*>(kVersion);
    case GL_SHADING_LANGUAGE_VERSION:
        return reinterpret_cast<const GLubyte*>(kShadingLanguageVersion);
    case GL_EXTENSIONS: return reinterpret_cast<const GLubyte*>(kExtensions);
    default:
        setError(GLError::InvalidEnum);
        return nullptr;
    }
}

GLObjectName Context::genBuffer() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<BufferObject>(name);
    obj->backend = backend_.resourceFactory().createBuffer();
    if (obj->backend)
        backend_.bindNativeObject(name, obj->backend->nativeId());
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

void Context::bufferData(uint32_t target, intptr_t size, uint32_t usage,
                          const void* data) {
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
    obj->store.assign(size > 0 ? static_cast<size_t>(size) : 0, 0);
    if (data && size > 0)
        std::memcpy(obj->store.data(), data, static_cast<size_t>(size));
    obj->immutable = false;
    obj->immutableFlags = 0;
    if (obj->backend) {
        obj->backend->bufferData(target, size, usage, data);
    }
}

void Context::bufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                            const void* data) {
    GLObjectName bound = boundBuffer(target);
    if (bound == 0) {
        setError(GLError::InvalidOperation); // no buffer bound
        return;
    }
    BufferObject* obj = getBuffer(bound);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (offset < 0 || size < 0 || offset + size > obj->size) {
        setError(GLError::InvalidValue); // region out of bounds
        return;
    }
    if (size > 0) {
        std::memcpy(obj->store.data() + static_cast<size_t>(offset), data,
                    static_cast<size_t>(size));
    }
    if (obj->backend) {
        obj->backend->bufferSubData(target, offset, size, data);
    }
}

void Context::bufferStorage(uint32_t target, intptr_t size, const void* data,
                            uint32_t flags) {
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
    if (!backend_.capabilities().isSupported(Feature::ImmutableBufferStorage)) {
        setError(GLError::InvalidOperation); // backend cannot do immutable storage
        return;
    }
    if (obj->immutable) {
        // Already-immutable storage cannot be reallocated (SPEC §6).
        setError(GLError::InvalidOperation);
        return;
    }
    if (size <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    obj->size = size;
    obj->usage = flags; // storage flags act as the effective usage for queries
    obj->immutable = true;
    obj->immutableFlags = flags;
    obj->store.assign(static_cast<size_t>(size), 0);
    if (data) std::memcpy(obj->store.data(), data, static_cast<size_t>(size));
    if (obj->backend) {
        obj->backend->bufferStorage(target, size, flags, data);
    }
}

void Context::copyBufferSubData(uint32_t readTarget, uint32_t writeTarget,
                                intptr_t readOffset, intptr_t writeOffset,
                                intptr_t size) {
    GLObjectName r = boundBuffer(readTarget);
    GLObjectName w = boundBuffer(writeTarget);
    if (r == 0 || w == 0) {
        setError(GLError::InvalidOperation); // read or write target not bound
        return;
    }
    BufferObject* src = getBuffer(r);
    BufferObject* dst = getBuffer(w);
    if (src == nullptr || dst == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (readOffset < 0 || size < 0 || readOffset + size > src->size ||
        writeOffset < 0 || writeOffset + size > dst->size) {
        setError(GLError::InvalidValue); // copy region out of bounds
        return;
    }
    if (size > 0) {
        std::memcpy(dst->store.data() + static_cast<size_t>(writeOffset),
                    src->store.data() + static_cast<size_t>(readOffset),
                    static_cast<size_t>(size));
    }
    if (dst->backend) {
        dst->backend->copySubData(readTarget, writeTarget, readOffset,
                                  writeOffset, size);
    }
}

void Context::getBufferParameteriv(uint32_t target, uint32_t pname,
                                   int32_t* params) {
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
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    switch (pname) {
    case GL_BUFFER_SIZE: *params = static_cast<int32_t>(obj->size); break;
    case GL_BUFFER_USAGE: *params = static_cast<int32_t>(obj->usage); break;
    case GL_BUFFER_ACCESS: *params = static_cast<int32_t>(obj->mapAccess); break;
    case GL_BUFFER_ACCESS_FLAGS:
        *params = static_cast<int32_t>(obj->immutableFlags); break;
    case GL_BUFFER_IMMUTABLE_STORAGE:
        *params = obj->immutable ? GL_TRUE : GL_FALSE; break;
    case GL_BUFFER_MAPPED:
        *params = obj->mapped ? GL_TRUE : GL_FALSE; break;
    case GL_BUFFER_MAP_LENGTH:
        *params = static_cast<int32_t>(obj->mapLength); break;
    case GL_BUFFER_MAP_OFFSET:
        *params = static_cast<int32_t>(obj->mapOffset); break;
    default:
        // Unknown pname: report GL_INVALID_ENUM (desktop GL behavior).
        setError(GLError::InvalidEnum);
        *params = 0;
        return;
    }
}

void* Context::mapBuffer(uint32_t target, uint32_t access) {
    return mapBufferRange(target, 0, 0, access);
}

void* Context::mapBufferRange(uint32_t target, intptr_t offset, intptr_t length,
                              uint32_t access) {
    GLObjectName bound = boundBuffer(target);
    if (bound == 0) {
        setError(GLError::InvalidOperation);
        return nullptr;
    }
    BufferObject* obj = getBuffer(bound);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation);
        return nullptr;
    }
    if (obj->mapped) {
        // Already mapped; GL reports GL_INVALID_OPERATION for a second map.
        setError(GLError::InvalidOperation);
        return nullptr;
    }
    if (length == 0) length = obj->size; // glMapBuffer maps the whole buffer
    if (offset < 0 || length < 0 || offset + length > obj->size) {
        setError(GLError::InvalidValue);
        return nullptr;
    }
    obj->mapped = true;
    obj->mapOffset = offset;
    obj->mapLength = length;
    obj->mapAccess = access;
    if (obj->backend) {
        // Real backends map their native copy; the frontend still serves the CPU
        // mirror so the application gets a stable pointer to its data.
        obj->backend->mapBufferRange(target, offset, length, access);
    }
    return obj->store.data() + static_cast<size_t>(offset);
}

bool Context::unmapBuffer(uint32_t target) {
    GLObjectName bound = boundBuffer(target);
    if (bound == 0) {
        setError(GLError::InvalidOperation);
        return false;
    }
    BufferObject* obj = getBuffer(bound);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation);
        return false;
    }
    if (!obj->mapped) {
        setError(GLError::InvalidOperation);
        return false;
    }
    // Flush the CPU mirror back to the backend's native copy (so writes made via
    // the mapped pointer reach the driver), then clear the mapping.
    if (obj->backend && obj->mapLength > 0) {
        obj->backend->bufferSubData(target, obj->mapOffset, obj->mapLength,
                                   obj->store.data() +
                                       static_cast<size_t>(obj->mapOffset));
        obj->backend->unmapBuffer(target);
    }
    obj->mapped = false;
    obj->mapOffset = 0;
    obj->mapLength = 0;
    obj->mapAccess = 0;
    return true;
}

namespace {
// Map a shader stage to the capability that gates it. Vertex and fragment
// shaders are gated by ShaderObjects; the advanced stages by their own
// per-stage capability. An unknown stage maps to FeatureCount so the caller
// can report GL_INVALID_ENUM honestly.
Feature shaderStageFeature(uint32_t stage) {
    switch (stage) {
    case GL_VERTEX_SHADER:
    case GL_FRAGMENT_SHADER: return Feature::ShaderObjects;
    case GL_GEOMETRY_SHADER: return Feature::GeometryShaders;
    case GL_TESS_CONTROL_SHADER:
    case GL_TESS_EVALUATION_SHADER: return Feature::TessellationShaders;
    case GL_COMPUTE_SHADER: return Feature::ComputeShaders;
    default: return Feature::FeatureCount; // unknown stage -> invalid enum
    }
}

// Highest GLSL versions YAGLT can translate/emit. Above these, the frontend
// rejects the shader before attempting translation (journal Next Step #3).
constexpr int kMaxDesktopGLSLVersion = 460; // desktop OpenGL 4.6 profile
constexpr int kMaxESGLSLVersion = 320;       // OpenGL ES 3.2

// Parse a `#version NNN [profile]` directive from shader source. Returns true
// when a directive is found; fills *version (e.g. 330) and *es (true for the
// `es` profile). Desktop profiles (core/compatibility/unspecified) yield es=false.
bool parseVersionDirective(const std::string& src, int* version, bool* es) {
    size_t pos = src.find("#version");
    if (pos == std::string::npos) return false;
    size_t i = pos + 8;
    while (i < src.size() && (src[i] == ' ' || src[i] == '\t')) ++i;
    int v = 0;
    bool got = false;
    while (i < src.size() && std::isdigit(static_cast<unsigned char>(src[i]))) {
        v = v * 10 + (src[i] - '0');
        got = true;
        ++i;
    }
    if (!got) return false;
    *version = v;
    while (i < src.size() && (src[i] == ' ' || src[i] == '\t')) ++i;
    bool isEs = false;
    if (i < src.size() && std::isalpha(static_cast<unsigned char>(src[i]))) {
        size_t tok = i;
        while (i < src.size() &&
               (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_'))
            ++i;
        if (src.substr(tok, i - tok) == "es") isEs = true;
    }
    *es = isEs;
    return true;
}

// Map an indexed buffer target to the capability that gates it.
Feature bufferTargetFeature(uint32_t target) {
    switch (target) {
    case GL_UNIFORM_BUFFER: return Feature::UniformBufferObjects;
    case GL_SHADER_STORAGE_BUFFER: return Feature::ShaderStorageBufferObjects;
    case GL_TRANSFORM_FEEDBACK_BUFFER: return Feature::TransformFeedback;
    default: return Feature::FeatureCount; // unknown -> unsupported
    }
}

// Sampler-object scalar parameters (SPEC §8.2, table 23.23). These are the
// pnames accepted by glSamplerParameteri; non-scalar pnames (TEXTURE_BORDER_COLOR,
// TEXTURE_SWIZZLE_RGBA) are rejected, matching the spec.
bool isValidSamplerParameter(uint32_t pname) {
    switch (pname) {
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_COMPARE_FUNC:
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_LOD_BIAS:
        return true;
    default:
        return false;
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
    if (obj->backend) backend_.bindNativeObject(name, obj->backend->nativeId());
    textures_.emplace(name, std::move(obj));
    return name;
}

void Context::activeTexture(GLenum texture) {
    if (!state_.setActiveTexture(texture)) {
        // setActiveTexture returns false for an out-of-range unit; report it as
        // an invalid enum (desktop GL: only GL_TEXTURE0+i in range is allowed).
        if (texture < GL_TEXTURE0 ||
            texture >= GL_TEXTURE0 + state_.maxCombinedTextureUnits()) {
            setError(GLError::InvalidEnum);
        }
    }
}

void Context::bindTexture(GLenum target, GLObjectName name) {
    if (name != 0 && textures_.find(name) == textures_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (TextureObject* tex = getTexture(name)) {
        tex->target = target;
    }
    state_.setTextureBinding(target, name);
}

GLObjectName Context::boundTextureForTarget(GLenum target) const {
    return state_.boundTextureForTarget(target);
}

namespace {
// Valid GL texture targets accepted by the DSA bind entry points (SPEC §8.1).
bool isValidTextureTarget(GLenum target) {
    switch (target) {
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_RECTANGLE:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_CUBE_MAP_ARRAY:
    case GL_TEXTURE_2D_MULTISAMPLE:
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        return true;
    default:
        return false;
    }
}
} // namespace

void Context::bindTextureUnit(uint32_t unit, GLObjectName texture) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (unit >= state_.maxCombinedTextureUnits()) {
        setError(GLError::InvalidValue);
        return;
    }
    if (texture != 0 && textures_.find(texture) == textures_.end()) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    GLenum target = GL_TEXTURE_2D;
    if (texture != 0) {
        if (TextureObject* tex = getTexture(texture)) target = tex->target;
    }
    state_.setTextureUnitBinding(unit, target, texture);
}

void Context::bindTextures(uint32_t first, uint32_t count, GLenum target,
                           const GLObjectName* textures) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidTextureTarget(target)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (first > state_.maxCombinedTextureUnits() ||
        first + count > state_.maxCombinedTextureUnits()) {
        setError(GLError::InvalidValue);
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        GLObjectName name = (textures != nullptr) ? textures[i] : 0;
        if (name != 0 && textures_.find(name) == textures_.end()) {
            setError(GLError::InvalidOperation); // ungenerated name
            return;
        }
    }
    state_.setTextureBindings(first, count, target, textures);
}

GLObjectName Context::boundTextureForUnitTarget(uint32_t unit,
                                                GLenum target) const {
    return state_.boundTextureForUnitTarget(unit, target);
}

void Context::deleteTexture(GLObjectName name) {
    auto it = textures_.find(name);
    if (it == textures_.end()) return;
    state_.clearTextureBinding(name);
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

void Context::texImage2D(uint32_t target, int level, uint32_t internalFormat,
                          int width, int height, uint32_t format, uint32_t type,
                          const void* data) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (width < 0 || height < 0 || level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    TextureObject::Image img;
    img.level = level;
    img.internalFormat = internalFormat;
    img.width = width;
    img.height = height;
    img.format = format;
    img.type = type;
    img.hasData = (data != nullptr);
    bool replaced = false;
    for (auto& e : tex->images) {
        if (e.level == level) { e = img; replaced = true; break; }
    }
    if (!replaced) tex->images.push_back(img);
    tex->storageSet = true;
    if (tex->backend) {
        tex->backend->texImage2D(target, level, internalFormat, width, height,
                                 format, type, data);
    }
}

void Context::texImage1D(uint32_t target, int level, uint32_t internalFormat,
                          int width, uint32_t format, uint32_t type,
                          const void* data) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (width < 0 || level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    TextureObject::Image img;
    img.level = level;
    img.internalFormat = internalFormat;
    img.width = width;
    img.height = 1;
    img.depth = 0;
    img.format = format;
    img.type = type;
    img.hasData = (data != nullptr);
    bool replaced = false;
    for (auto& e : tex->images) {
        if (e.level == level) { e = img; replaced = true; break; }
    }
    if (!replaced) tex->images.push_back(img);
    tex->storageSet = true;
    if (tex->backend) {
        tex->backend->texImage1D(target, level, internalFormat, width, format, type,
                                 data);
    }
}

void Context::texImage3D(uint32_t target, int level, uint32_t internalFormat,
                          int width, int height, int depth, uint32_t format,
                          uint32_t type, const void* data) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (width < 0 || height < 0 || depth < 0 || level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    TextureObject::Image img;
    img.level = level;
    img.internalFormat = internalFormat;
    img.width = width;
    img.height = height;
    img.depth = depth;
    img.format = format;
    img.type = type;
    img.hasData = (data != nullptr);
    bool replaced = false;
    for (auto& e : tex->images) {
        if (e.level == level) { e = img; replaced = true; break; }
    }
    if (!replaced) tex->images.push_back(img);
    tex->storageSet = true;
    if (tex->backend) {
        tex->backend->texImage3D(target, level, internalFormat, width, height, depth,
                                 format, type, data);
    }
}

void Context::texParameteri(uint32_t target, uint32_t pname, int param) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    tex->target = target;
    tex->params[pname] = param;
    if (tex->backend) tex->backend->texParameteri(target, pname, param);
}

void Context::texParameterf(uint32_t target, uint32_t pname, float param) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    tex->target = target;
    tex->paramsf[pname] = param;
    if (tex->backend) tex->backend->texParameterf(target, pname, param);
}

void Context::texParameterfv(uint32_t target, uint32_t pname,
                             const float* params, int count) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr || count <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->paramsfv[pname].assign(params, params + count);
    if (tex->backend) tex->backend->texParameterfv(target, pname, params, count);
}

void Context::texParameteriv(uint32_t target, uint32_t pname, const int* params,
                             int count) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr || count <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->paramsiv[pname].assign(params, params + count);
    if (tex->backend) tex->backend->texParameteriv(target, pname, params, count);
}

void Context::getTexParameteriv(GLenum target, GLenum pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    auto it = tex->params.find(pname);
    *params = (it != tex->params.end()) ? it->second : 0;
}

void Context::getTexParameterfv(GLenum target, GLenum pname, float* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    auto fi = tex->paramsf.find(pname);
    if (fi != tex->paramsf.end()) {
        *params = fi->second;
        return;
    }
    auto fv = tex->paramsfv.find(pname);
    if (fv != tex->paramsfv.end() && !fv->second.empty()) {
        *params = fv->second[0];
        return;
    }
    *params = 0.0f; // GL default for an unset parameter
}

namespace {
// Returns the previously allocated image for `level`, or nullptr when the level
// was never defined by a TexImage call (glTexSubImage requires existing storage).
const TextureObject::Image* findLevel(const TextureObject* tex, int level) {
    for (const auto& img : tex->images) {
        if (img.level == level) return &img;
    }
    return nullptr;
}
} // namespace

void Context::texSubImage1D(uint32_t target, int level, int xoffset, int width,
                            uint32_t format, uint32_t type, const void* data) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || width < 0 || xoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    const TextureObject::Image* lvl = findLevel(tex, level);
    if (lvl == nullptr) {
        setError(GLError::InvalidOperation); // level not allocated
        return;
    }
    if (xoffset + width > lvl->width) {
        setError(GLError::InvalidValue); // region exceeds allocated level
        return;
    }
    tex->target = target;
    TextureObject::SubImage sub;
    sub.target = target; sub.level = level; sub.xoffset = xoffset;
    sub.width = width; sub.format = format; sub.type = type; sub.dim = 1;
    tex->subimages.push_back(sub);
    if (tex->backend) tex->backend->texSubImage1D(target, level, xoffset, width,
                                                  format, type, data);
}

void Context::texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                            int width, int height, uint32_t format, uint32_t type,
                            const void* data) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || width < 0 || height < 0 || xoffset < 0 || yoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    const TextureObject::Image* lvl = findLevel(tex, level);
    if (lvl == nullptr) {
        setError(GLError::InvalidOperation); // level not allocated
        return;
    }
    if (xoffset + width > lvl->width || yoffset + height > lvl->height) {
        setError(GLError::InvalidValue); // region exceeds allocated level
        return;
    }
    tex->target = target;
    TextureObject::SubImage sub;
    sub.target = target; sub.level = level; sub.xoffset = xoffset;
    sub.yoffset = yoffset; sub.width = width; sub.height = height;
    sub.format = format; sub.type = type; sub.dim = 2;
    tex->subimages.push_back(sub);
    if (tex->backend) tex->backend->texSubImage2D(target, level, xoffset, yoffset,
                                                 width, height, format, type,
                                                 data);
}

void Context::texSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                            int zoffset, int width, int height, int depth,
                            uint32_t format, uint32_t type, const void* data) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || width < 0 || height < 0 || depth < 0 || xoffset < 0 ||
        yoffset < 0 || zoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    const TextureObject::Image* lvl = findLevel(tex, level);
    if (lvl == nullptr) {
        setError(GLError::InvalidOperation); // level not allocated
        return;
    }
    if (xoffset + width > lvl->width || yoffset + height > lvl->height) {
        setError(GLError::InvalidValue); // region exceeds allocated level
        return;
    }
    tex->target = target;
    TextureObject::SubImage sub;
    sub.target = target; sub.level = level; sub.xoffset = xoffset;
    sub.yoffset = yoffset; sub.zoffset = zoffset; sub.width = width;
    sub.height = height; sub.depth = depth; sub.format = format;
    sub.type = type; sub.dim = 3;
    tex->subimages.push_back(sub);
    if (tex->backend) tex->backend->texSubImage3D(target, level, xoffset, yoffset,
                                                 zoffset, width, height, depth,
                                                 format, type, data);
}

void Context::copyTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                             int x, int y, int width, int border) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || width < 0 || border != 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    if (tex->backend) tex->backend->copyTexImage1D(target, level, internalFormat,
                                                  x, y, width, border);
}

void Context::copyTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                             int x, int y, int width, int height, int border) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || width < 0 || height < 0 || border != 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    if (tex->backend) tex->backend->copyTexImage2D(target, level, internalFormat,
                                                  x, y, width, height, border);
}

void Context::getTextureParameteriv(GLObjectName texture, GLenum pname,
                                     int32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (texture != 0 && textures_.find(texture) == textures_.end()) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    TextureObject* tex = getTexture(texture);
    if (tex == nullptr) {
        *params = 0; // default (name 0) texture object
        return;
    }
    auto it = tex->params.find(pname);
    *params = (it != tex->params.end()) ? it->second : 0;
}

namespace {
// Resolve a DSA texture op: returns the texture object or nullptr when the op
// must be refused (error already set). `name` is the explicit object name.
TextureObject* dsaTexture(Context& ctx, GLObjectName name) {
    if (!ctx.backend().capabilities().isSupported(Feature::DirectStateAccess)) {
        ctx.setError(GLError::InvalidOperation);
        return nullptr;
    }
    if (name == 0 || ctx.getTexture(name) == nullptr) {
        ctx.setError(GLError::InvalidOperation); // ungenerated / default name
        return nullptr;
    }
    return ctx.getTexture(name);
}
} // namespace

void Context::createTextures(uint32_t target, uint32_t n, GLObjectName* names) {
    if (!isValidTextureTarget(target)) {
        setError(GLError::InvalidEnum);
        return;
    }
    for (uint32_t i = 0; i < n; ++i) {
        GLObjectName name = genTexture();
        if (TextureObject* tex = getTexture(name)) tex->target = target;
        names[i] = name;
    }
}

void Context::textureStorage1D(GLObjectName texture, int levels,
                               uint32_t internalFormat, int width) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (levels < 1 || width < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = GL_TEXTURE_1D;
    tex->storageLevels = levels;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = 1;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend) tex->backend->storage1D(GL_TEXTURE_1D, levels, internalFormat,
                                              width);
}

void Context::textureStorage2D(GLObjectName texture, int levels,
                               uint32_t internalFormat, int width, int height) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (levels < 1 || width < 1 || height < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = GL_TEXTURE_2D;
    tex->storageLevels = levels;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend) tex->backend->storage2D(GL_TEXTURE_2D, levels, internalFormat,
                                              width, height);
}

void Context::textureStorage3D(GLObjectName texture, int levels,
                               uint32_t internalFormat, int width, int height,
                               int depth) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (levels < 1 || width < 1 || height < 1 || depth < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = GL_TEXTURE_3D;
    tex->storageLevels = levels;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = depth;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage3D(GL_TEXTURE_3D, levels, internalFormat, width,
                                height, depth);
}

void Context::textureSubImage1D(GLObjectName texture, int level, int xoffset,
                                int width, uint32_t format, uint32_t type,
                                const void* data) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || width < 0 || xoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!tex->immutableStorage && findLevel(tex, level) == nullptr) {
        setError(GLError::InvalidOperation); // no storage allocated
        return;
    }
    if (tex->immutableStorage &&
        (xoffset + width >
         (tex->storageBaseWidth >> level ? tex->storageBaseWidth >> level : 1))) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = GL_TEXTURE_1D;
    if (tex->backend)
        tex->backend->texSubImage1D(GL_TEXTURE_1D, level, xoffset, width, format,
                                    type, data);
}

void Context::textureSubImage2D(GLObjectName texture, int level, int xoffset,
                                int yoffset, int width, int height,
                                uint32_t format, uint32_t type, const void* data) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || width < 0 || height < 0 || xoffset < 0 || yoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!tex->immutableStorage && findLevel(tex, level) == nullptr) {
        setError(GLError::InvalidOperation); // no storage allocated
        return;
    }
    if (tex->immutableStorage) {
        int w = tex->storageBaseWidth >> level ? tex->storageBaseWidth >> level : 1;
        int h = tex->storageBaseHeight >> level ? tex->storageBaseHeight >> level : 1;
        if (xoffset + width > w || yoffset + height > h) {
            setError(GLError::InvalidValue);
            return;
        }
    }
    tex->target = GL_TEXTURE_2D;
    if (tex->backend)
        tex->backend->texSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width,
                                    height, format, type, data);
}

void Context::textureSubImage3D(GLObjectName texture, int level, int xoffset,
                                int yoffset, int zoffset, int width, int height,
                                int depth, uint32_t format, uint32_t type,
                                const void* data) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || width < 0 || height < 0 || depth < 0 || xoffset < 0 ||
        yoffset < 0 || zoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!tex->immutableStorage && findLevel(tex, level) == nullptr) {
        setError(GLError::InvalidOperation); // no storage allocated
        return;
    }
    tex->target = GL_TEXTURE_3D;
    if (tex->backend)
        tex->backend->texSubImage3D(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset,
                                    width, height, depth, format, type, data);
}

void Context::textureParameteri(GLObjectName texture, uint32_t pname, int param) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    tex->params[pname] = param;
    if (tex->backend) tex->backend->texParameteri(tex->target, pname, param);
}

void Context::textureParameterf(GLObjectName texture, uint32_t pname, float param) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    tex->paramsf[pname] = param;
    if (tex->backend) tex->backend->texParameterf(tex->target, pname, param);
}

void Context::textureParameterfv(GLObjectName texture, uint32_t pname,
                                 const float* params, int count) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (params == nullptr || count <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->paramsfv[pname].assign(params, params + count);
    if (tex->backend) tex->backend->texParameterfv(tex->target, pname, params, count);
}

void Context::textureParameteriv(GLObjectName texture, uint32_t pname,
                                 const int* params, int count) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (params == nullptr || count <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->paramsiv[pname].assign(params, params + count);
    if (tex->backend) tex->backend->texParameteriv(tex->target, pname, params, count);
}

void Context::generateTextureMipmap(GLObjectName texture) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (tex->backend) tex->backend->generateMipmap(tex->target);
}

void Context::getTextureParameterfv(GLObjectName texture, GLenum pname,
                                    float* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (texture != 0 && textures_.find(texture) == textures_.end()) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    TextureObject* tex = getTexture(texture);
    if (tex == nullptr) {
        *params = 0.0f; // default (name 0) texture object
        return;
    }
    auto fi = tex->paramsf.find(pname);
    if (fi != tex->paramsf.end()) {
        *params = fi->second;
        return;
    }
    auto fv = tex->paramsfv.find(pname);
    if (fv != tex->paramsfv.end() && !fv->second.empty()) {
        *params = fv->second[0];
        return;
    }
    *params = 0.0f; // GL default for an unset parameter
}

void Context::getTextureLevelParameteriv(GLObjectName texture, int level,
                                         GLenum pname, int32_t* params) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (level < 0 || level >= tex->storageLevels) {
        setError(GLError::InvalidValue);
        return;
    }
    int w = tex->storageBaseWidth >> level ? tex->storageBaseWidth >> level : 1;
    int h = tex->storageBaseHeight >> level ? tex->storageBaseHeight >> level : 1;
    int d = tex->storageBaseDepth >> level ? tex->storageBaseDepth >> level : 1;
    switch (pname) {
    case GL_TEXTURE_WIDTH: *params = w; break;
    case GL_TEXTURE_HEIGHT: *params = h; break;
    case GL_TEXTURE_DEPTH: *params = d; break;
    case GL_TEXTURE_INTERNAL_FORMAT:
        *params = static_cast<int32_t>(tex->storageInternalFormat);
        break;
    default:
        // Unknown pname: ask the backend (driver introspection) when present.
        if (tex->backend) tex->backend->getLevelParameteriv(tex->target, level,
                                                           pname, params);
        else *params = 0;
        return;
    }
    // Frontend owns these values; do not round-trip to the backend.
}

void Context::getTextureLevelParameterfv(GLObjectName texture, int level,
                                         GLenum pname, float* params) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (level < 0 || level >= tex->storageLevels) {
        setError(GLError::InvalidValue);
        return;
    }
    int w = tex->storageBaseWidth >> level ? tex->storageBaseWidth >> level : 1;
    int h = tex->storageBaseHeight >> level ? tex->storageBaseHeight >> level : 1;
    int d = tex->storageBaseDepth >> level ? tex->storageBaseDepth >> level : 1;
    switch (pname) {
    case GL_TEXTURE_WIDTH: *params = static_cast<float>(w); break;
    case GL_TEXTURE_HEIGHT: *params = static_cast<float>(h); break;
    case GL_TEXTURE_DEPTH: *params = static_cast<float>(d); break;
    case GL_TEXTURE_INTERNAL_FORMAT:
        *params = static_cast<float>(tex->storageInternalFormat);
        break;
    default:
        if (tex->backend) tex->backend->getLevelParameterfv(tex->target, level,
                                                           pname, params);
        else *params = 0.0f;
        return;
    }
}

void Context::getTextureImage(GLObjectName texture, int level, uint32_t format,
                              uint32_t type, void* pixels) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getTexImage(tex->target, level, format, type,
                                                pixels);
}

void Context::getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                          void* pixels) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getTexImage(target, level, format, type, pixels);
}

void Context::textureBuffer(GLObjectName texture, uint32_t internalFormat,
                            GLObjectName buffer) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation); // ungenerated buffer name
        return;
    }
    uint32_t nativeBuffer = 0;
    if (buffer != 0) {
        if (auto* b = getBuffer(buffer))
            nativeBuffer = b->backend ? b->backend->nativeId() : 0;
    }
    tex->target = GL_TEXTURE_BUFFER;
    if (tex->backend)
        tex->backend->textureBuffer(GL_TEXTURE_BUFFER, internalFormat, nativeBuffer);
}

void Context::textureBufferRange(GLObjectName texture, uint32_t internalFormat,
                                 GLObjectName buffer, intptr_t offset,
                                 intptr_t size) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation); // ungenerated buffer name
        return;
    }
    if (offset < 0 || size < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t nativeBuffer = 0;
    if (buffer != 0) {
        if (auto* b = getBuffer(buffer))
            nativeBuffer = b->backend ? b->backend->nativeId() : 0;
    }
    tex->target = GL_TEXTURE_BUFFER;
    if (tex->backend)
        tex->backend->textureBufferRange(GL_TEXTURE_BUFFER, internalFormat,
                                         nativeBuffer, offset, size);
}

GLObjectName Context::genRenderbuffer() {
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<RenderbufferObject>(name);
    obj->backend = backend_.resourceFactory().createRenderbuffer();
    if (obj->backend) backend_.bindNativeObject(name, obj->backend->nativeId());
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

void Context::renderbufferStorage(uint32_t target, uint32_t internalFormat,
                                 int width, int height) {
    if (!backend_.capabilities().isSupported(Feature::RenderbufferObjects)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (target != GL_RENDERBUFFER) {
        setError(GLError::InvalidOperation);
        return;
    }
    RenderbufferObject* rbo = getRenderbuffer(boundRenderbuffer_);
    if (rbo == nullptr) {
        setError(GLError::InvalidOperation); // no renderbuffer bound
        return;
    }
    if (width < 0 || height < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    rbo->internalFormat = internalFormat;
    rbo->width = width;
    rbo->height = height;
    rbo->storageSet = true;
    if (rbo->backend) {
        rbo->backend->renderbufferStorage(target, internalFormat, width, height);
    }
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
    if (obj->backend) backend_.bindNativeObject(name, obj->backend->nativeId());
    framebuffers_.emplace(name, std::move(obj));
    return name;
}

void Context::bindFramebuffer(GLObjectName name) {
    if (name != 0 && framebuffers_.find(name) == framebuffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    boundFramebuffer_ = name;
    // Push the bind to the backend so draws/clears/readback target the right FBO
    // on the real driver (SPEC §9.4 / §15). The frontend otherwise only records
    // the name and the driver keeps the default framebuffer bound.
    if (name != 0) {
        if (auto* f = getFramebuffer(name))
            backend_.bindFramebuffer(GL_FRAMEBUFFER,
                                     f->backend ? f->backend->nativeId() : name);
    } else {
        backend_.bindFramebuffer(GL_FRAMEBUFFER, 0);
    }
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

void Context::framebufferTexture2D(uint32_t target, uint32_t attachment,
                                   uint32_t texTarget, GLObjectName texture,
                                   int level) {
    FramebufferObject* fbo = getFramebuffer(boundFramebuffer_);
    if (fbo == nullptr) {
        setError(GLError::InvalidOperation); // no framebuffer bound
        return;
    }
    if (texture != 0 && textures_.find(texture) == textures_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    // Record / replace the attachment for this attachment point.
    FramebufferObject::Attachment att;
    att.attachment = attachment;
    att.type = 0; // texture
    att.name = texture;
    att.texTarget = texTarget;
    att.level = level;
    for (auto& a : fbo->attachments) {
        if (a.attachment == attachment) { a = att; goto applied; }
    }
    fbo->attachments.push_back(att);
applied:
    if (fbo->backend) {
        uint32_t nativeTex = 0;
        if (texture != 0) {
            if (auto* t = getTexture(texture)) nativeTex = t->backend ? t->backend->nativeId() : 0;
        }
        fbo->backend->framebufferTexture2D(target, attachment, texTarget,
                                            nativeTex, level);
    }
}

void Context::framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                      uint32_t rbTarget, GLObjectName renderbuffer) {
    FramebufferObject* fbo = getFramebuffer(boundFramebuffer_);
    if (fbo == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (renderbuffer != 0 &&
        renderbuffers_.find(renderbuffer) == renderbuffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    FramebufferObject::Attachment att;
    att.attachment = attachment;
    att.type = 1; // renderbuffer
    att.name = renderbuffer;
    att.texTarget = rbTarget;
    att.level = 0;
    for (auto& a : fbo->attachments) {
        if (a.attachment == attachment) { a = att; goto applied; }
    }
    fbo->attachments.push_back(att);
applied:
    if (fbo->backend) {
        uint32_t nativeRb = 0;
        if (renderbuffer != 0) {
            if (auto* r = getRenderbuffer(renderbuffer))
                nativeRb = r->backend ? r->backend->nativeId() : 0;
        }
        fbo->backend->framebufferRenderbuffer(target, attachment, rbTarget,
                                              nativeRb);
    }
}

uint32_t Context::checkFramebufferStatus(uint32_t target) {
    FramebufferObject* fbo = getFramebuffer(boundFramebuffer_);
    if (fbo == nullptr) return GL_FRAMEBUFFER_COMPLETE; // default FBO
    if (fbo->attachments.empty())
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    // Per-attachment completeness (SPEC §9.4): every attachment must reference
    // an existing object that has had storage allocated. A missing or
    // not-yet-specified attachment is reported honestly as INCOMPLETE_ATTACHMENT
    // rather than pretending the FBO is complete.
    for (const auto& a : fbo->attachments) {
        if (a.name == 0) return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        if (a.type == 0) { // texture attachment
            TextureObject* tex = getTexture(a.name);
            if (tex == nullptr || !tex->storageSet)
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        } else { // renderbuffer attachment
            RenderbufferObject* rbo = getRenderbuffer(a.name);
            if (rbo == nullptr || !rbo->storageSet)
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }
    }
    if (fbo->backend) return fbo->backend->checkStatus(target);
    return GL_FRAMEBUFFER_COMPLETE;
}

void Context::genFramebuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genFramebuffer();
}
void Context::deleteFramebuffers(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteFramebuffer(names[i]);
}

namespace {
// Resolve a DSA renderbuffer op (SPEC §9.2). Returns the object or nullptr with
// an error already set when the op must be refused (DSA unsupported / ungenerated
// name). Mirrors dsaTexture() for renderbuffers.
RenderbufferObject* dsaRenderbuffer(Context& ctx, GLObjectName name) {
    if (!ctx.backend().capabilities().isSupported(Feature::DirectStateAccess)) {
        ctx.setError(GLError::InvalidOperation);
        return nullptr;
    }
    if (name == 0 || ctx.getRenderbuffer(name) == nullptr) {
        ctx.setError(GLError::InvalidOperation); // ungenerated / default name
        return nullptr;
    }
    return ctx.getRenderbuffer(name);
}
// Resolve a DSA framebuffer op (SPEC §9.2). Same contract as dsaRenderbuffer.
FramebufferObject* dsaFramebuffer(Context& ctx, GLObjectName name) {
    if (!ctx.backend().capabilities().isSupported(Feature::DirectStateAccess)) {
        ctx.setError(GLError::InvalidOperation);
        return nullptr;
    }
    if (name == 0 || ctx.getFramebuffer(name) == nullptr) {
        ctx.setError(GLError::InvalidOperation); // ungenerated / default name
        return nullptr;
    }
    return ctx.getFramebuffer(name);
}
// Backend-native id for a named framebuffer, or 0 for the default framebuffer.
uint32_t namedFramebufferNativeId(Context& ctx, GLObjectName fb) {
    if (fb == 0) return 0;
    if (FramebufferObject* f = ctx.getFramebuffer(fb))
        return f->backend ? f->backend->nativeId() : fb;
    return fb;
}
// Record / replace the attachment for `attachment` on `fbo` (SPEC §2.1). A new
// attachment is appended; an existing one at the same point is replaced in place.
void attachToFramebuffer(FramebufferObject& fbo,
                        const FramebufferObject::Attachment& att) {
    for (auto& a : fbo.attachments) {
        if (a.attachment == att.attachment) { a = att; return; }
    }
    fbo.attachments.push_back(att);
}
} // namespace

void Context::createRenderbuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genRenderbuffer();
}

void Context::namedRenderbufferStorage(GLObjectName renderbuffer,
                                       uint32_t internalFormat, int width,
                                       int height) {
    RenderbufferObject* rbo = dsaRenderbuffer(*this, renderbuffer);
    if (rbo == nullptr) return;
    if (width < 0 || height < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    rbo->internalFormat = internalFormat;
    rbo->width = width;
    rbo->height = height;
    rbo->storageSet = true;
    if (rbo->backend)
        rbo->backend->renderbufferStorage(GL_RENDERBUFFER, internalFormat, width,
                                          height);
}

void Context::namedRenderbufferStorageMultisample(GLObjectName renderbuffer,
                                                 int samples,
                                                 uint32_t internalFormat, int width,
                                                 int height) {
    RenderbufferObject* rbo = dsaRenderbuffer(*this, renderbuffer);
    if (rbo == nullptr) return;
    if (samples < 0 || width < 0 || height < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    rbo->internalFormat = internalFormat;
    rbo->width = width;
    rbo->height = height;
    rbo->samples = samples;
    rbo->storageSet = true;
    if (rbo->backend)
        rbo->backend->renderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                                    internalFormat, width, height);
}

void Context::getNamedRenderbufferParameteriv(GLObjectName renderbuffer,
                                             uint32_t pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    RenderbufferObject* rbo = dsaRenderbuffer(*this, renderbuffer);
    if (rbo == nullptr) return;
    switch (pname) {
        case GL_RENDERBUFFER_WIDTH:        *params = rbo->width; break;
        case GL_RENDERBUFFER_HEIGHT:       *params = rbo->height; break;
        case GL_RENDERBUFFER_INTERNAL_FORMAT:
            *params = static_cast<int32_t>(rbo->internalFormat); break;
        case GL_RENDERBUFFER_SAMPLES:      *params = rbo->samples; break;
        default: *params = 0; break;
    }
}

void Context::createFramebuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genFramebuffer();
}

void Context::namedFramebufferRenderbuffer(GLObjectName framebuffer,
                                          uint32_t attachment,
                                          uint32_t renderbufferTarget,
                                          GLObjectName renderbuffer) {
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return;
    if (renderbuffer != 0 &&
        renderbuffers_.find(renderbuffer) == renderbuffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    FramebufferObject::Attachment att;
    att.attachment = attachment;
    att.type = 1; // renderbuffer
    att.name = renderbuffer;
    att.texTarget = renderbufferTarget;
    att.level = 0;
    attachToFramebuffer(*fbo, att);
    if (fbo->backend) {
        uint32_t nativeRb = 0;
        if (renderbuffer != 0) {
            if (auto* r = getRenderbuffer(renderbuffer))
                nativeRb = r->backend ? r->backend->nativeId() : 0;
        }
        fbo->backend->framebufferRenderbuffer(GL_FRAMEBUFFER, attachment,
                                              renderbufferTarget, nativeRb);
    }
}

void Context::namedFramebufferTexture(GLObjectName framebuffer, uint32_t attachment,
                                     GLObjectName texture, int level) {
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return;
    if (texture != 0 && textures_.find(texture) == textures_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    FramebufferObject::Attachment att;
    att.attachment = attachment;
    att.type = 0; // texture
    att.name = texture;
    att.texTarget = texture ? (getTexture(texture) ? getTexture(texture)->target
                                                   : GL_TEXTURE_2D)
                            : GL_TEXTURE_2D;
    att.level = level;
    att.layer = 0;
    attachToFramebuffer(*fbo, att);
    if (fbo->backend) {
        uint32_t nativeTex = 0;
        if (texture != 0) {
            if (auto* t = getTexture(texture))
                nativeTex = t->backend ? t->backend->nativeId() : 0;
        }
        fbo->backend->framebufferTexture2D(GL_FRAMEBUFFER, attachment,
                                          att.texTarget, nativeTex, level);
    }
}

void Context::namedFramebufferTextureLayer(GLObjectName framebuffer,
                                          uint32_t attachment, GLObjectName texture,
                                          int level, int layer) {
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return;
    if (texture != 0 && textures_.find(texture) == textures_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    FramebufferObject::Attachment att;
    att.attachment = attachment;
    att.type = 0; // texture
    att.name = texture;
    att.texTarget = texture ? (getTexture(texture) ? getTexture(texture)->target
                                                   : GL_TEXTURE_2D)
                            : GL_TEXTURE_2D;
    att.level = level;
    att.layer = layer;
    attachToFramebuffer(*fbo, att);
    if (fbo->backend) {
        uint32_t nativeTex = 0;
        if (texture != 0) {
            if (auto* t = getTexture(texture))
                nativeTex = t->backend ? t->backend->nativeId() : 0;
        }
        fbo->backend->framebufferTextureLayer(GL_FRAMEBUFFER, attachment, nativeTex,
                                              level, layer);
    }
}

uint32_t Context::checkNamedFramebufferStatus(GLObjectName framebuffer,
                                             uint32_t target) {
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return GL_FRAMEBUFFER_COMPLETE; // error already set
    if (fbo->attachments.empty())
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    for (const auto& a : fbo->attachments) {
        if (a.name == 0) return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        if (a.type == 0) { // texture attachment
            TextureObject* tex = getTexture(a.name);
            if (tex == nullptr || !tex->storageSet)
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        } else { // renderbuffer attachment
            RenderbufferObject* rbo = getRenderbuffer(a.name);
            if (rbo == nullptr || !rbo->storageSet)
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }
    }
    if (fbo->backend) return fbo->backend->checkStatus(target);
    return GL_FRAMEBUFFER_COMPLETE;
}

void Context::namedFramebufferParameteri(GLObjectName framebuffer, uint32_t pname,
                                        int param) {
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return;
    if (fbo->backend) fbo->backend->framebufferParameteri(GL_FRAMEBUFFER, pname,
                                                         param);
}

void Context::getNamedFramebufferParameteriv(GLObjectName framebuffer,
                                            uint32_t pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return;
    // FRAMEBUFFER_DEFAULT_* describe the default framebuffer; a user FBO has no
    // default dimensions, so the GL default is 0 (frontend-owned, SPEC §10).
    *params = 0;
}

void Context::getNamedFramebufferAttachmentParameteriv(GLObjectName framebuffer,
                                                     uint32_t attachment,
                                                     uint32_t pname,
                                                     int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    FramebufferObject* fbo = dsaFramebuffer(*this, framebuffer);
    if (fbo == nullptr) return;
    const FramebufferObject::Attachment* found = nullptr;
    for (const auto& a : fbo->attachments) {
        if (a.attachment == attachment) { found = &a; break; }
    }
    switch (pname) {
        case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
            *params = found ? (found->type == 0 ? GL_TEXTURE : GL_RENDERBUFFER)
                            : GL_NONE;
            break;
        case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            *params = found ? static_cast<int32_t>(found->name) : 0;
            break;
        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
            *params = found ? found->level : 0;
            break;
        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER:
            *params = found ? (found->type == 0 ? found->layer : 0) : 0;
            break;
        default:
            *params = 0;
            break;
    }
}

void Context::blitNamedFramebuffer(GLObjectName readFb, GLObjectName drawFb,
                                  int32_t srcX0, int32_t srcY0, int32_t srcX1,
                                  int32_t srcY1, int32_t dstX0, int32_t dstY0,
                                  int32_t dstX1, int32_t dstY1, uint32_t mask,
                                  uint32_t filter) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    constexpr uint32_t kValidMask =
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    if (mask & ~kValidMask) {
        setError(GLError::InvalidValue);
        return;
    }
    backend_.bindFramebuffer(GL_READ_FRAMEBUFFER,
                            namedFramebufferNativeId(*this, readFb));
    backend_.bindFramebuffer(GL_DRAW_FRAMEBUFFER,
                            namedFramebufferNativeId(*this, drawFb));
    backend_.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                            dstY1, mask, filter);
    bindFramebuffer(boundFramebuffer_); // restore tracked binding (DSA: no side effect)
}

void Context::invalidateNamedFramebufferData(GLObjectName framebuffer,
                                            int32_t numAttachments,
                                            const uint32_t* attachments) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (numAttachments < 0 || (numAttachments > 0 && attachments == nullptr)) {
        setError(GLError::InvalidValue);
        return;
    }
    backend_.bindFramebuffer(GL_FRAMEBUFFER,
                            namedFramebufferNativeId(*this, framebuffer));
    backend_.invalidateFramebuffer(GL_FRAMEBUFFER, numAttachments, attachments, 0,
                                  0, 0, 0);
    bindFramebuffer(boundFramebuffer_);
}

void Context::invalidateNamedFramebufferSubData(GLObjectName framebuffer,
                                               int32_t numAttachments,
                                               const uint32_t* attachments,
                                               int32_t x, int32_t y, int32_t width,
                                               int32_t height) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (numAttachments < 0 || (numAttachments > 0 && attachments == nullptr)) {
        setError(GLError::InvalidValue);
        return;
    }
    backend_.bindFramebuffer(GL_FRAMEBUFFER,
                            namedFramebufferNativeId(*this, framebuffer));
    backend_.invalidateFramebuffer(GL_FRAMEBUFFER, numAttachments, attachments, x,
                                  y, width, height);
    bindFramebuffer(boundFramebuffer_);
}

namespace {
// Bind the named framebuffer for clearing, drive the backend clear, then restore
// the tracked binding (DSA must not leave a side effect on the bound FBO). The
// clear value itself is pushed by the caller before invoking this (SPEC §9.2
// glClearNamedFramebuffer* does not read the context clear-color state).
void clearNamedFramebufferImpl(Context& ctx, GLObjectName framebuffer,
                               uint32_t mask) {
    ctx.backend().bindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                 namedFramebufferNativeId(ctx, framebuffer));
    ctx.backend().clear(mask);
    ctx.bindFramebuffer(ctx.boundFramebuffer()); // DSA: no binding side effect
}
} // namespace

void Context::clearNamedFramebufferiv(GLObjectName framebuffer, uint32_t buffer,
                                     int /*drawbuffer*/, const int32_t* value) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (value == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t mask = 0;
    if (buffer == GL_COLOR) {
        mask = GL_COLOR_BUFFER_BIT;
        if (GLStateSink* sink = backend_.stateSink())
            sink->clearColor(static_cast<float>(value[0]),
                             static_cast<float>(value[1]),
                             static_cast<float>(value[2]),
                             static_cast<float>(value[3]));
    } else if (buffer == GL_DEPTH) {
        mask = GL_DEPTH_BUFFER_BIT;
        if (GLStateSink* sink = backend_.stateSink())
            sink->clearDepth(static_cast<double>(value[0]));
    } else if (buffer == GL_STENCIL) {
        mask = GL_STENCIL_BUFFER_BIT; // stencil clear value: driver default 0
    }
    clearNamedFramebufferImpl(*this, framebuffer, mask);
}

void Context::clearNamedFramebufferuiv(GLObjectName framebuffer, uint32_t buffer,
                                      int /*drawbuffer*/, const uint32_t* value) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (value == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (buffer != GL_COLOR) return;
    if (GLStateSink* sink = backend_.stateSink())
        sink->clearColor(static_cast<float>(value[0]),
                         static_cast<float>(value[1]),
                         static_cast<float>(value[2]),
                         static_cast<float>(value[3]));
    clearNamedFramebufferImpl(*this, framebuffer, GL_COLOR_BUFFER_BIT);
}

void Context::clearNamedFramebufferfv(GLObjectName framebuffer, uint32_t buffer,
                                     int /*drawbuffer*/, const float* value) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (value == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t mask = 0;
    if (buffer == GL_COLOR) {
        mask = GL_COLOR_BUFFER_BIT;
        if (GLStateSink* sink = backend_.stateSink())
            sink->clearColor(value[0], value[1], value[2], value[3]);
    } else if (buffer == GL_DEPTH) {
        mask = GL_DEPTH_BUFFER_BIT;
        if (GLStateSink* sink = backend_.stateSink())
            sink->clearDepth(static_cast<double>(value[0]));
    }
    clearNamedFramebufferImpl(*this, framebuffer, mask);
}

void Context::clearNamedFramebufferfi(GLObjectName framebuffer, uint32_t buffer,
                                     int /*drawbuffer*/, float depth, int stencil) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    (void)stencil; // stencil clear value: backend sink has no stencil clear; driver default 0
    if (GLStateSink* sink = backend_.stateSink())
        sink->clearDepth(static_cast<double>(depth));
    clearNamedFramebufferImpl(*this, framebuffer, GL_DEPTH_BUFFER_BIT);
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

// --- Transform feedback (SPEC §13.3) ---

GLObjectName Context::genTransformFeedback() {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<TransformFeedbackObject>(name);
    obj->backend = backend_.resourceFactory().createTransformFeedback();
    transformFeedbacks_.emplace(name, std::move(obj));
    return name;
}

void Context::genTransformFeedbacks(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genTransformFeedback();
}

void Context::bindTransformFeedback(GLObjectName name) {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (name != 0 && transformFeedbacks_.find(name) == transformFeedbacks_.end()) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    boundTransformFeedback_ = name;
}

GLObjectName Context::boundTransformFeedback() const {
    return boundTransformFeedback_;
}

void Context::deleteTransformFeedback(GLObjectName name) {
    auto it = transformFeedbacks_.find(name);
    if (it == transformFeedbacks_.end()) return;
    if (boundTransformFeedback_ == name) boundTransformFeedback_ = 0;
    transformFeedbacks_.erase(it);
}

void Context::deleteTransformFeedbacks(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteTransformFeedback(names[i]);
}

TransformFeedbackObject* Context::getTransformFeedback(GLObjectName name) {
    auto it = transformFeedbacks_.find(name);
    return it == transformFeedbacks_.end() ? nullptr : it->second.get();
}

void Context::beginTransformFeedback(uint32_t primitiveMode) {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (transformFeedbackActive_) {
        setError(GLError::InvalidOperation); // already capturing
        return;
    }
    if (TransformFeedbackObject* tf = getTransformFeedback(boundTransformFeedback_)) {
        if (tf->backend) tf->backend->begin(primitiveMode);
    }
    transformFeedbackActive_ = true;
    transformFeedbackPaused_ = false;
}

void Context::endTransformFeedback() {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!transformFeedbackActive_) {
        setError(GLError::InvalidOperation); // not capturing
        return;
    }
    if (TransformFeedbackObject* tf = getTransformFeedback(boundTransformFeedback_)) {
        if (tf->backend) tf->backend->end();
    }
    transformFeedbackActive_ = false;
    transformFeedbackPaused_ = false;
}

void Context::pauseTransformFeedback() {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!transformFeedbackActive_ || transformFeedbackPaused_) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (TransformFeedbackObject* tf = getTransformFeedback(boundTransformFeedback_)) {
        if (tf->backend) tf->backend->pause();
    }
    transformFeedbackPaused_ = true;
}

void Context::resumeTransformFeedback() {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!transformFeedbackActive_ || !transformFeedbackPaused_) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (TransformFeedbackObject* tf = getTransformFeedback(boundTransformFeedback_)) {
        if (tf->backend) tf->backend->resume();
    }
    transformFeedbackPaused_ = false;
}

// --- Query objects (SPEC §4 / §19) ---

GLObjectName Context::genQuery() {
    if (!backend_.capabilities().isSupported(Feature::Queries)) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<QueryObject>(name);
    obj->backend = backend_.resourceFactory().createQuery();
    queries_.emplace(name, std::move(obj));
    return name;
}

void Context::genQueries(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genQuery();
}

void Context::deleteQuery(GLObjectName name) {
    auto it = queries_.find(name);
    if (it == queries_.end()) return;
    // A query active at delete time is first ended (SPEC §4: deleting an active
    // query object ends its capture on that target).
    if (it->second->active) {
        for (auto ait = activeQueries_.begin(); ait != activeQueries_.end();) {
            if (ait->second == name) ait = activeQueries_.erase(ait);
            else ++ait;
        }
    }
    queries_.erase(it);
}

void Context::deleteQueries(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteQuery(names[i]);
}

bool Context::isQuery(GLObjectName name) const {
    return queries_.find(name) != queries_.end();
}

void Context::beginQuery(uint32_t target, GLObjectName id) {
    if (!backend_.capabilities().isSupported(Feature::Queries)) {
        setError(GLError::InvalidOperation);
        return;
    }
    QueryObject* q = getQuery(id);
    if (!q) {
        setError(GLError::InvalidOperation); // ungenerated id
        return;
    }
    if (q->active || activeQueries_.count(target)) {
        setError(GLError::InvalidOperation); // already active
        return;
    }
    q->target = target;
    q->active = true;
    activeQueries_[target] = id;
    if (q->backend) q->backend->begin(target);
}

void Context::endQuery(uint32_t target) {
    if (!backend_.capabilities().isSupported(Feature::Queries)) {
        setError(GLError::InvalidOperation);
        return;
    }
    auto it = activeQueries_.find(target);
    if (it == activeQueries_.end()) {
        setError(GLError::InvalidOperation); // no active query for target
        return;
    }
    if (QueryObject* q = getQuery(it->second)) {
        if (q->backend) q->backend->end();
        q->active = false;
    }
    activeQueries_.erase(it);
}

void Context::beginQueryIndexed(uint32_t target, uint32_t /*index*/, GLObjectName id) {
    // Indexed variants only exist for primitive/tf-written counters (SPEC §4).
    if (target != GL_PRIMITIVES_GENERATED &&
        target != GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
        setError(GLError::InvalidEnum);
        return;
    }
    beginQuery(target, id);
}

void Context::endQueryIndexed(uint32_t target, uint32_t /*index*/) {
    if (target != GL_PRIMITIVES_GENERATED &&
        target != GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
        setError(GLError::InvalidEnum);
        return;
    }
    endQuery(target);
}

QueryObject* Context::getQuery(GLObjectName name) {
    auto it = queries_.find(name);
    return it == queries_.end() ? nullptr : it->second.get();
}

const QueryObject* Context::getQuery(GLObjectName name) const {
    auto it = queries_.find(name);
    return it == queries_.end() ? nullptr : it->second.get();
}

void Context::getQueryiv(uint32_t target, uint32_t pname, int32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::Queries)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!params) {
        setError(GLError::InvalidValue);
        return;
    }
    switch (pname) {
        case GL_CURRENT_QUERY: {
            auto it = activeQueries_.find(target);
            *params = (it == activeQueries_.end()) ? 0
                                                  : static_cast<int32_t>(it->second);
            return;
        }
        case GL_QUERY_COUNTER_BITS:
            // Boolean-style queries report 0 counter bits; a real driver returns
            // the counter width. The mock has no counter, so 0 is honest.
            *params = 0;
            return;
        default:
            setError(GLError::InvalidEnum);
            return;
    }
}

void Context::getQueryObjectiv(GLObjectName id, uint32_t pname, int32_t* params) {
    getQueryObjectImpl(id, pname, reinterpret_cast<void*>(params), /*is64=*/false,
                       /*isSigned=*/true);
}

void Context::getQueryObjectuiv(GLObjectName id, uint32_t pname, uint32_t* params) {
    getQueryObjectImpl(id, pname, reinterpret_cast<void*>(params), /*is64=*/false,
                       /*isSigned=*/false);
}

void Context::getQueryObjecti64v(GLObjectName id, uint32_t pname, int64_t* params) {
    getQueryObjectImpl(id, pname, reinterpret_cast<void*>(params), /*is64=*/true,
                       /*isSigned=*/true);
}

void Context::getQueryObjectui64v(GLObjectName id, uint32_t pname, uint64_t* params) {
    getQueryObjectImpl(id, pname, reinterpret_cast<void*>(params), /*is64=*/true,
                       /*isSigned=*/false);
}

// Shared body for glGetQueryObject* (SPEC §4): reads the cached result/availability
// from the backend query resource and writes it in the requested width/sign.
void Context::getQueryObjectImpl(GLObjectName id, uint32_t pname, void* params,
                                 bool is64, bool isSigned) {
    if (!backend_.capabilities().isSupported(Feature::Queries)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!params) {
        setError(GLError::InvalidValue);
        return;
    }
    QueryObject* q = getQuery(id);
    if (!q) {
        setError(GLError::InvalidOperation); // ungenerated id
        return;
    }
    int64_t value = 0;
    bool available = false;
    if (q->backend) q->backend->queryResult(&value, &available);
    switch (pname) {
        case GL_QUERY_RESULT:
            if (is64) {
                if (isSigned) *static_cast<int64_t*>(params) = value;
                else *static_cast<uint64_t*>(params) = static_cast<uint64_t>(value);
            } else {
                if (isSigned) *static_cast<int32_t*>(params) = static_cast<int32_t>(value);
                else *static_cast<uint32_t*>(params) = static_cast<uint32_t>(value);
            }
            return;
        case GL_QUERY_RESULT_AVAILABLE:
            if (is64) {
                if (isSigned) *static_cast<int64_t*>(params) = available ? GL_TRUE : GL_FALSE;
                else *static_cast<uint64_t*>(params) = available ? GL_TRUE : GL_FALSE;
            } else {
                if (isSigned) *static_cast<int32_t*>(params) = available ? GL_TRUE : GL_FALSE;
                else *static_cast<uint32_t*>(params) = available ? GL_TRUE : GL_FALSE;
            }
            return;
        default:
            setError(GLError::InvalidEnum);
            return;
    }
}

// --- Sync objects (SPEC §4 / §20, ARB_sync) ---

GLsync Context::fenceSync(uint32_t condition, uint32_t flags) {
    if (!backend_.capabilities().isSupported(Feature::SyncObjects)) {
        setError(GLError::InvalidOperation);
        return nullptr;
    }
    if (condition != GL_SYNC_GPU_COMMANDS_COMPLETE) {
        setError(GLError::InvalidEnum);
        return nullptr;
    }
    // Flush pending commands so the fence will eventually be signaled (SPEC §20:
    // the fence tracks completion of previously issued GPU commands).
    backend_.flush();
    auto obj = std::make_unique<SyncObject>(nextName_++);
    obj->condition = condition;
    obj->flags = flags;
    GLsync sync = reinterpret_cast<GLsync>(obj.get());
    syncs_.push_back(std::move(obj));
    return sync;
}

GLenum Context::clientWaitSync(GLsync sync, uint32_t /*flags*/, uint64_t /*timeout*/) {
    if (!isSync(sync)) return GL_WAIT_FAILED;
    // The mock has no real GPU timeline; a fence is treated as already satisfied.
    SyncObject* s = reinterpret_cast<SyncObject*>(sync);
    s->signaled = true;
    return GL_ALREADY_SIGNALED;
}

void Context::waitSync(GLsync sync, uint32_t /*flags*/, uint64_t /*timeout*/) {
    if (!isSync(sync)) {
        setError(GLError::InvalidValue);
        return;
    }
    // Server-side stall; no observable effect in the mock.
    reinterpret_cast<SyncObject*>(sync)->signaled = true;
}

void Context::deleteSync(GLsync sync) {
    if (!isSync(sync)) return; // non-sync / already-deleted is a silent no-op
    GLObjectName needle = reinterpret_cast<SyncObject*>(sync)->name;
    for (auto it = syncs_.begin(); it != syncs_.end(); ++it) {
        if ((*it)->name == needle) {
            syncs_.erase(it);
            return;
        }
    }
}

bool Context::isSync(GLsync sync) const {
    if (!sync) return false;
    const auto* p = reinterpret_cast<const SyncObject*>(sync);
    for (const auto& s : syncs_) {
        if (s.get() == p) return true;
    }
    return false;
}

void Context::getSynciv(GLsync sync, uint32_t pname, uint32_t bufSize,
                        int32_t* length, int32_t* values) {
    if (!isSync(sync)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (bufSize < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    SyncObject* s = reinterpret_cast<SyncObject*>(sync);
    int32_t out = 0;
    switch (pname) {
        case GL_SYNC_STATUS:
            out = s->signaled ? static_cast<int32_t>(GL_SIGNALED)
                              : static_cast<int32_t>(GL_UNSIGNALED);
            break;
        case GL_SYNC_CONDITION:
            out = static_cast<int32_t>(s->condition);
            break;
        case GL_SYNC_FLAGS:
            out = static_cast<int32_t>(s->flags);
            break;
        default:
            setError(GLError::InvalidEnum);
            return;
    }
    if (length) *length = 1;
    if (values) *values = out;
}

// --- Sampler objects (SPEC §8.2) ---

GLObjectName Context::genSampler() {
    if (!backend_.capabilities().isSupported(Feature::SamplerObjects)) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    GLObjectName name = nextName_++;
    auto obj = std::make_unique<SamplerObject>(name);
    obj->backend = backend_.resourceFactory().createSampler();
    // Register the name -> native id so the backend can bind the sampler at
    // flush time (SPEC §3/§11).
    if (obj->backend)
        backend_.bindNativeObject(name, obj->backend->nativeId());
    samplers_.emplace(name, std::move(obj));
    return name;
}

void Context::genSamplers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) names[i] = genSampler();
}

void Context::bindSampler(uint32_t unit, GLObjectName sampler) {
    if (!backend_.capabilities().isSupported(Feature::SamplerObjects)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (unit >= state_.maxCombinedTextureUnits()) {
        setError(GLError::InvalidValue); // unit out of range
        return;
    }
    if (sampler != 0 && samplers_.find(sampler) == samplers_.end()) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    state_.setSamplerBinding(unit, sampler);
}

GLObjectName Context::boundSampler(uint32_t unit) const {
    return state_.boundSamplerForUnit(unit);
}

void Context::deleteSampler(GLObjectName name) {
    auto it = samplers_.find(name);
    if (it == samplers_.end()) return;
    state_.clearSamplerBinding(name);
    samplers_.erase(it);
}

void Context::deleteSamplers(uint32_t n, const GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) deleteSampler(names[i]);
}

SamplerObject* Context::getSampler(GLObjectName name) {
    auto it = samplers_.find(name);
    return it == samplers_.end() ? nullptr : it->second.get();
}

const SamplerObject* Context::getSampler(GLObjectName name) const {
    auto it = samplers_.find(name);
    return it == samplers_.end() ? nullptr : it->second.get();
}

bool Context::isSampler(GLObjectName name) const {
    return samplers_.find(name) != samplers_.end();
}

void Context::samplerParameteri(GLObjectName sampler, uint32_t pname, int param) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSamplerParameter(pname)) {
        setError(GLError::InvalidEnum); // non-scalar / unknown pname
        return;
    }
    s->params[pname] = param;
    if (s->backend) s->backend->samplerParameteri(pname, param);
}

void Context::getSamplerParameteriv(GLObjectName sampler, uint32_t pname,
                                    int32_t* params) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isValidSamplerParameter(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    auto it = s->params.find(pname);
    *params = (it != s->params.end()) ? it->second : 0;
}

void Context::flushState() {
    GLStateSink* sink = backend_.stateSink();
    if (sink) {
        state_.apply(*sink);
        if (vertexStateDirty_) {
            if (boundVertexArray_ != 0) {
                if (VertexArrayObject* vao = getVertexArray(boundVertexArray_)) {
                    sink->bindVertexArray(boundVertexArray_);
                    // The element array buffer is part of VAO state on GLES; bind
                    // it while the VAO is bound so it is captured (SPEC §10.3.1).
                    if (vao->elementBuffer != 0) {
                        sink->bindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                         vao->elementBuffer);
                    }
                    VertexArrayObject::VertexBufferBinding empty{};
                    for (const auto& a : vao->attribs) {
                        if (a.enabled)
                            sink->enableVertexAttribArray(a.index);
                        else
                            sink->disableVertexAttribArray(a.index);
                        // Resolve the attribute's vertex buffer binding point
                        // (SPEC §10.3.1 separate model). The final attribute
                        // pointer = binding.offset + attrib.relativeoffset.
                        auto bit = vao->bindings.find(a.binding);
                        const VertexArrayObject::VertexBufferBinding& b =
                            (bit != vao->bindings.end()) ? bit->second : empty;
                        // GLES captures the attribute's buffer binding from the
                        // bound ARRAY_BUFFER at gl*VertexAttribPointer time; bind
                        // it (frontend name -> native id) before the native call.
                        // Pushed for every recorded attribute (SPEC §10: the
                        // legacy path always issued the pointer, even with no
                        // buffer bound), so the driver state stays consistent.
                        sink->bindBuffer(GL_ARRAY_BUFFER, b.buffer);
                        sink->vertexAttribPointer(
                            a.index, a.size, a.type, a.normalized, b.stride,
                            b.offset + a.relativeoffset);
                        // Non-zero divisor is pushed; 0 is the GL default so it
                        // needs no native call (SPEC §10: skip redundant state).
                        if (b.divisor != 0)
                            sink->vertexAttribDivisor(a.index, b.divisor);
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

void Context::multiDrawArrays(uint32_t mode, const int32_t* firsts,
                              const int32_t* counts, int32_t drawcount) {
    if (!backend_.capabilities().isSupported(Feature::MultiDraw)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (drawcount < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.multiDrawArrays(mode, firsts, counts, drawcount);
}

void Context::multiDrawElements(uint32_t mode, const int32_t* counts,
                                uint32_t type, const intptr_t* indices,
                                int32_t drawcount) {
    if (!backend_.capabilities().isSupported(Feature::MultiDraw)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (drawcount < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.multiDrawElements(mode, counts, type, indices, drawcount);
}

void Context::drawRangeElements(uint32_t mode, uint32_t start, uint32_t end,
                                int32_t count, uint32_t type, intptr_t indices) {
    if (!backend_.capabilities().isSupported(Feature::DrawRangeElements)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (end < start) {
        setError(GLError::InvalidValue);
        return;
    }
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.drawRangeElements(mode, start, end, count, type, indices);
}

void Context::drawElementsBaseVertex(uint32_t mode, int32_t count, uint32_t type,
                                     intptr_t indices, int32_t basevertex) {
    if (!backend_.capabilities().isSupported(Feature::DrawElementsBaseVertex)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (state_.activeProgram() == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    flushState();
    backend_.drawElementsBaseVertex(mode, count, type, indices, basevertex);
}

// --- Shaders / programs (SPEC §8) ---

GLObjectName Context::createShader(uint32_t stage) {
    Feature f = shaderStageFeature(stage);
    if (f == Feature::FeatureCount) {
        setError(GLError::InvalidEnum); // unrecognized shader type
        return 0;
    }
    if (!backend_.capabilities().isSupported(f)) {
        // Stage unsupported by the backend (e.g. geometry/tessellation/compute
        // have no GLES equivalent when emulation is absent). Report honestly
        // rather than letting the unsupported shader fail later at compile/link.
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
    // Reject GLSL versions beyond what YAGLT can translate before involving the
    // translator/backend (SPEC §8: fail fast, report honestly). Desktop profiles
    // are capped at 4.60; ES at 3.20. The COMPILE_STATUS path carries the
    // diagnostic (standard GL semantics: no glGetError for bad source).
    {
        int ver = 0;
        bool es = false;
        if (parseVersionDirective(s->source, &ver, &es)) {
            int maxVer = es ? kMaxESGLSLVersion : kMaxDesktopGLSLVersion;
            if (ver > maxVer) {
                s->compiled = false;
                s->infoLog = "YAGLT: unsupported GLSL version " + std::to_string(ver) +
                             (es ? " es (max " : " (max ") +
                             std::to_string(maxVer) + ")";
                return;
            }
        }
    }
    // Translate desktop GLSL -> backend source when a translator is wired in.
    // On failure we report the translation error honestly (no fake success).
    if (!backend_.shaderCompiler().compile(s->source, s->stage, out, err)) {
        s->compiled = false;
        s->infoLog = err;
        glcompat::log(LogCategory::Shader, LogLevel::Error)
            << "shader translation failed (stage=" << s->stage << "): " << err;
        return;
    }
    std::string blog;
    bool ok = s->backend ? s->backend->compile(out, blog) : false;
    s->compiled = ok;
    s->infoLog = blog;
    if (!ok) {
        setError(GLError::InvalidOperation);
        glcompat::log(LogCategory::Shader, LogLevel::Error)
            << "shader compile failed (stage=" << s->stage << "): " << blog;
    }
}

bool Context::isShaderCompiled(GLObjectName shader) const {
    const ShaderObject* s = getShader(shader);
    return s != nullptr && s->compiled;
}

GLint Context::getShaderiv(GLObjectName shader, uint32_t pname) {
    const ShaderObject* s = getShader(shader);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    switch (pname) {
    case GL_SHADER_TYPE: return static_cast<GLint>(s->stage);
    case GL_COMPILE_STATUS: return s->compiled ? GL_TRUE : GL_FALSE;
    case GL_DELETE_STATUS: return GL_FALSE; // frontend does not flag pending delete
    case GL_INFO_LOG_LENGTH: return static_cast<GLint>(s->infoLog.size() + 1);
    case GL_SHADER_SOURCE_LENGTH: return static_cast<GLint>(s->source.size() + 1);
    default:
        setError(GLError::InvalidEnum);
        return 0;
    }
}

GLint Context::getProgramiv(GLObjectName program, uint32_t pname) {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    switch (pname) {
    case GL_LINK_STATUS: return p->linked ? GL_TRUE : GL_FALSE;
    case GL_DELETE_STATUS: return GL_FALSE;
    case GL_ATTACHED_SHADERS:
        return static_cast<GLint>(p->attachedShaders.size());
    case GL_INFO_LOG_LENGTH: return static_cast<GLint>(p->infoLog.size() + 1);
    case GL_ACTIVE_UNIFORMS:
        return p->backend ? p->backend->activeUniformCount() : 0;
    case GL_ACTIVE_ATTRIBUTES:
        return p->backend ? p->backend->activeAttributeCount() : 0;
    case GL_ACTIVE_UNIFORM_BLOCKS:
        return p->backend ? p->backend->activeUniformBlockCount() : 0;
    default:
        setError(GLError::InvalidEnum);
        return 0;
    }
}

namespace {

bool isValidProgramInterface(uint32_t iface) {
    switch (iface) {
    case GL_UNIFORM:
    case GL_UNIFORM_BLOCK:
    case GL_ATOMIC_COUNTER_BUFFER:
    case GL_PROGRAM_INPUT:
    case GL_PROGRAM_OUTPUT:
    case GL_TRANSFORM_FEEDBACK_VARYING:
    case GL_BUFFER_VARIABLE:
    case GL_SHADER_STORAGE_BLOCK:
    case GL_VERTEX_SUBROUTINE:
    case GL_TESS_CONTROL_SUBROUTINE:
    case GL_TESS_EVALUATION_SUBROUTINE:
    case GL_GEOMETRY_SUBROUTINE:
    case GL_FRAGMENT_SUBROUTINE:
    case GL_COMPUTE_SUBROUTINE:
    case GL_VERTEX_SUBROUTINE_UNIFORM:
    case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
    case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
    case GL_GEOMETRY_SUBROUTINE_UNIFORM:
    case GL_FRAGMENT_SUBROUTINE_UNIFORM:
    case GL_COMPUTE_SUBROUTINE_UNIFORM:
        return true;
    default:
        return false;
    }
}

bool isKnownProgramResourceProperty(uint32_t prop) {
    switch (prop) {
    case GL_NAME_LENGTH:
    case GL_TYPE:
    case GL_ARRAY_SIZE:
    case GL_OFFSET:
    case GL_BLOCK_INDEX:
    case GL_ARRAY_STRIDE:
    case GL_MATRIX_STRIDE:
    case GL_IS_ROW_MAJOR:
    case GL_ATOMIC_COUNTER_BUFFER_INDEX:
    case GL_BUFFER_BINDING:
    case GL_BUFFER_DATA_SIZE:
    case GL_NUM_ACTIVE_VARIABLES:
    case GL_ACTIVE_VARIABLES:
    case GL_REFERENCED_BY_VERTEX_SHADER:
    case GL_REFERENCED_BY_TESS_CONTROL_SHADER:
    case GL_REFERENCED_BY_TESS_EVALUATION_SHADER:
    case GL_REFERENCED_BY_GEOMETRY_SHADER:
    case GL_REFERENCED_BY_FRAGMENT_SHADER:
    case GL_REFERENCED_BY_COMPUTE_SHADER:
    case GL_TOP_LEVEL_ARRAY_SIZE:
    case GL_TOP_LEVEL_ARRAY_STRIDE:
    case GL_LOCATION:
    case GL_LOCATION_COMPONENT:
    case GL_TRANSFORM_FEEDBACK_BUFFER_INDEX:
        return true;
    default:
        return false;
    }
}

} // namespace

uint32_t Context::getProgramResourceIndex(GLObjectName program,
                                          uint32_t programInterface,
                                          const std::string& name) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return GL_INVALID_INDEX;
    }
    if (!isValidProgramInterface(programInterface)) {
        setError(GLError::InvalidEnum);
        return GL_INVALID_INDEX;
    }
    return p->backend->getProgramResourceIndex(programInterface, name);
}

void Context::getProgramResourceName(GLObjectName program, uint32_t programInterface,
                                     uint32_t index, int32_t bufSize, int32_t* length,
                                     char* name) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidProgramInterface(programInterface)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t count = p->backend->programResourceCount(programInterface);
    if (index >= count) {
        setError(GLError::InvalidValue);
        return;
    }
    if (name == nullptr || bufSize == 0) {
        if (length) *length = 0;
        return;
    }
    p->backend->getProgramResourceName(programInterface, index, bufSize, length, name);
}

void Context::getProgramResourceiv(GLObjectName program, uint32_t programInterface,
                                   uint32_t index, int32_t propCount,
                                   const uint32_t* props, int32_t bufSize,
                                   int32_t* length, int32_t* params) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidProgramInterface(programInterface)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (propCount < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (propCount > 0) {
        if (props == nullptr || params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        for (int32_t i = 0; i < propCount; ++i) {
            if (!isKnownProgramResourceProperty(props[i])) {
                setError(GLError::InvalidEnum);
                return;
            }
        }
    }
    if (propCount > 0 && bufSize < propCount) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t count = p->backend->programResourceCount(programInterface);
    if (index >= count) {
        setError(GLError::InvalidValue);
        return;
    }
    if (propCount == 0) {
        if (length) *length = 0;
        return;
    }
    p->backend->getProgramResourceiv(programInterface, index, propCount, props,
                                    bufSize, length, params);
}

int32_t Context::getProgramResourceLocation(GLObjectName program,
                                            uint32_t programInterface,
                                            const std::string& name) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return -1;
    }
    if (!isValidProgramInterface(programInterface)) {
        setError(GLError::InvalidEnum);
        return -1;
    }
    return p->backend->getProgramResourceLocation(programInterface, name);
}

int32_t Context::getProgramResourceLocationIndex(GLObjectName program,
                                                 uint32_t programInterface,
                                                 const std::string& name) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return -1;
    }
    if (!isValidProgramInterface(programInterface)) {
        setError(GLError::InvalidEnum);
        return -1;
    }
    return p->backend->getProgramResourceLocationIndex(programInterface, name);
}

namespace {

bool isValidSubroutineStage(uint32_t stage) {
    switch (stage) {
    case GL_VERTEX_SHADER:
    case GL_TESS_CONTROL_SHADER:
    case GL_TESS_EVALUATION_SHADER:
    case GL_GEOMETRY_SHADER:
    case GL_FRAGMENT_SHADER:
    case GL_COMPUTE_SHADER:
        return true;
    default:
        return false;
    }
}

} // namespace

uint32_t Context::getSubroutineIndex(GLObjectName program, uint32_t shadertype,
                                     const std::string& name) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return GL_INVALID_INDEX;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return GL_INVALID_INDEX;
    }
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return GL_INVALID_INDEX;
    }
    return p->backend->getSubroutineIndex(shadertype, name);
}

int32_t Context::getSubroutineUniformLocation(GLObjectName program,
                                              uint32_t shadertype,
                                              const std::string& name) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return -1;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return -1;
    }
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return -1;
    }
    return p->backend->getSubroutineUniformLocation(shadertype, name);
}

void Context::getActiveSubroutineUniformiv(GLObjectName program, uint32_t shadertype,
                                           uint32_t index, uint32_t pname,
                                           int32_t* values) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (values == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    p->backend->getActiveSubroutineUniformiv(shadertype, index, pname, values);
}

void Context::getActiveSubroutineUniformName(GLObjectName program,
                                             uint32_t shadertype, uint32_t index,
                                             int32_t bufSize, int32_t* length,
                                             char* name) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (name == nullptr || bufSize == 0) {
        if (length) *length = 0;
        return;
    }
    p->backend->getActiveSubroutineUniformName(shadertype, index, bufSize, length,
                                              name);
}

void Context::getActiveSubroutineName(GLObjectName program, uint32_t shadertype,
                                     uint32_t index, int32_t bufSize,
                                     int32_t* length, char* name) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (name == nullptr || bufSize == 0) {
        if (length) *length = 0;
        return;
    }
    p->backend->getActiveSubroutineName(shadertype, index, bufSize, length, name);
}

void Context::uniformSubroutinesuiv(uint32_t shadertype, int32_t count,
                                    const uint32_t* indices) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return;
    }
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (count < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    bp->uniformSubroutinesuiv(shadertype, count, indices);
}

void Context::getUniformSubroutineuiv(uint32_t shadertype, int32_t location,
                                      uint32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return;
    }
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    bp->getUniformSubroutineuiv(shadertype, location, params);
}

void Context::getIntegerv(uint32_t pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    int32_t buf[4] = {0, 0, 0, 0};
    int n = state_.getInteger(static_cast<GLenum>(pname), buf);
    if (n == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    for (int i = 0; i < n; ++i) params[i] = buf[i];
}

void Context::getBooleanv(uint32_t pname, unsigned char* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    unsigned char buf[4] = {0, 0, 0, 0};
    int n = state_.getBoolean(static_cast<GLenum>(pname), buf);
    if (n == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    for (int i = 0; i < n; ++i) params[i] = buf[i];
}

void Context::getFloatv(uint32_t pname, float* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    float buf[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int n = state_.getFloat(static_cast<GLenum>(pname), buf);
    if (n == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    for (int i = 0; i < n; ++i) params[i] = buf[i];
}

void Context::getDoublev(uint32_t pname, double* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    double buf[4] = {0.0, 0.0, 0.0, 0.0};
    int n = state_.getDouble(static_cast<GLenum>(pname), buf);
    if (n == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    for (int i = 0; i < n; ++i) params[i] = buf[i];
}

bool Context::isEnabled(uint32_t cap) {
    bool enabled = false;
    if (!state_.isCapabilityEnabled(static_cast<GLenum>(cap), &enabled)) {
        setError(GLError::InvalidEnum);
        return false;
    }
    return enabled;
}

namespace {
// Copy `log` into `out` (up to bufSize-1 chars, nul-terminated). Sets *length to
// the number of characters written, excluding the nul. Honors bufSize==0.
void copyInfoLog(const std::string& log, uint32_t bufSize, int32_t* length,
                 char* out) {
    if (length) *length = 0;
    if (out == nullptr || bufSize == 0) return;
    uint32_t n = 0;
    for (char c : log) {
        if (n + 1 >= bufSize) break; // leave room for nul
        out[n++] = c;
    }
    out[n] = '\0';
    if (length) *length = static_cast<int32_t>(n);
}
} // namespace

void Context::getShaderInfoLog(GLObjectName shader, uint32_t bufSize,
                              int32_t* length, char* infoLog) {
    const ShaderObject* s = getShader(shader);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        copyInfoLog({}, bufSize, length, infoLog);
        return;
    }
    copyInfoLog(s->infoLog, bufSize, length, infoLog);
}

void Context::getProgramInfoLog(GLObjectName program, uint32_t bufSize,
                               int32_t* length, char* infoLog) {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        copyInfoLog({}, bufSize, length, infoLog);
        return;
    }
    copyInfoLog(p->infoLog, bufSize, length, infoLog);
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

// --- Program pipelines (SPEC §7.4) ---

namespace {

constexpr uint32_t kPipelineStageMask =
    GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT | GL_GEOMETRY_SHADER_BIT |
    GL_TESS_CONTROL_SHADER_BIT | GL_TESS_EVALUATION_SHADER_BIT |
    GL_COMPUTE_SHADER_BIT;

// Map a glGetProgramPipelineiv stage pname to its stage bit (0 if not a stage).
uint32_t stageBitForProgramPipelinePname(uint32_t pname) {
    switch (pname) {
        case GL_VERTEX_SHADER:        return GL_VERTEX_SHADER_BIT;
        case GL_FRAGMENT_SHADER:      return GL_FRAGMENT_SHADER_BIT;
        case GL_GEOMETRY_SHADER:      return GL_GEOMETRY_SHADER_BIT;
        case GL_TESS_CONTROL_SHADER:  return GL_TESS_CONTROL_SHADER_BIT;
        case GL_TESS_EVALUATION_SHADER: return GL_TESS_EVALUATION_SHADER_BIT;
        case GL_COMPUTE_SHADER:       return GL_COMPUTE_SHADER_BIT;
        default:                      return 0;
    }
}

} // namespace

GLObjectName Context::createShaderProgramv(uint32_t type, int32_t count,
                                            const char* const* strings) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        return 0;
    }
    if (count < 0) {
        setError(GLError::InvalidValue);
        return 0;
    }
    GLObjectName sh = createShader(type);
    if (sh == 0) return 0; // capability/type error already recorded
    std::string src;
    for (int32_t i = 0; i < count; ++i) {
        if (strings && strings[i]) src += strings[i];
    }
    shaderSource(sh, src);
    compileShader(sh);
    GLObjectName prog = createProgram();
    if (prog == 0) {
        deleteShader(sh);
        return 0;
    }
    ProgramObject* po = programs_[prog].get();
    po->separable = true; // created for use in a pipeline (PROGRAM_SEPARABLE)
    attachShader(prog, sh);
    // Link without surfacing a GL error on failure: glCreateShaderProgramv
    // reports link failure via LINK_STATUS / INFO_LOG, not glGetError.
    for (GLObjectName s : po->attachedShaders) {
        if (ShaderObject* so = getShader(s)) {
            if (so->backend && po->backend) po->backend->attach(*so->backend);
        }
    }
    std::string log;
    bool ok = po->backend ? po->backend->link(log) : false;
    po->linked = ok;
    po->infoLog = log;
    if (po->backend) backend_.bindNativeObject(prog, po->backend->nativeId());
    deleteShader(sh);
    return prog;
}

void Context::genProgramPipelines(uint32_t n, GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (uint32_t i = 0; i < n; ++i) {
        GLObjectName name = nextName_++;
        pipelines_.emplace(name,
                           std::make_unique<ProgramPipelineObject>(name));
        names[i] = name;
    }
}

void Context::deleteProgramPipelines(uint32_t n, const GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (uint32_t i = 0; i < n; ++i) {
        auto it = pipelines_.find(names[i]);
        if (it == pipelines_.end()) continue;
        if (boundProgramPipeline_ == names[i]) boundProgramPipeline_ = 0;
        pipelines_.erase(it);
    }
}

bool Context::isProgramPipeline(GLObjectName name) const {
    return name != 0 && pipelines_.find(name) != pipelines_.end();
}

void Context::bindProgramPipeline(GLObjectName pipeline) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (pipeline != 0 && getProgramPipeline(pipeline) == nullptr) {
        setError(GLError::InvalidOperation); // not a generated pipeline name
        return;
    }
    boundProgramPipeline_ = pipeline;
    state_.bindProgramPipeline(pipeline);
}

void Context::useProgramStages(GLObjectName pipeline, uint32_t stages,
                               GLObjectName program) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramPipelineObject* p = getProgramPipeline(pipeline);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    uint32_t effectiveStages = stages;
    if (stages != GL_ALL_SHADER_BITS) {
        if (stages & ~kPipelineStageMask) {
            setError(GLError::InvalidValue);
            return;
        }
    } else {
        effectiveStages = kPipelineStageMask;
    }
    GLObjectName prog = program;
    if (program == 0) prog = p->activeProgram; // use glActiveShaderProgram's
    if (prog != 0) {
        ProgramObject* po = getProgram(prog);
        if (po == nullptr || !po->linked) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (!po->separable) {
            setError(GLError::InvalidOperation); // must be a separable program
            return;
        }
    }
    for (uint32_t bit = 1; bit <= kPipelineStageMask; bit <<= 1) {
        if (effectiveStages & bit) p->stagePrograms[bit] = prog;
    }
}

void Context::activeShaderProgram(GLObjectName pipeline, GLObjectName program) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramPipelineObject* p = getProgramPipeline(pipeline);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (program != 0 && getProgram(program) == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    p->activeProgram = program;
}

void Context::getProgramPipelineiv(GLObjectName pipeline, uint32_t pname,
                                   int32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramPipelineObject* p = getProgramPipeline(pipeline);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (pname == GL_ACTIVE_PROGRAM) {
        *params = static_cast<int32_t>(p->activeProgram);
        return;
    }
    if (pname == GL_VALID_STATUS) {
        *params = p->validated ? 1 : 0;
        return;
    }
    if (pname == GL_INFO_LOG_LENGTH) {
        *params = static_cast<int32_t>(p->infoLog.size() + 1);
        return;
    }
    uint32_t bit = stageBitForProgramPipelinePname(pname);
    if (bit != 0) {
        auto it = p->stagePrograms.find(bit);
        *params = static_cast<int32_t>(it == p->stagePrograms.end() ? 0 : it->second);
        return;
    }
    setError(GLError::InvalidEnum);
}

void Context::validateProgramPipeline(GLObjectName pipeline) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramPipelineObject* p = getProgramPipeline(pipeline);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    // The frontend cannot truly validate GPU linkage, but useProgramStages only
    // stores well-formed stage mappings, so attest validity and clear any log.
    p->validated = true;
    p->infoLog.clear();
}

void Context::getProgramPipelineInfoLog(GLObjectName pipeline, uint32_t bufSize,
                                        int32_t* length, char* infoLog) {
    if (!backend_.capabilities().isSupported(Feature::ProgramPipelines)) {
        setError(GLError::InvalidOperation);
        copyInfoLog({}, bufSize, length, infoLog);
        return;
    }
    ProgramPipelineObject* p = getProgramPipeline(pipeline);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        copyInfoLog({}, bufSize, length, infoLog);
        return;
    }
    copyInfoLog(p->infoLog, bufSize, length, infoLog);
}

ProgramPipelineObject* Context::getProgramPipeline(GLObjectName name) {
    auto it = pipelines_.find(name);
    return it == pipelines_.end() ? nullptr : it->second.get();
}

const ProgramPipelineObject* Context::getProgramPipeline(GLObjectName name) const {
    auto it = pipelines_.find(name);
    return it == pipelines_.end() ? nullptr : it->second.get();
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
    auto* vao = getVertexArray(boundVertexArray_);
    auto& a = vao->attrib(index);
    a.size = size;
    a.type = type;
    a.normalized = normalized;
    a.stride = stride;
    a.offset = offset;
    a.buffer = boundBuffer(GL_ARRAY_BUFFER);
    a.enabled = true;
    // Default to the single-binding model: attribute i is fed by binding i.
    a.binding = index;
    a.relativeoffset = 0;
    // Mirror the attribute's buffer/offset/stride/divisor into the binding so the
    // flush can derive the native vertexAttribPointer purely from binding points
    // (unifying the legacy and DSA paths).
    auto& b = vao->bindings[index];
    b.buffer = a.buffer;
    b.offset = offset;
    b.stride = stride;
    b.divisor = a.divisor;
    vertexStateDirty_ = true;
}

void Context::vertexAttribDivisor(uint32_t index, uint32_t divisor) {
    if (boundVertexArray_ == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!backend_.capabilities().isSupported(Feature::VertexAttribDivisor)) {
        setError(GLError::InvalidOperation);
        return;
    }
    auto* vao = getVertexArray(boundVertexArray_);
    auto& a = vao->attrib(index);
    a.divisor = divisor;
    vao->bindings[a.binding].divisor = divisor;
    vertexStateDirty_ = true;
}

// --- Direct State Access vertex arrays (SPEC §10.3.1) ---

void Context::createVertexArrays(uint32_t n, GLObjectName* names) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (uint32_t i = 0; i < n; ++i) names[i] = genVertexArray();
}

void Context::vertexArrayElementBuffer(GLObjectName vaoName,
                                       GLObjectName buffer) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated VAO name
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation); // ungenerated buffer name
        return;
    }
    vao->elementBuffer = buffer;
    vertexStateDirty_ = true;
}

void Context::enableVertexArrayAttrib(GLObjectName vaoName, uint32_t index) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    vao->attrib(index).enabled = true;
    vertexStateDirty_ = true;
}

void Context::disableVertexArrayAttrib(GLObjectName vaoName, uint32_t index) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    vao->attrib(index).enabled = false;
    vertexStateDirty_ = true;
}

void Context::vertexArrayVertexBuffer(GLObjectName vaoName,
                                      uint32_t bindingindex,
                                      GLObjectName buffer, intptr_t offset,
                                      int32_t stride) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    auto& b = vao->bindings[bindingindex];
    b.buffer = buffer;
    b.offset = offset;
    b.stride = stride;
    vertexStateDirty_ = true;
}

void Context::vertexArrayVertexBuffers(GLObjectName vaoName, uint32_t first,
                                       uint32_t count, const GLObjectName* buffers,
                                       const intptr_t* offsets,
                                       const int32_t* strides) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (buffers == nullptr || offsets == nullptr || strides == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t bindingindex = first + i;
        if (buffers[i] != 0 &&
            buffers_.find(buffers[i]) == buffers_.end()) {
            setError(GLError::InvalidOperation); // ungenerated buffer name
            return;
        }
        auto& b = vao->bindings[bindingindex];
        b.buffer = buffers[i];
        b.offset = offsets[i];
        b.stride = strides[i];
    }
    vertexStateDirty_ = true;
}

// Shared body for the three *Attrib*Format variants (SPEC §10.3.1): they differ
// only in whether the attribute is normalized and the value type family.
static void setAttribFormat(VertexArrayObject& vao, uint32_t attribindex,
                            int32_t size, uint32_t type, bool normalized,
                            uint32_t relativeoffset) {
    auto& a = vao.attrib(attribindex);
    a.size = size;
    a.type = type;
    a.normalized = normalized;
    a.relativeoffset = relativeoffset;
}

void Context::vertexArrayAttribFormat(GLObjectName vaoName, uint32_t attribindex,
                                      int32_t size, uint32_t type, bool normalized,
                                      uint32_t relativeoffset) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (size < 1 || size > 4) {
        setError(GLError::InvalidValue);
        return;
    }
    setAttribFormat(*vao, attribindex, size, type, normalized, relativeoffset);
    vertexStateDirty_ = true;
}

void Context::vertexArrayAttribIFormat(GLObjectName vaoName, uint32_t attribindex,
                                       int32_t size, uint32_t type,
                                       uint32_t relativeoffset) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (size < 1 || size > 4) {
        setError(GLError::InvalidValue);
        return;
    }
    // Integer attributes are never normalized (SPEC §10.3.1, glVertexArrayAttrib
    // IFormat).
    setAttribFormat(*vao, attribindex, size, type, false, relativeoffset);
    vertexStateDirty_ = true;
}

void Context::vertexArrayAttribLFormat(GLObjectName vaoName, uint32_t attribindex,
                                       int32_t size, uint32_t type,
                                       uint32_t relativeoffset) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (size < 1 || size > 4) {
        setError(GLError::InvalidValue);
        return;
    }
    // Double-precision attributes are never normalized (SPEC §10.3.1, glVertex
    // ArrayAttribLFormat).
    setAttribFormat(*vao, attribindex, size, type, false, relativeoffset);
    vertexStateDirty_ = true;
}

void Context::vertexArrayAttribBinding(GLObjectName vaoName, uint32_t attribindex,
                                       uint32_t bindingindex) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    vao->attrib(attribindex).binding = bindingindex;
    vertexStateDirty_ = true;
}

void Context::vertexArrayBindingDivisor(GLObjectName vaoName,
                                        uint32_t bindingindex, uint32_t divisor) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vao = getVertexArray(vaoName);
    if (vao == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    vao->bindings[bindingindex].divisor = divisor;
    vertexStateDirty_ = true;
}

void Context::pixelStorei(uint32_t pname, int param) {
    // Push only when the value actually changed (SPEC §10: avoid redundant
    // backend calls). The backend's initial pixel-store state matches the GL
    // default, so an unchanged value needs no push before a texImage upload.
    if (state_.setPixelStorei(pname, param)) {
        if (GLStateSink* sink = backend_.stateSink()) {
            sink->pixelStorei(pname, param);
        }
    }
}

void Context::setViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    state_.setViewport(x, y, width, height);
}

void Context::setScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    state_.setScissor(x, y, width, height);
}

void Context::setClearColor(float r, float g, float b, float a) {
    state_.setClearColor(r, g, b, a);
}

void Context::setClearDepth(double d) {
    state_.setClearDepth(d);
}

void Context::clear(uint32_t mask) {
    // glClear accepts only the color/depth/stencil buffer bits; any other bit
    // is GL_INVALID_VALUE (SPEC §2.1).
    constexpr uint32_t kValidMask =
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    if (mask & ~kValidMask) {
        setError(GLError::InvalidValue);
        return;
    }
    // Push tracked state (including the clear color/depth) before issuing the
    // native clear so the driver clears with the current values (SPEC §10).
    flushState();
    backend_.clear(mask);
}

namespace {
// Valid draw buffer names (SPEC §15): GL_NONE, GL_BACK (default framebuffer), or
// GL_COLOR_ATTACHMENTi (user framebuffers).
bool isValidDrawBuffer(GLenum buf) {
    if (buf == GL_NONE || buf == GL_BACK) return true;
    if (buf >= GL_COLOR_ATTACHMENT0 &&
        buf <= GL_COLOR_ATTACHMENT0 + 0x0F) return true;
    return false;
}
bool isValidReadBuffer(GLenum buf) {
    switch (buf) {
        case GL_NONE:
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
            return true;
        default:
            return buf >= GL_COLOR_ATTACHMENT0 &&
                   buf <= GL_COLOR_ATTACHMENT0 + 0x0F;
    }
}
} // namespace

void Context::drawBuffers(int32_t n, const GLenum* bufs) {
    if (n <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (bufs == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    std::vector<GLenum> selection;
    selection.reserve(n);
    for (int32_t i = 0; i < n; ++i) {
        if (!isValidDrawBuffer(bufs[i])) {
            setError(GLError::InvalidEnum);
            return;
        }
        selection.push_back(bufs[i]);
    }
    state_.setDrawBuffers(selection);
}

void Context::readBuffer(GLenum buf) {
    if (!isValidReadBuffer(buf)) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setReadBuffer(buf);
}

void Context::logicOp(uint32_t mode) {
    if (!backend_.capabilities().isSupported(Feature::LogicOp)) {
        setError(GLError::InvalidOperation);
        return;
    }
    state_.setLogicOp(mode);
}

void Context::primitiveRestartIndex(uint32_t index) {
    state_.setPrimitiveRestartIndex(index);
}

void Context::polygonMode(GLenum face, GLenum mode) {
    const bool validFace = (face == GL_FRONT || face == GL_BACK ||
                            face == GL_FRONT_AND_BACK);
    const bool validMode =
        (mode == GL_POINT || mode == GL_LINE || mode == GL_FILL);
    if (!validFace || !validMode) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setPolygonMode(face, mode);
}

void Context::sampleMaski(uint32_t maskNumber, uint32_t mask) {
    if (maskNumber >= GLStateTracker::kMaxSampleMaskWords) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setSampleMaski(maskNumber, mask);
}

void Context::minSampleShading(float value) {
    if (value < 0.0f || value > 1.0f) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setMinSampleShading(value);
}

void Context::blitFramebuffer(int32_t srcX0, int32_t srcY0, int32_t srcX1,
                             int32_t srcY1, int32_t dstX0, int32_t dstY0,
                             int32_t dstX1, int32_t dstY1, uint32_t mask,
                             uint32_t filter) {
    // A mask with bits outside color/depth/stencil is invalid (SPEC §15).
    constexpr uint32_t kValidMask =
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    if (mask & ~kValidMask) {
        setError(GLError::InvalidValue);
        return;
    }
    flushState();
    backend_.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                            mask, filter);
}

void Context::invalidateFramebuffer(uint32_t target, int32_t numAttachments,
                                    const uint32_t* attachments) {
    invalidateSubFramebuffer(target, numAttachments, attachments, 0, 0, 0, 0);
}

void Context::invalidateSubFramebuffer(uint32_t target, int32_t numAttachments,
                                      const uint32_t* attachments, int32_t x,
                                      int32_t y, int32_t width, int32_t height) {
    if (numAttachments < 0 ||
        (numAttachments > 0 && attachments == nullptr)) {
        setError(GLError::InvalidValue);
        return;
    }
    if (width < 0 || height < 0 || x < 0 || y < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    flushState();
    backend_.invalidateFramebuffer(target, numAttachments, attachments, x, y, width,
                                 height);
}

void Context::flushCommands() {
    backend_.flush();
}

void Context::finishCommands() {
    backend_.finish();
}

void Context::readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                         uint32_t format, uint32_t type, void* pixels) {
    if (width <= 0 || height <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    // Flush tracked state first so the backend reads the current framebuffer.
    flushState();
    backend_.readPixels(x, y, width, height, format, type, pixels);
}

// --- Uniforms (SPEC §8) ---

BackendProgram* Context::activeBackendProgram() {
    GLObjectName prog = state_.activeProgram();
    if (prog == 0) return nullptr;
    ProgramObject* p = getProgram(prog);
    if (p == nullptr || !p->linked || !p->backend) return nullptr;
    return p->backend.get();
}

int Context::getUniformLocation(GLObjectName program, const std::string& name) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return -1;
    }
    return p->backend->getUniformLocation(name);
}

void Context::uniform1f(int loc, float v0) {
    if (loc < 0) return; // silent no-op (desktop semantics)
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform1f(loc, v0);
}

void Context::uniform2f(int loc, float v0, float v1) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform2f(loc, v0, v1);
}

void Context::uniform3f(int loc, float v0, float v1, float v2) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform3f(loc, v0, v1, v2);
}

void Context::uniform4f(int loc, float v0, float v1, float v2, float v3) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform4f(loc, v0, v1, v2, v3);
}

void Context::uniform1i(int loc, int v0) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform1i(loc, v0);
}

void Context::uniform2i(int loc, int v0, int v1) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform2i(loc, v0, v1);
}

void Context::uniform3i(int loc, int v0, int v1, int v2) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform3i(loc, v0, v1, v2);
}

void Context::uniform4i(int loc, int v0, int v1, int v2, int v3) {
    if (loc < 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform4i(loc, v0, v1, v2, v3);
}

void Context::uniform1fv(int loc, const float* v, int count) {
    if (loc < 0 || v == nullptr || count <= 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform1fv(loc, v, count);
}

void Context::uniform1iv(int loc, const int* v, int count) {
    if (loc < 0 || v == nullptr || count <= 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniform1iv(loc, v, count);
}

void Context::uniformMatrix4fv(int loc, const float* m, int count, bool transpose) {
    if (loc < 0 || m == nullptr || count <= 0) return;
    BackendProgram* bp = activeBackendProgram();
    if (bp == nullptr) { setError(GLError::InvalidOperation); return; }
    bp->uniformMatrix4fv(loc, m, count, transpose);
}

} // namespace glcompat
