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
    // Desktop GL rejects non-power-of-two / invalid sizes depending on feature;
    // we record storage and forward to the backend. Negative dimensions are an
    // INVALID_VALUE on the real API.
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
    // Replace existing level or append.
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
                    for (const auto& a : vao->attribs) {
                        if (a.enabled)
                            sink->enableVertexAttribArray(a.index);
                        else
                            sink->disableVertexAttribArray(a.index);
                        // GLES captures the attribute's buffer binding from the
                        // ARRAY_BUFFER bound at gl*VertexAttribPointer time; bind
                        // it (frontend name -> native id) before the native call.
                        sink->bindBuffer(GL_ARRAY_BUFFER, a.buffer);
                        sink->vertexAttribPointer(a.index, a.size, a.type,
                                                  a.normalized, a.stride,
                                                  a.offset);
                        // Non-zero divisor is pushed; 0 is the GL default so it
                        // needs no native call (SPEC §10: skip redundant state).
                        if (a.divisor != 0)
                            sink->vertexAttribDivisor(a.index, a.divisor);
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
    a.buffer = boundBuffer(GL_ARRAY_BUFFER);
    a.enabled = true;
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
    getVertexArray(boundVertexArray_)->attrib(index).divisor = divisor;
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
