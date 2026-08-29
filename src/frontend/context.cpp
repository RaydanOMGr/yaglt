#include "glcompat/frontend/context.hpp"
#include "glcompat/core/capabilities.hpp"
#include "glcompat/core/factory.hpp"
#include "glcompat/core/log.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {
// Number of scalar elements a given texture-parameter pname carries for the
// integer setter variants (glTexParameterIiv / glTexParameterIuiv), which do not
// pass an explicit count (it is derived from the pname, matching desktop GL).
int texParamElementCount(uint32_t pname) {
    // GL_TEXTURE_BORDER_COLOR is the only multi-element integer texture param.
    if (pname == 0x1003 /* GL_TEXTURE_BORDER_COLOR */) return 4;
    return 1;
}
}
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

const GLubyte* Context::getStringi(GLenum name, uint32_t index) {
    // Only GL_EXTENSIONS is indexable (SPEC §22.2). This frontend exposes no
    // extensions, so the valid index range is empty and every index is out of
    // range.
    if (name != GL_EXTENSIONS) {
        setError(GLError::InvalidEnum);
        return nullptr;
    }
    setError(GLError::InvalidValue);
    return nullptr;
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

bool Context::isBuffer(GLObjectName name) const {
    return buffers_.find(name) != buffers_.end();
}

void Context::genBuffers(uint32_t n, GLObjectName* names) {
    for (uint32_t i = 0; i < n; ++i) {
        names[i] = genBuffer();
    }
}

void Context::createBuffers(uint32_t n, GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
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

void Context::getBufferParameteri64v(uint32_t target, uint32_t pname,
                                     int64_t* params) {
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
    case GL_BUFFER_SIZE: *params = static_cast<int64_t>(obj->size); break;
    case GL_BUFFER_USAGE: *params = static_cast<int64_t>(obj->usage); break;
    case GL_BUFFER_ACCESS: *params = static_cast<int64_t>(obj->mapAccess); break;
    case GL_BUFFER_ACCESS_FLAGS:
        *params = static_cast<int64_t>(obj->immutableFlags); break;
    case GL_BUFFER_IMMUTABLE_STORAGE:
        *params = obj->immutable ? GL_TRUE : GL_FALSE; break;
    case GL_BUFFER_MAPPED:
        *params = obj->mapped ? GL_TRUE : GL_FALSE; break;
    case GL_BUFFER_MAP_LENGTH:
        *params = static_cast<int64_t>(obj->mapLength); break;
    case GL_BUFFER_MAP_OFFSET:
        *params = static_cast<int64_t>(obj->mapOffset); break;
    default:
        setError(GLError::InvalidEnum);
        *params = 0;
        return;
    }
}

void Context::getNamedBufferParameteriv(GLObjectName buffer, uint32_t pname,
                                        int32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated name
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
        setError(GLError::InvalidEnum);
        *params = 0;
        return;
    }
    }

namespace {

// The internalformat-query pname universe from SPEC §22.3 (ARB_internalformat_query2,
// core in GL 4.3+). Only these pnames are accepted; any other pname reports
// GL_INVALID_ENUM.
bool isKnownInternalformatPname(uint32_t pname) {
    switch (pname) {
    case GL_NUM_SAMPLE_COUNTS:
    case GL_SAMPLES:
    case GL_INTERNALFORMAT_SUPPORTED:
    case GL_INTERNALFORMAT_PREFERRED:
    case GL_INTERNALFORMAT_RED_SIZE:
    case GL_INTERNALFORMAT_GREEN_SIZE:
    case GL_INTERNALFORMAT_BLUE_SIZE:
    case GL_INTERNALFORMAT_ALPHA_SIZE:
    case GL_INTERNALFORMAT_DEPTH_SIZE:
    case GL_INTERNALFORMAT_STENCIL_SIZE:
    case GL_INTERNALFORMAT_SHARED_SIZE:
    case GL_INTERNALFORMAT_RED_TYPE:
    case GL_INTERNALFORMAT_GREEN_TYPE:
    case GL_INTERNALFORMAT_BLUE_TYPE:
    case GL_INTERNALFORMAT_ALPHA_TYPE:
    case GL_INTERNALFORMAT_DEPTH_TYPE:
    case GL_INTERNALFORMAT_STENCIL_TYPE:
    case GL_INTERNALFORMAT_COLOR_COMPONENTS:
    case GL_INTERNALFORMAT_COLOR_RENDERABLE:
    case GL_INTERNALFORMAT_DEPTH_RENDERABLE:
    case GL_INTERNALFORMAT_STENCIL_RENDERABLE:
    case GL_INTERNALFORMAT_FRAGMENT_LOAD_STORE:
    case GL_INTERNALFORMAT_VERTEX_ATOMIC:
    case GL_INTERNALFORMAT_FRAGMENT_ATOMIC:
    case GL_INTERNALFORMAT_TEXEL_SIZE:
    case GL_INTERNALFORMAT_TEXTURE_COMPRESSED:
    case GL_INTERNALFORMAT_TEXTURE_COMPRESSED_BLOCK_WIDTH:
    case GL_INTERNALFORMAT_TEXTURE_COMPRESSED_BLOCK_HEIGHT:
    case GL_INTERNALFORMAT_TEXTURE_COMPRESSED_BLOCK_SIZE:
    case GL_INTERNALFORMAT_FRAMEBUFFER_BLEND:
    case GL_INTERNALFORMAT_READ_PIXELS:
    case GL_INTERNALFORMAT_READ_PIXELS_FORMAT:
    case GL_INTERNALFORMAT_READ_PIXELS_TYPE:
    case GL_INTERNALFORMAT_TEXTURE_IMAGE_FORMAT:
    case GL_INTERNALFORMAT_TEXTURE_IMAGE_TYPE:
    case GL_INTERNALFORMAT_GET_TEXTURE_IMAGE_FORMAT:
    case GL_INTERNALFORMAT_GET_TEXTURE_IMAGE_TYPE:
    case GL_INTERNALFORMAT_MANUAL_GENERATE_MIPMAP:
    case GL_INTERNALFORMAT_AUTO_GENERATE_MIPMAP:
    case GL_INTERNALFORMAT_SRGB_READ:
    case GL_INTERNALFORMAT_SRGB_WRITE:
    case GL_INTERNALFORMAT_SRGB_RENDERABLE:
        return true;
    default:
        return false;
    }
}

} // namespace

void Context::getInternalformativ(uint32_t target, uint32_t internalformat,
                                 uint32_t pname, int32_t bufSize,
                                 int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isKnownInternalformatPname(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    backend_.getInternalformativ(target, internalformat, pname, bufSize, params);
}

void Context::getInternalformati64v(uint32_t target, uint32_t internalformat,
                                   uint32_t pname, int32_t bufSize,
                                   int64_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isKnownInternalformatPname(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    backend_.getInternalformati64v(target, internalformat, pname, bufSize, params);
}

void Context::getMultisamplefv(uint32_t pname, uint32_t index, float* val) {
    if (val == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (pname != GL_SAMPLE_POSITION) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= backend_.getMultisampleSampleCount()) {
        setError(GLError::InvalidValue);
        return;
    }
    backend_.getMultisamplefv(pname, index, val);
}

void Context::getNamedBufferParameteri64v(GLObjectName buffer, uint32_t pname,

                                           int64_t* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    switch (pname) {
    case GL_BUFFER_SIZE: *params = static_cast<int64_t>(obj->size); break;
    case GL_BUFFER_USAGE: *params = static_cast<int64_t>(obj->usage); break;
    case GL_BUFFER_ACCESS: *params = static_cast<int64_t>(obj->mapAccess); break;
    case GL_BUFFER_ACCESS_FLAGS:
        *params = static_cast<int64_t>(obj->immutableFlags); break;
    case GL_BUFFER_IMMUTABLE_STORAGE:
        *params = obj->immutable ? GL_TRUE : GL_FALSE; break;
    case GL_BUFFER_MAPPED:
        *params = obj->mapped ? GL_TRUE : GL_FALSE; break;
    case GL_BUFFER_MAP_LENGTH:
        *params = static_cast<int64_t>(obj->mapLength); break;
    case GL_BUFFER_MAP_OFFSET:
        *params = static_cast<int64_t>(obj->mapOffset); break;
    default:
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
    obj->mapPointer = obj->store.data() + static_cast<size_t>(offset);
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
    obj->mapPointer = nullptr;
    return true;
}

namespace {

// Per-internalformat layout used by Clear*Buffer* (SPEC §6 / table 8.24).
struct BufferFormatInfo {
    int components;     // 1..4
    int bytesPerComp;   // 1, 2, or 4
    bool normalized;    // normalized integer (UNORM/SNORM) vs pure integer/float
    bool isFloat;       // float / half-float base type
    bool isSigned;      // for pure integer or normalized signed
};

// Resolve a sized internal format to its component layout. Covers the practical
// subset of table 8.24 used for buffer clears; packed/exotic formats
// (e.g. R11F_G11F_B10F, RGB10_A2) intentionally return false so the caller can
// report GL_INVALID_ENUM honestly.
bool lookupBufferFormat(uint32_t internalformat, BufferFormatInfo& info) {
    switch (internalformat) {
    case GL_R8: info = {1,1,true,false,false}; return true;
    case GL_RG8: info = {2,1,true,false,false}; return true;
    case GL_RGB8: info = {3,1,true,false,false}; return true;
    case GL_RGBA8: info = {4,1,true,false,false}; return true;
    case GL_SRGB8: info = {3,1,true,false,false}; return true;
    case GL_SRGB8_ALPHA8: info = {4,1,true,false,false}; return true;
    case GL_R16F: info = {1,2,false,true,false}; return true;
    case GL_RG16F: info = {2,2,false,true,false}; return true;
    case GL_RGB16F: info = {3,2,false,true,false}; return true;
    case GL_RGBA16F: info = {4,2,false,true,false}; return true;
    case GL_R32F: info = {1,4,false,true,false}; return true;
    case GL_RG32F: info = {2,4,false,true,false}; return true;
    case GL_RGB32F: info = {3,4,false,true,false}; return true;
    case GL_RGBA32F: info = {4,4,false,true,false}; return true;
    case GL_R8I: info = {1,1,false,false,true}; return true;
    case GL_R8UI: info = {1,1,false,false,false}; return true;
    case GL_RG8I: info = {2,1,false,false,true}; return true;
    case GL_RG8UI: info = {2,1,false,false,false}; return true;
    case GL_RGB8I: info = {3,1,false,false,true}; return true;
    case GL_RGB8UI: info = {3,1,false,false,false}; return true;
    case GL_RGBA8I: info = {4,1,false,false,true}; return true;
    case GL_RGBA8UI: info = {4,1,false,false,false}; return true;
    case GL_R16I: info = {1,2,false,false,true}; return true;
    case GL_R16UI: info = {1,2,false,false,false}; return true;
    case GL_RG16I: info = {2,2,false,false,true}; return true;
    case GL_RG16UI: info = {2,2,false,false,false}; return true;
    case GL_RGB16I: info = {3,2,false,false,true}; return true;
    case GL_RGB16UI: info = {3,2,false,false,false}; return true;
    case GL_RGBA16I: info = {4,2,false,false,true}; return true;
    case GL_RGBA16UI: info = {4,2,false,false,false}; return true;
    case GL_R32I: info = {1,4,false,false,true}; return true;
    case GL_R32UI: info = {1,4,false,false,false}; return true;
    case GL_RG32I: info = {2,4,false,false,true}; return true;
    case GL_RG32UI: info = {2,4,false,false,false}; return true;
    case GL_RGB32I: info = {3,4,false,false,true}; return true;
    case GL_RGB32UI: info = {3,4,false,false,false}; return true;
    case GL_RGBA32I: info = {4,4,false,false,true}; return true;
    case GL_RGBA32UI: info = {4,4,false,false,false}; return true;
    default: return false;
    }
}

// Convert a IEEE-754 half (uint16) to a 32-bit float. Used when a Clear*Buffer*
// clear value is supplied as GL_HALF_FLOAT.
float halfToFloat(uint16_t h) {
    uint32_t sign = (h >> 15) & 0x1;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mant = h & 0x3FF;
    uint32_t f;
    if (exp == 0) {
        if (mant == 0) {
            f = sign << 31;
        } else {
            // Subnormal: renormalize.
            int e = 0;
            while ((mant & 0x400) == 0) { mant <<= 1; --e; }
            mant &= 0x3FF;
            exp = static_cast<uint32_t>(1 - e);
            f = (sign << 31) | (exp << 23) | (mant << 13);
        }
    } else if (exp == 0x1F) {
        f = (sign << 31) | 0x7F800000u | (mant << 13);
    } else {
        f = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
    }
    float out;
    std::memcpy(&out, &f, sizeof(out));
    return out;
}

// Build the per-element fill pattern (components * bytesPerComp bytes) for a
// Clear*Buffer* call. `data` is described by `format`/`type`; the destination
// layout is `info`. Returns false (and leaves `pattern` empty) when `format` or
// `type` is not a valid combination (caller reports GL_INVALID_VALUE).
bool buildClearPattern(uint32_t format, uint32_t type, const BufferFormatInfo& info,
                       const void* data, std::vector<uint8_t>& pattern) {
    int srcComps = 0;
    switch (format) {
    case GL_RED: srcComps = 1; break;
    case GL_RG: srcComps = 2; break;
    case GL_RGB: srcComps = 3; break;
    case GL_RGBA: srcComps = 4; break;
    case GL_RED_INTEGER: srcComps = 1; break;
    case GL_RG_INTEGER: srcComps = 2; break;
    case GL_RGB_INTEGER: srcComps = 3; break;
    case GL_RGBA_INTEGER: srcComps = 4; break;
    case GL_DEPTH_COMPONENT:
    case GL_STENCIL_INDEX: srcComps = 1; break;
    default: return false;
    }
    // The value is read as floating-point only for FLOAT / HALF_FLOAT source
    // types; every other (byte/short/int, signed or unsigned) source is an
    // integer type, regardless of whether the destination format is color or
    // integer (SPEC §6: ClearBuffer* converts the source to the destination).
    bool srcIsFloat = false;
    switch (type) {
    case GL_FLOAT: case GL_HALF_FLOAT: srcIsFloat = true; break;
    case GL_BYTE: case GL_UNSIGNED_BYTE: case GL_SHORT: case GL_UNSIGNED_SHORT:
    case GL_INT: case GL_UNSIGNED_INT: break;
    default: return false;
    }

    pattern.assign(static_cast<size_t>(info.components) * info.bytesPerComp, 0);

    double fsrc[4] = {0.0, 0.0, 0.0, 0.0};
    int64_t isrc[4] = {0, 0, 0, 0};
    if (data) {
        if (srcIsFloat) {
            float fv[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            switch (type) {
            case GL_FLOAT: { const auto* p = static_cast<const float*>(data);
                for (int i = 0; i < srcComps; ++i) fv[i] = p[i]; } break;
            case GL_HALF_FLOAT: { const auto* p = static_cast<const uint16_t*>(data);
                for (int i = 0; i < srcComps; ++i) fv[i] = halfToFloat(p[i]); } break;
            default: break;
            }
            for (int i = 0; i < srcComps; ++i) {
                fsrc[i] = fv[i];
                isrc[i] = static_cast<int64_t>(fv[i]);
            }
        } else {
            int64_t iv[4] = {0, 0, 0, 0};
            switch (type) {
            case GL_BYTE: { const auto* p = static_cast<const int8_t*>(data);
                for (int i = 0; i < srcComps; ++i) iv[i] = p[i]; } break;
            case GL_UNSIGNED_BYTE: { const auto* p = static_cast<const uint8_t*>(data);
                for (int i = 0; i < srcComps; ++i) iv[i] = p[i]; } break;
            case GL_SHORT: { const auto* p = static_cast<const int16_t*>(data);
                for (int i = 0; i < srcComps; ++i) iv[i] = p[i]; } break;
            case GL_UNSIGNED_SHORT: { const auto* p = static_cast<const uint16_t*>(data);
                for (int i = 0; i < srcComps; ++i) iv[i] = p[i]; } break;
            case GL_INT: { const auto* p = static_cast<const int32_t*>(data);
                for (int i = 0; i < srcComps; ++i) iv[i] = p[i]; } break;
            case GL_UNSIGNED_INT: { const auto* p = static_cast<const uint32_t*>(data);
                for (int i = 0; i < srcComps; ++i) iv[i] = static_cast<int64_t>(p[i]); } break;
            default: break;
            }
            for (int i = 0; i < srcComps; ++i) {
                isrc[i] = iv[i];
                fsrc[i] = static_cast<double>(iv[i]);
            }
        }
    }

    for (int c = 0; c < info.components; ++c) {
        double v = 0.0;
        int64_t iv = 0;
        if (c < srcComps) { v = fsrc[c]; iv = isrc[c]; }
        else { v = (c == 3) ? 1.0 : 0.0; iv = (c == 3) ? 1 : 0; }

        size_t off = static_cast<size_t>(c) * info.bytesPerComp;
        uint8_t* dst = pattern.data() + off;
        if (info.isFloat) {
            float f = static_cast<float>(v);
            std::memcpy(dst, &f, sizeof(float));
        } else if (info.normalized) {
            // For a floating-point source the value is in [0,1] (signed [-1,1])
            // and scales to the UNORM/SNORM range; for an integer source the
            // value is already expressed in that range and is stored directly
            // (SPEC §6: ClearBuffer* converts source -> destination).
            if (info.isSigned) {
                int64_t lo = (info.bytesPerComp == 1) ? -128 : -32768;
                int64_t hi = (info.bytesPerComp == 1) ? 127 : 32767;
                int64_t s = srcIsFloat ? static_cast<int64_t>(v * static_cast<double>(hi)) : iv;
                if (s < lo) s = lo;
                if (s > hi) s = hi;
                if (info.bytesPerComp == 1) { int8_t b = static_cast<int8_t>(s); std::memcpy(dst, &b, 1); }
                else { int16_t b = static_cast<int16_t>(s); std::memcpy(dst, &b, 2); }
            } else {
                int64_t lo = 0;
                int64_t hi = (info.bytesPerComp == 1) ? 255 : 65535;
                int64_t u = srcIsFloat ? static_cast<int64_t>(v * static_cast<double>(hi) + 0.5) : iv;
                if (u < lo) u = lo;
                if (u > hi) u = hi;
                if (info.bytesPerComp == 1) { uint8_t b = static_cast<uint8_t>(u); std::memcpy(dst, &b, 1); }
                else { uint16_t b = static_cast<uint16_t>(u); std::memcpy(dst, &b, 2); }
            }
        } else { // pure integer
            if (info.isSigned) {
                int64_t s = iv;
                if (info.bytesPerComp == 1) { int8_t b = static_cast<int8_t>(s); std::memcpy(dst, &b, 1); }
                else if (info.bytesPerComp == 2) { int16_t b = static_cast<int16_t>(s); std::memcpy(dst, &b, 2); }
                else { int32_t b = static_cast<int32_t>(s); std::memcpy(dst, &b, 4); }
            } else {
                uint64_t u = static_cast<uint64_t>(iv);
                if (info.bytesPerComp == 1) { uint8_t b = static_cast<uint8_t>(u); std::memcpy(dst, &b, 1); }
                else if (info.bytesPerComp == 2) { uint16_t b = static_cast<uint16_t>(u); std::memcpy(dst, &b, 2); }
                else { uint32_t b = static_cast<uint32_t>(u); std::memcpy(dst, &b, 4); }
            }
        }
    }
    return true;
}

} // namespace

void Context::getBufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                              void* data) {
    getNamedBufferSubData(boundBuffer(target), offset, size, data);
}

void Context::getNamedBufferSubData(GLObjectName buffer, intptr_t offset,
                                   intptr_t size, void* data) {
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation); // not an existing buffer object
        return;
    }
    if (offset < 0 || size < 0 || offset + size > obj->size) {
        setError(GLError::InvalidValue); // region out of bounds
        return;
    }
    if (obj->mapped && (obj->mapAccess & GL_MAP_PERSISTENT_BIT) == 0) {
        setError(GLError::InvalidOperation); // non-persistent map active
        return;
    }
    if (data && size > 0) {
        std::memcpy(data, obj->store.data() + static_cast<size_t>(offset),
                    static_cast<size_t>(size));
    }
}

void Context::clearBufferData(uint32_t target, uint32_t internalformat,
                             uint32_t format, uint32_t type, const void* data) {
    clearNamedBufferData(boundBuffer(target), internalformat, format, type, data);
}

void Context::clearNamedBufferData(GLObjectName buffer, uint32_t internalformat,
                                  uint32_t format, uint32_t type, const void* data) {
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation); // not an existing buffer object
        return;
    }
    BufferFormatInfo info;
    if (!lookupBufferFormat(internalformat, info)) {
        setError(GLError::InvalidEnum); // not a supported sized internal format
        return;
    }
    std::vector<uint8_t> pattern;
    if (!buildClearPattern(format, type, info, data, pattern)) {
        setError(GLError::InvalidValue); // bad format/type
        return;
    }
    if (obj->mapped && (obj->mapAccess & GL_MAP_PERSISTENT_BIT) == 0) {
        setError(GLError::InvalidOperation); // non-persistent map active
        return;
    }
    // Fill the whole store with the pattern.
    if (obj->size > 0) {
        size_t elemSize = static_cast<size_t>(info.components) * info.bytesPerComp;
        for (size_t pos = 0; pos + elemSize <= static_cast<size_t>(obj->size);
             pos += elemSize) {
            std::memcpy(obj->store.data() + pos, pattern.data(), elemSize);
        }
    }
    if (obj->backend) {
        uint32_t fwdTarget = obj->target ? obj->target : GL_ARRAY_BUFFER;
        obj->backend->bufferSubData(fwdTarget, 0, obj->size, obj->store.data());
    }
}

void Context::clearBufferSubData(uint32_t target, uint32_t internalformat,
                                intptr_t offset, intptr_t size, uint32_t format,
                                uint32_t type, const void* data) {
    clearNamedBufferSubData(boundBuffer(target), internalformat, offset, size,
                            format, type, data);
}

void Context::clearNamedBufferSubData(GLObjectName buffer, uint32_t internalformat,
                                     intptr_t offset, intptr_t size, uint32_t format,
                                     uint32_t type, const void* data) {
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation); // not an existing buffer object
        return;
    }
    BufferFormatInfo info;
    if (!lookupBufferFormat(internalformat, info)) {
        setError(GLError::InvalidEnum); // not a supported sized internal format
        return;
    }
    size_t elemSize = static_cast<size_t>(info.components) * info.bytesPerComp;
    if (offset < 0 || size < 0 || offset + size > obj->size) {
        setError(GLError::InvalidValue); // region out of bounds
        return;
    }
    if (offset % elemSize != 0 || size % elemSize != 0) {
        setError(GLError::InvalidValue); // not aligned to the element size
        return;
    }
    std::vector<uint8_t> pattern;
    if (!buildClearPattern(format, type, info, data, pattern)) {
        setError(GLError::InvalidValue); // bad format/type
        return;
    }
    if (obj->mapped && (obj->mapAccess & GL_MAP_PERSISTENT_BIT) == 0) {
        setError(GLError::InvalidOperation); // non-persistent map active
        return;
    }
    if (size > 0) {
        size_t start = static_cast<size_t>(offset);
        for (size_t pos = start; pos + elemSize <= start + static_cast<size_t>(size);
             pos += elemSize) {
            std::memcpy(obj->store.data() + pos, pattern.data(), elemSize);
        }
        if (obj->backend) {
            uint32_t fwdTarget = obj->target ? obj->target : GL_ARRAY_BUFFER;
            obj->backend->bufferSubData(fwdTarget, offset, size,
                                        obj->store.data() + start);
        }
    }
}

void Context::invalidateBufferData(GLObjectName buffer) {
    // SPEC §6.5: equivalent to invalidating [0, BUFFER_SIZE).
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidValue); // zero or not an existing buffer object
        return;
    }
    if (obj->mapped && (obj->mapAccess & GL_MAP_PERSISTENT_BIT) == 0) {
        setError(GLError::InvalidOperation); // non-persistent map active
        return;
    }
    if (obj->backend) obj->backend->invalidateBufferData(obj->target);
}

void Context::invalidateBufferSubData(GLObjectName buffer, intptr_t offset,
                                      intptr_t length) {
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidValue); // zero or not an existing buffer object
        return;
    }
    if (offset < 0 || length < 0 || offset + length > obj->size) {
        setError(GLError::InvalidValue); // region out of bounds
        return;
    }
    if (obj->mapped && (obj->mapAccess & GL_MAP_PERSISTENT_BIT) == 0) {
        setError(GLError::InvalidOperation); // non-persistent map active
        return;
    }
    if (obj->backend) obj->backend->invalidateBufferSubData(obj->target, offset, length);
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

// Targets that have an array of indexed binding points (SPEC §6.1.1). A target
// outside this set is GL_INVALID_ENUM; one inside it that the backend cannot
// support is reported as GL_INVALID_OPERATION (honest capability gap) via
// bufferTargetFeature above.
bool isIndexedBufferTarget(uint32_t target) {
    switch (target) {
    case GL_ATOMIC_COUNTER_BUFFER:
    case GL_TRANSFORM_FEEDBACK_BUFFER:
    case GL_UNIFORM_BUFFER:
    case GL_SHADER_STORAGE_BUFFER:
        return true;
    default:
        return false;
    }
}

// Number of indexed binding points the frontend tracks per target (SPEC §6.7.1
// requires at least 4; GL implementations expose far more). Used to validate
// `index` / `first + count`.
constexpr uint32_t kMaxIndexedBufferBindings = 16;

// Sampler-object scalar parameters (SPEC §8.2, table 23.23). These are the
// Per-type pname validation for sampler parameters (SPEC §8.2). Samplers accept
// the same pname set as textures but the valid accessor type differs.
bool isSamplerIntParam(uint32_t pname) {
    switch (pname) {
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_COMPARE_FUNC:
    case GL_TEXTURE_SWIZZLE_R:
    case GL_TEXTURE_SWIZZLE_G:
    case GL_TEXTURE_SWIZZLE_B:
    case GL_TEXTURE_SWIZZLE_A:
        return true;
    default:
        return false;
    }
}
bool isSamplerFloatParam(uint32_t pname) {
    switch (pname) {
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_LOD_BIAS:
        return true;
    default:
        return false;
    }
}
bool isSamplerFloatVecParam(uint32_t pname) {
    return pname == GL_TEXTURE_BORDER_COLOR; // vec4
}
} // namespace

GLError Context::checkIndexedBufferTarget(uint32_t target) const {
    // SPEC §6.1.1: a target without indexed binding points is GL_INVALID_ENUM;
    // a legal target the backend cannot provide is GL_INVALID_OPERATION.
    if (!isIndexedBufferTarget(target)) return GLError::InvalidEnum;
    const Feature f = bufferTargetFeature(target);
    if (f == Feature::FeatureCount ||
        !backend_.capabilities().isSupported(f)) {
        return GLError::InvalidOperation;
    }
    return GLError::NoError;
}

GLError Context::checkIndexedBufferBinding(uint32_t index, GLObjectName buffer,
                                           intptr_t offset, intptr_t size,
                                           bool range) const {
    // SPEC §6.1.1 per-binding validation, shared by the single and multi-bind
    // forms so both agree.
    if (index >= kMaxIndexedBufferBindings) return GLError::InvalidValue;
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end())
        return GLError::InvalidOperation; // ungenerated buffer name
    if (range) {
        if (offset < 0) return GLError::InvalidValue;
        if (buffer != 0 && size <= 0) return GLError::InvalidValue;
    }
    return GLError::NoError;
}

void Context::applyIndexedBufferBinding(uint32_t target, uint32_t index,
                                        GLObjectName buffer, intptr_t offset,
                                        intptr_t size, bool range) {
    if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        if (TransformFeedbackObject::TfBufferBinding* slot =
                activeTransformFeedbackBinding(index)) {
            slot->buffer = buffer;
            slot->offset = range ? offset : 0;
            slot->size = range ? size : 0;
        }
    }
    if (GLStateSink* sink = backend_.stateSink()) {
        if (range) {
            sink->bindBufferRange(target, index, buffer, offset, size);
        } else {
            sink->bindBufferBase(target, index, buffer);
        }
    }
}

void Context::bindBufferBase(uint32_t target, uint32_t index,
                             GLObjectName buffer) {
    GLError err = checkIndexedBufferTarget(target);
    if (err == GLError::NoError)
        err = checkIndexedBufferBinding(index, buffer, 0, 0, false);
    if (err != GLError::NoError) {
        setError(err);
        return;
    }
    applyIndexedBufferBinding(target, index, buffer, 0, 0, false);
}

void Context::bindBufferRange(uint32_t target, uint32_t index,
                              GLObjectName buffer, intptr_t offset,
                              intptr_t size) {
    GLError err = checkIndexedBufferTarget(target);
    if (err == GLError::NoError)
        err = checkIndexedBufferBinding(index, buffer, offset, size, true);
    if (err != GLError::NoError) {
        setError(err);
        return;
    }
    applyIndexedBufferBinding(target, index, buffer, offset, size, true);
}

void Context::bindBuffersImpl(uint32_t target, uint32_t first, GLsizei count,
                              const GLObjectName* buffers,
                              const intptr_t* offsets, const intptr_t* sizes,
                              bool range) {
    GLError err = checkIndexedBufferTarget(target);
    if (err != GLError::NoError) {
        setError(err);
        return;
    }
    if (count < 0) {
        setError(GLError::InvalidValue); // SPEC §6.1.1: count negative
        return;
    }
    const uint32_t n = static_cast<uint32_t>(count);
    if (first > kMaxIndexedBufferBindings ||
        n > kMaxIndexedBufferBindings - first) {
        // SPEC §6.1.1: first + count past the indexed binding point count is
        // GL_INVALID_OPERATION.
        setError(GLError::InvalidOperation);
        return;
    }
    // A null `buffers` array resets the whole range to the unbound state,
    // ignoring offsets/sizes (SPEC §6.1.1).
    if (buffers == nullptr) {
        for (uint32_t i = 0; i < n; ++i)
            applyIndexedBufferBinding(target, first + i, 0, 0, 0, range);
        return;
    }
    if (range && (offsets == nullptr || sizes == nullptr)) {
        setError(GLError::InvalidValue);
        return;
    }
    // Values are checked separately per binding point: an invalid entry leaves
    // that binding point unchanged and reports an error, while the valid entries
    // are still bound (SPEC §6.1.1).
    GLError firstError = GLError::NoError;
    for (uint32_t i = 0; i < n; ++i) {
        const intptr_t offset = range ? offsets[i] : 0;
        const intptr_t size = range ? sizes[i] : 0;
        const GLError e = checkIndexedBufferBinding(first + i, buffers[i],
                                                    offset, size, range);
        if (e != GLError::NoError) {
            if (firstError == GLError::NoError) firstError = e;
            continue;
        }
        applyIndexedBufferBinding(target, first + i, buffers[i], offset, size,
                                  range);
    }
    if (firstError != GLError::NoError) setError(firstError);
}

void Context::bindBuffersBase(uint32_t target, uint32_t first, GLsizei count,
                              const GLObjectName* buffers) {
    bindBuffersImpl(target, first, count, buffers, nullptr, nullptr, false);
}

void Context::bindBuffersRange(uint32_t target, uint32_t first, GLsizei count,
                               const GLObjectName* buffers,
                               const intptr_t* offsets, const intptr_t* sizes) {
    bindBuffersImpl(target, first, count, buffers, offsets, sizes, true);
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

void Context::bindTextures(uint32_t first, GLsizei count,
                           const GLObjectName* textures) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (count < 0) {
        setError(GLError::InvalidValue); // SPEC §8.1: count negative
        return;
    }
    const uint32_t n = static_cast<uint32_t>(count);
    const uint32_t maxUnits = state_.maxCombinedTextureUnits();
    if (first > maxUnits || n > maxUnits - first) {
        // SPEC §8.1: first + count beyond the texture image unit count is
        // GL_INVALID_OPERATION (not GL_INVALID_VALUE).
        setError(GLError::InvalidOperation);
        return;
    }
    // SPEC §8.1: entries are validated separately per texture image unit. An
    // invalid entry leaves that unit unchanged and generates
    // GL_INVALID_OPERATION; the valid entries are still bound. Each texture is
    // bound to the target it was created with; a zero entry (or a null array)
    // resets every target of that unit to its default texture.
    bool sawInvalid = false;
    for (uint32_t i = 0; i < n; ++i) {
        const GLObjectName name = (textures != nullptr) ? textures[i] : 0;
        const uint32_t unit = first + i;
        if (name == 0) {
            state_.setTextureUnitBinding(unit, GL_TEXTURE_2D, 0);
            continue;
        }
        TextureObject* tex = getTexture(name);
        if (tex == nullptr) {
            sawInvalid = true; // ungenerated name: this unit stays unchanged
            continue;
        }
        state_.setTextureUnitBinding(unit, tex->target, name);
    }
    if (sawInvalid) setError(GLError::InvalidOperation);
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

bool Context::isTexture(GLObjectName name) const {
    return textures_.find(name) != textures_.end();
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
    tex->target = normalizeTextureTarget(target);
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
    updateMutableTextureStorage(tex);
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
    tex->target = normalizeTextureTarget(target);
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
    updateMutableTextureStorage(tex);
    if (tex->backend) {
        tex->backend->texImage1D(target, level, internalFormat, width, format, type,
                                 data);
    }
}

void Context::updateMutableTextureStorage(TextureObject* tex) {
    if (tex == nullptr || tex->images.empty()) return;
    int maxLevel = 0;
    for (const auto& e : tex->images) maxLevel = std::max(maxLevel, e.level);
    tex->storageLevels = maxLevel + 1;
    for (const auto& e : tex->images) {
        if (e.level == 0) {
            tex->storageBaseWidth = e.width;
            tex->storageBaseHeight = e.height;
            tex->storageBaseDepth = e.depth;
            tex->storageInternalFormat = e.internalFormat;
            break;
        }
    }
    tex->storageSet = true;
    tex->immutableStorage = false;
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
    tex->target = normalizeTextureTarget(target);
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
    updateMutableTextureStorage(tex);
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

    void Context::texParameterIiv(uint32_t target, uint32_t pname, const int32_t* params) {
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        int count = texParamElementCount(pname);
        tex->target = target;
        tex->paramsIiv[pname].assign(params, params + count);
        if (tex->backend) tex->backend->texParameterIiv(target, pname, params, count);
    }

    void Context::texParameterIuiv(uint32_t target, uint32_t pname, const uint32_t* params) {
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        int count = texParamElementCount(pname);
        tex->target = target;
        tex->paramsIuiv[pname].assign(params, params + count);
        if (tex->backend) tex->backend->texParameterIuiv(target, pname, params, count);
    }

    void Context::getTexParameterIiv(GLenum target, GLenum pname, int32_t* params) {
        if (params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation); // no texture bound
            return;
        }
        auto it = tex->paramsIiv.find(pname);
        if (it != tex->paramsIiv.end() && !it->second.empty()) {
            std::copy(it->second.begin(), it->second.end(), params);
        } else {
            *params = 0; // GL default for an unset parameter
        }
    }

    void Context::getTexParameterIuiv(GLenum target, GLenum pname, uint32_t* params) {
        if (params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation); // no texture bound
            return;
        }
        auto it = tex->paramsIuiv.find(pname);
        if (it != tex->paramsIuiv.end() && !it->second.empty()) {
            std::copy(it->second.begin(), it->second.end(), params);
        } else {
            *params = 0u; // GL default for an unset parameter
        }
    }

    void Context::generateMipmap(uint32_t target) {
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation); // no texture bound
            return;
        }
        tex->target = target;
        if (tex->backend) tex->backend->generateMipmap(target);
    }

    void Context::invalidateTexImage(uint32_t target, int level) {
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation); // no texture bound
            return;
        }
        if (level < 0) {
            setError(GLError::InvalidValue);
            return;
        }
        tex->target = target;
        if (tex->backend) tex->backend->invalidateTexImage(target, level);
    }

    void Context::invalidateTexSubImage(uint32_t target, int level, int xoffset,
                                      int yoffset, int zoffset, int width, int height,
                                      int depth) {
        TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
        if (tex == nullptr) {
            setError(GLError::InvalidOperation); // no texture bound
            return;
        }
        if (level < 0 || xoffset < 0 || yoffset < 0 || zoffset < 0 || width < 0 ||
            height < 0 || depth < 0) {
            setError(GLError::InvalidValue);
            return;
        }
        tex->target = target;
        if (tex->backend)
            tex->backend->invalidateTexSubImage(target, level, xoffset, yoffset, zoffset,
                                              width, height, depth);
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

void Context::textureView(GLObjectName texture, uint32_t target,
                          GLObjectName origtexture, uint32_t internalFormat,
                          uint32_t minLevel, uint32_t numLevels, uint32_t minLayer,
                          uint32_t numLayers) {
    if (!backend_.capabilities().isSupported(Feature::TextureViews)) {
        setError(GLError::InvalidOperation);
        return;
    }
    TextureObject* src = getTexture(origtexture);
    TextureObject* view = getTexture(texture);
    if (src == nullptr || view == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated / default name
        return;
    }
    if (texture == origtexture) {
        setError(GLError::InvalidOperation); // a texture may not view itself
        return;
    }
    if (!src->immutableStorage) {
        setError(GLError::InvalidOperation); // source lacks immutable storage
        return;
    }
    if (!isValidTextureTarget(target)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (internalFormat == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (numLevels == 0 ||
        static_cast<uint64_t>(minLevel) + numLevels >
            static_cast<uint64_t>(src->storageLevels)) {
        setError(GLError::InvalidValue); // empty or out-of-range level range
        return;
    }
    // Level 0 of the view maps to source level minLevel; derive exposed storage.
    int shift = static_cast<int>(minLevel);
    view->target = target;
    view->isView = true;
    view->viewSource = origtexture;
    view->viewInternalFormat = internalFormat;
    view->viewMinLevel = minLevel;
    view->viewNumLevels = numLevels;
    view->viewMinLayer = minLayer;
    view->viewNumLayers = numLayers;
    view->immutableStorage = true;
    view->storageSet = true;
    view->storageLevels = static_cast<int>(numLevels);
    view->storageInternalFormat = internalFormat;
    int w = src->storageBaseWidth >> shift; view->storageBaseWidth = w < 1 ? 1 : w;
    int h = src->storageBaseHeight >> shift; view->storageBaseHeight = h < 1 ? 1 : h;
    int d = src->storageBaseDepth >> shift; view->storageBaseDepth = d < 1 ? 1 : d;
    if (view->backend) {
        uint32_t origNative = src->backend ? src->backend->nativeId() : 0;
        view->backend->view(target, origNative, internalFormat, minLevel, numLevels,
                            minLayer, numLayers);
    }
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

    void Context::textureParameterIiv(GLObjectName texture, uint32_t pname,
                                    const int32_t* params) {
        TextureObject* tex = dsaTexture(*this, texture);
        if (tex == nullptr) return;
        if (params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        int count = texParamElementCount(pname);
        tex->paramsIiv[pname].assign(params, params + count);
        if (tex->backend) tex->backend->texParameterIiv(tex->target, pname, params, count);
    }

    void Context::textureParameterIuiv(GLObjectName texture, uint32_t pname,
                                     const uint32_t* params) {
        TextureObject* tex = dsaTexture(*this, texture);
        if (tex == nullptr) return;
        if (params == nullptr) {
            setError(GLError::InvalidValue);
            return;
        }
        int count = texParamElementCount(pname);
        tex->paramsIuiv[pname].assign(params, params + count);
        if (tex->backend) tex->backend->texParameterIuiv(tex->target, pname, params, count);
    }

    void Context::getTextureParameterIiv(GLObjectName texture, GLenum pname,
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
        auto it = tex->paramsIiv.find(pname);
        if (it != tex->paramsIiv.end() && !it->second.empty()) {
            std::copy(it->second.begin(), it->second.end(), params);
        } else {
            *params = 0; // GL default for an unset parameter
        }
    }

    void Context::getTextureParameterIuiv(GLObjectName texture, GLenum pname,
                                       uint32_t* params) {
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
            *params = 0u; // default (name 0) texture object
            return;
        }
        auto it = tex->paramsIuiv.find(pname);
        if (it != tex->paramsIuiv.end() && !it->second.empty()) {
            std::copy(it->second.begin(), it->second.end(), params);
        } else {
            *params = 0u; // GL default for an unset parameter
        }
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

void Context::getTexLevelParameteriv(uint32_t target, int level, GLenum pname,
                                      int32_t* params) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    getTexLevelParameterivImpl(tex, level, pname, params);
}

void Context::getTexLevelParameterfv(uint32_t target, int level, GLenum pname,
                                     float* params) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    getTexLevelParameterfvImpl(tex, level, pname, params);
}

void Context::getTextureLevelParameteriv(GLObjectName texture, int level,
                                         GLenum pname, int32_t* params) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    getTexLevelParameterivImpl(tex, level, pname, params);
}

void Context::getTextureLevelParameterfv(GLObjectName texture, int level,
                                         GLenum pname, float* params) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    getTexLevelParameterfvImpl(tex, level, pname, params);
}

void Context::getTexLevelParameterivImpl(TextureObject* tex, int level,
                                         GLenum pname, int32_t* params) {
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

void Context::getTexLevelParameterfvImpl(TextureObject* tex, int level,
                                         GLenum pname, float* params) {
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

void Context::getCompressedTextureImage(GLObjectName texture, int level,
                                        void* pixels) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getCompressedTexImage(tex->target, level, pixels);
}

void Context::getCompressedTexImage(uint32_t target, int level, void* pixels) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getCompressedTexImage(target, level, pixels);
}

void Context::getTextureImage(GLObjectName texture, int level, uint32_t format,
                              uint32_t type, int bufSize, void* pixels) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getTexImage(tex->target, level, format, type,
                                                 bufSize, pixels);
}

void Context::getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                          int bufSize, void* pixels) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getTexImage(target, level, format, type, bufSize,
                                                pixels);
}

void Context::getCompressedTextureImage(GLObjectName texture, int level, int bufSize,
                                        void* pixels) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getCompressedTexImage(tex->target, level, bufSize,
                                                          pixels);
}

void Context::getCompressedTexImage(uint32_t target, int level, int bufSize,
                                     void* pixels) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (level < 0 || bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getCompressedTexImage(target, level, bufSize, pixels);
}

void Context::getTextureSubImage(GLObjectName texture, int level, int xoffset,
                                 int yoffset, int zoffset, int width, int height,
                                 int depth, uint32_t format, uint32_t type,
                                 int bufSize, void* pixels) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || width < 0 || height < 0 || depth < 0 || bufSize < 0 ||
        xoffset < 0 || yoffset < 0 || zoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getTextureSubImage(tex->target, level, xoffset,
                                                       yoffset, zoffset, width, height,
                                                       depth, format, type, bufSize,
                                                       pixels);
}

void Context::getCompressedTextureSubImage(GLObjectName texture, int level, int xoffset,
                                           int yoffset, int zoffset, int width,
                                           int height, int depth, int bufSize,
                                           void* pixels) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (level < 0 || width < 0 || height < 0 || depth < 0 || bufSize < 0 ||
        xoffset < 0 || yoffset < 0 || zoffset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (tex->backend) tex->backend->getCompressedTextureSubImage(tex->target, level,
                                                                xoffset, yoffset,
                                                                zoffset, width, height,
                                                                depth, bufSize, pixels);
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

void Context::texStorage1D(uint32_t target, int levels, uint32_t internalFormat,
                           int width) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (levels < 1 || width < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = levels;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = 1;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend) tex->backend->storage1D(target, levels, internalFormat, width);
}

void Context::texStorage2D(uint32_t target, int levels, uint32_t internalFormat,
                           int width, int height) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (levels < 1 || width < 1 || height < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = levels;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage2D(target, levels, internalFormat, width, height);
}

void Context::texStorage3D(uint32_t target, int levels, uint32_t internalFormat,
                           int width, int height, int depth) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (levels < 1 || width < 1 || height < 1 || depth < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = levels;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = depth;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage3D(target, levels, internalFormat, width, height, depth);
}

void Context::texBuffer(uint32_t target, uint32_t internalFormat,
                       GLObjectName buffer) {
    if (target != GL_TEXTURE_BUFFER) {
        setError(GLError::InvalidEnum);
        return;
    }
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation); // ungenerated buffer name
        return;
    }
    uint32_t nativeBuffer = 0;
    if (buffer != 0) {
        if (auto* b = getBuffer(buffer))
            nativeBuffer = b->backend ? b->backend->nativeId() : 0;
    }
    tex->target = target;
    if (tex->backend)
        tex->backend->textureBuffer(target, internalFormat, nativeBuffer);
}

void Context::texBufferRange(uint32_t target, uint32_t internalFormat,
                             GLObjectName buffer, intptr_t offset, intptr_t size) {
    if (target != GL_TEXTURE_BUFFER) {
        setError(GLError::InvalidEnum);
        return;
    }
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
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
    tex->target = target;
    if (tex->backend)
        tex->backend->textureBufferRange(target, internalFormat, nativeBuffer, offset,
                                         size);
}

void Context::texStorage2DMultisample(uint32_t target, int samples,
                                      uint32_t internalFormat, int width, int height,
                                      bool fixedSampleLocations) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (target != GL_TEXTURE_2D_MULTISAMPLE) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (samples < 0 || width < 1 || height < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = 1;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage2DMultisample(target, samples, internalFormat, width,
                                           height, fixedSampleLocations);
}

void Context::texStorage3DMultisample(uint32_t target, int samples,
                                      uint32_t internalFormat, int width, int height,
                                      int depth, bool fixedSampleLocations) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (samples < 0 || width < 1 || height < 1 || depth < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = 1;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = depth;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage3DMultisample(target, samples, internalFormat, width,
                                           height, depth, fixedSampleLocations);
}

void Context::texImage2DMultisample(uint32_t target, int samples,
                                   uint32_t internalFormat, int width, int height,
                                   bool fixedSampleLocations) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (target != GL_TEXTURE_2D_MULTISAMPLE) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (samples < 0 || width < 1 || height < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = 1;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = false;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->texImage2DMultisample(target, samples, internalFormat, width,
                                            height, fixedSampleLocations);
}

void Context::texImage3DMultisample(uint32_t target, int samples,
                                   uint32_t internalFormat, int width, int height,
                                   int depth, bool fixedSampleLocations) {
    TextureObject* tex = getTexture(state_.boundTextureForTarget(target));
    if (tex == nullptr) {
        setError(GLError::InvalidOperation); // no texture bound
        return;
    }
    if (target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (samples < 0 || width < 1 || height < 1 || depth < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = target;
    tex->storageLevels = 1;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = depth;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = false;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->texImage3DMultisample(target, samples, internalFormat, width,
                                            height, depth, fixedSampleLocations);
}

void Context::textureStorage2DMultisample(GLObjectName texture, int samples,
                                          uint32_t internalFormat, int width,
                                          int height, bool fixedSampleLocations) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (samples < 0 || width < 1 || height < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = GL_TEXTURE_2D_MULTISAMPLE;
    tex->storageLevels = 1;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = 1;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples,
                                           internalFormat, width, height,
                                           fixedSampleLocations);
}

void Context::textureStorage3DMultisample(GLObjectName texture, int samples,
                                          uint32_t internalFormat, int width,
                                          int height, int depth,
                                          bool fixedSampleLocations) {
    TextureObject* tex = dsaTexture(*this, texture);
    if (tex == nullptr) return;
    if (samples < 0 || width < 1 || height < 1 || depth < 1) {
        setError(GLError::InvalidValue);
        return;
    }
    tex->target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    tex->storageLevels = 1;
    tex->storageBaseWidth = width;
    tex->storageBaseHeight = height;
    tex->storageBaseDepth = depth;
    tex->storageInternalFormat = internalFormat;
    tex->immutableStorage = true;
    tex->storageSet = true;
    if (tex->backend)
        tex->backend->storage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples,
                                           internalFormat, width, height, depth,
                                           fixedSampleLocations);
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

bool Context::isRenderbuffer(GLObjectName name) const {
    return renderbuffers_.find(name) != renderbuffers_.end();
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

bool Context::isFramebuffer(GLObjectName name) const {
    return framebuffers_.find(name) != framebuffers_.end();
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

void Context::getRenderbufferParameteriv(uint32_t target, uint32_t pname,
                                         int32_t* params) {
    if (target != GL_RENDERBUFFER) {
        setError(GLError::InvalidEnum);
        return;
    }
    RenderbufferObject* rbo = getRenderbuffer(boundRenderbuffer());
    if (rbo == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    getRenderbufferParameterivImpl(rbo, pname, params);
}

void Context::getNamedRenderbufferParameteriv(GLObjectName renderbuffer,
                                             uint32_t pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    RenderbufferObject* rbo = dsaRenderbuffer(*this, renderbuffer);
    if (rbo == nullptr) return;
    getRenderbufferParameterivImpl(rbo, pname, params);
}

void Context::getRenderbufferParameterivImpl(RenderbufferObject* rbo,
                                            uint32_t pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
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

void Context::getFramebufferParameteriv(uint32_t target, uint32_t pname,
                                        int32_t* params) {
    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    FramebufferObject* fbo = getFramebuffer(boundFramebuffer());
    if (fbo == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    // FRAMEBUFFER_DEFAULT_* describe the default framebuffer; a user FBO has no
    // default dimensions, so the GL default is 0 (frontend-owned, SPEC §10).
    *params = 0;
}

void Context::getFramebufferAttachmentParameteriv(uint32_t target,
                                                  uint32_t attachment,
                                                  uint32_t pname,
                                                  int32_t* params) {
    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        setError(GLError::InvalidEnum);
        return;
    }
    FramebufferObject* fbo = getFramebuffer(boundFramebuffer());
    if (fbo == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    getFramebufferAttachmentParameterivImpl(fbo, attachment, pname, params);
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
    getFramebufferAttachmentParameterivImpl(fbo, attachment, pname, params);
}

void Context::getFramebufferAttachmentParameterivImpl(FramebufferObject* fbo,
                                                      uint32_t attachment,
                                                      uint32_t pname,
                                                      int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
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
        mask = GL_STENCIL_BUFFER_BIT;
        if (GLStateSink* sink = backend_.stateSink())
            sink->clearStencil(value[0]);
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
    if (GLStateSink* sink = backend_.stateSink()) {
        sink->clearDepth(static_cast<double>(depth));
        sink->clearStencil(stencil);
    }
    clearNamedFramebufferImpl(*this, framebuffer,
                              GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Context::clearBufferiv(uint32_t buffer, int drawbuffer, const int32_t* value) {
    if (value == nullptr || drawbuffer < 0) {
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
    } else if (buffer == GL_STENCIL) {
        mask = GL_STENCIL_BUFFER_BIT;
        if (GLStateSink* sink = backend_.stateSink())
            sink->clearStencil(value[0]);
    } else {
        setError(GLError::InvalidEnum);
        return;
    }
    clearNamedFramebufferImpl(*this, boundFramebuffer(), mask);
}

void Context::clearBufferuiv(uint32_t buffer, int drawbuffer, const uint32_t* value) {
    if (value == nullptr || drawbuffer < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (buffer != GL_COLOR) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (GLStateSink* sink = backend_.stateSink())
        sink->clearColor(static_cast<float>(value[0]),
                         static_cast<float>(value[1]),
                         static_cast<float>(value[2]),
                         static_cast<float>(value[3]));
    clearNamedFramebufferImpl(*this, boundFramebuffer(), GL_COLOR_BUFFER_BIT);
}

void Context::clearBufferfv(uint32_t buffer, int drawbuffer, const float* value) {
    if (value == nullptr || drawbuffer < 0) {
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
    } else {
        setError(GLError::InvalidEnum);
        return;
    }
    clearNamedFramebufferImpl(*this, boundFramebuffer(), mask);
}

void Context::clearBufferfi(uint32_t buffer, int drawbuffer, float depth, int stencil) {
    if (drawbuffer < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (buffer != GL_DEPTH) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (GLStateSink* sink = backend_.stateSink()) {
        sink->clearDepth(static_cast<double>(depth));
        sink->clearStencil(stencil);
    }
    clearNamedFramebufferImpl(*this, boundFramebuffer(),
                              GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Context::clipControl(uint32_t origin, uint32_t depth) {
    if (origin != GL_LOWER_LEFT && origin != GL_UPPER_LEFT) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (depth != GL_NEGATIVE_ONE_TO_ONE && depth != GL_ZERO_TO_ONE) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setClipControl(static_cast<GLenum>(origin), static_cast<GLenum>(depth));
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

bool Context::isVertexArray(GLObjectName name) const {
    if (name == 0) return false; // 0 is the default VAO, never a queried object
    return vertexArrays_.find(name) != vertexArrays_.end();
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

void Context::createTransformFeedbacks(uint32_t n, GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
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

bool Context::isTransformFeedback(GLObjectName name) const {
    return transformFeedbacks_.find(name) != transformFeedbacks_.end();
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

TransformFeedbackObject::TfBufferBinding* Context::tfBufferBindingSlot(
        GLObjectName xfb, uint32_t index) {
    if (index >= kMaxTransformFeedbackBuffers) {
        setError(GLError::InvalidValue);
        return nullptr;
    }
    if (xfb == 0) return &defaultTransformFeedbackBuffers_[index];
    TransformFeedbackObject* tf = getTransformFeedback(xfb);
    if (tf == nullptr) {
        // An ungenerated TF object name cannot carry bindings (SPEC §13.2.1:
        // the object must already exist).
        setError(GLError::InvalidOperation);
        return nullptr;
    }
    return &tf->bufferBindings[index];
}

TransformFeedbackObject::TfBufferBinding* Context::activeTransformFeedbackBinding(
        uint32_t index) {
    if (index >= kMaxTransformFeedbackBuffers) {
        setError(GLError::InvalidValue);
        return nullptr;
    }
    if (boundTransformFeedback_ == 0) return &defaultTransformFeedbackBuffers_[index];
    return tfBufferBindingSlot(boundTransformFeedback_, index);
}

void Context::transformFeedbackBufferBase(GLObjectName xfb, uint32_t index,
                                         GLObjectName buffer) {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    TransformFeedbackObject::TfBufferBinding* slot = tfBufferBindingSlot(xfb, index);
    if (slot == nullptr) return;
    slot->buffer = buffer;
    slot->offset = 0;
    slot->size = 0;
    if (GLStateSink* sink = backend_.stateSink()) {
        sink->bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer);
    }
}

void Context::transformFeedbackBufferRange(GLObjectName xfb, uint32_t index,
                                          GLObjectName buffer, intptr_t offset,
                                          intptr_t size) {
    if (!backend_.capabilities().isSupported(Feature::TransformFeedback)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation);
        return;
    }
    TransformFeedbackObject::TfBufferBinding* slot = tfBufferBindingSlot(xfb, index);
    if (slot == nullptr) return;
    slot->buffer = buffer;
    slot->offset = offset;
    slot->size = size;
    if (GLStateSink* sink = backend_.stateSink()) {
        sink->bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer,
                             offset, size);
    }
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

void Context::createQueries(uint32_t target, uint32_t n, GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    switch (target) {
        case GL_SAMPLES_PASSED:
        case GL_ANY_SAMPLES_PASSED:
        case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        case GL_TIME_ELAPSED:
        case GL_PRIMITIVES_GENERATED:
        case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
            break;
        default:
            setError(GLError::InvalidEnum);
            return;
    }
    for (uint32_t i = 0; i < n; ++i) {
        GLObjectName name = genQuery();
        if (name != 0 && queries_.count(name)) {
            queries_[name]->target = target;
        }
        names[i] = name;
    }
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

void Context::getQueryBufferObjectiv(GLObjectName id, GLObjectName buffer,
                                     uint32_t pname, intptr_t offset) {
    getQueryBufferObjectImpl(id, buffer, pname, offset, /*is64=*/false,
                             /*isSigned=*/true);
}

void Context::getQueryBufferObjectuiv(GLObjectName id, GLObjectName buffer,
                                      uint32_t pname, intptr_t offset) {
    getQueryBufferObjectImpl(id, buffer, pname, offset, /*is64=*/false,
                             /*isSigned=*/false);
}

void Context::getQueryBufferObjecti64v(GLObjectName id, GLObjectName buffer,
                                       uint32_t pname, intptr_t offset) {
    getQueryBufferObjectImpl(id, buffer, pname, offset, /*is64=*/true,
                             /*isSigned=*/true);
}

void Context::getQueryBufferObjectui64v(GLObjectName id, GLObjectName buffer,
                                        uint32_t pname, intptr_t offset) {
    getQueryBufferObjectImpl(id, buffer, pname, offset, /*is64=*/true,
                             /*isSigned=*/false);
}

// Shared body for glGetQueryBufferObject* (SPEC §4 / §19 / ARB_query_buffer_object):
// writes the cached query result/availability into the buffer's CPU mirror at
// `offset`, then uploads to the backend (emulating the driver write on backends
// without a native entry point).
void Context::getQueryBufferObjectImpl(GLObjectName id, GLObjectName buffer,
                                      uint32_t pname, intptr_t offset, bool is64,
                                      bool isSigned) {
    if (!backend_.capabilities().isSupported(Feature::Queries)) {
        setError(GLError::InvalidOperation);
        return;
    }
    BufferObject* buf = getBuffer(buffer);
    if (!buf) {
        setError(GLError::InvalidOperation); // ungenerated buffer object
        return;
    }
    QueryObject* q = getQuery(id);
    if (!q) {
        setError(GLError::InvalidOperation); // ungenerated query id
        return;
    }
    bool availOnly = false;
    switch (pname) {
        case GL_QUERY_RESULT: availOnly = false; break;
        case GL_QUERY_RESULT_AVAILABLE: availOnly = true; break;
        case GL_QUERY_RESULT_NO_WAIT: availOnly = false; break;
        default:
            setError(GLError::InvalidEnum); // unknown pname
            return;
    }
    const size_t typeSize = is64 ? 8u : 4u;
    if (offset < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (offset % static_cast<intptr_t>(typeSize) != 0) {
        setError(GLError::InvalidValue); // misaligned write
        return;
    }
    if (offset + static_cast<intptr_t>(typeSize) > buf->size) {
        setError(GLError::InvalidValue); // write extends past the buffer
        return;
    }
    int64_t value = 0;
    bool available = false;
    if (q->backend) q->backend->queryResult(&value, &available);
    uint8_t raw[8] = {0};
    if (availOnly) {
        const int64_t v = available ? GL_TRUE : GL_FALSE;
        if (is64) std::memcpy(raw, &v, 8);
        else { const int32_t v32 = static_cast<int32_t>(v); std::memcpy(raw, &v32, 4); }
    } else {
        // QUERY_RESULT / QUERY_RESULT_NO_WAIT: when the result is not yet available
        // the driver value is undefined; we write 0 to keep the CPU mirror defined.
        const int64_t v = available ? value : 0;
        if (is64) {
            std::memcpy(raw, &v, 8);
        } else if (isSigned) {
            const int32_t v32 = static_cast<int32_t>(value);
            std::memcpy(raw, &v32, 4);
        } else {
            const uint32_t v32 = static_cast<uint32_t>(value);
            std::memcpy(raw, &v32, 4);
        }
    }
    std::memcpy(buf->store.data() + static_cast<size_t>(offset), raw, typeSize);
    if (buf->backend) {
        // Use a dedicated copy target so the upload does not disturb application
        // binding points (the driver has no native glGetQueryBufferObject* in ES).
        buf->backend->bufferSubData(GL_COPY_WRITE_BUFFER, offset,
                                    static_cast<intptr_t>(typeSize), raw);
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

void Context::createSamplers(uint32_t n, GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
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

void Context::bindSamplers(uint32_t first, GLsizei count,
                           const GLObjectName* samplers) {
    if (!backend_.capabilities().isSupported(Feature::SamplerObjects)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (count < 0) {
        setError(GLError::InvalidValue); // SPEC §8.2: count negative
        return;
    }
    const uint32_t n = static_cast<uint32_t>(count);
    const uint32_t maxUnits = state_.maxCombinedTextureUnits();
    if (first > maxUnits || n > maxUnits - first) {
        // SPEC §8.2: first + count greater than the number of texture image
        // units is GL_INVALID_OPERATION (not GL_INVALID_VALUE).
        setError(GLError::InvalidOperation);
        return;
    }
    if (n == 0) return;
    // SPEC §8.2: each entry is validated separately. An invalid entry leaves
    // that unit's binding unchanged and generates GL_INVALID_OPERATION, while
    // valid entries in the same call are still applied.
    std::vector<GLObjectName> resolved(n);
    bool sawInvalid = false;
    for (uint32_t i = 0; i < n; ++i) {
        const GLObjectName name = (samplers != nullptr) ? samplers[i] : 0;
        if (name != 0 && samplers_.find(name) == samplers_.end()) {
            resolved[i] = state_.boundSamplerForUnit(first + i); // unchanged
            sawInvalid = true;
        } else {
            resolved[i] = name;
        }
    }
    state_.setSamplerBindings(first, n, resolved.data());
    if (sawInvalid) setError(GLError::InvalidOperation);
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
    if (!isSamplerIntParam(pname)) {
        setError(GLError::InvalidEnum); // non-int / unknown pname
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
    if (!isSamplerIntParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    auto it = s->params.find(pname);
    *params = (it != s->params.end()) ? it->second : 0;
}

void Context::samplerParameterf(GLObjectName sampler, uint32_t pname, float param) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isSamplerFloatParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    s->paramsf[pname] = param;
    if (s->backend) s->backend->samplerParameterf(pname, param);
}

void Context::samplerParameterfv(GLObjectName sampler, uint32_t pname,
                                 const float* params, int count) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isSamplerFloatVecParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr || count <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    s->paramsfv[pname].assign(params, params + count);
    if (s->backend) s->backend->samplerParameterfv(pname, params, count);
}

void Context::samplerParameterIiv(GLObjectName sampler, uint32_t pname,
                                  const int32_t* params) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isSamplerIntParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    s->params[pname] = params[0];
    if (s->backend) s->backend->samplerParameterIiv(pname, params);
}

void Context::samplerParameterIuiv(GLObjectName sampler, uint32_t pname,
                                   const uint32_t* params) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isSamplerIntParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    s->params[pname] = static_cast<int>(params[0]);
    if (s->backend) s->backend->samplerParameterIuiv(pname, params);
}

void Context::getSamplerParameterfv(GLObjectName sampler, uint32_t pname,
                                    float* params) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isSamplerFloatParam(pname) && !isSamplerFloatVecParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    auto fi = s->paramsf.find(pname);
    if (fi != s->paramsf.end()) {
        *params = fi->second;
        return;
    }
    auto fv = s->paramsfv.find(pname);
    if (fv != s->paramsfv.end() && !fv->second.empty()) {
        *params = fv->second[0];
        return;
    }
    auto ii = s->params.find(pname);
    if (ii != s->params.end()) {
        *params = static_cast<float>(ii->second);
        return;
    }
    *params = 0.0f; // GL default for an unset parameter
}

void Context::getSamplerParameterIiv(GLObjectName sampler, uint32_t pname,
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
    if (!isSamplerIntParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    auto it = s->params.find(pname);
    *params = (it != s->params.end()) ? static_cast<int32_t>(it->second) : 0;
}

void Context::getSamplerParameterIuiv(GLObjectName sampler, uint32_t pname,
                                      uint32_t* params) {
    SamplerObject* s = getSampler(sampler);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isSamplerIntParam(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    auto it = s->params.find(pname);
    *params = (it != s->params.end()) ? static_cast<uint32_t>(it->second) : 0u;
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

 void Context::drawArraysIndirect(uint32_t mode, const void* offset) {
     if (!backend_.capabilities().isSupported(Feature::IndirectDrawing)) {
         setError(GLError::InvalidOperation);
         return;
     }
     if (state_.activeProgram() == 0) {
         setError(GLError::InvalidOperation);
         return;
     }
     if (boundBuffer(GL_DRAW_INDIRECT_BUFFER) == 0) {
         setError(GLError::InvalidOperation);
         return;
     }
     flushState();
     backend_.drawArraysIndirect(mode, offset);
 }

    void Context::drawElementsIndirect(uint32_t mode, uint32_t type,
                                     const void* offset) {
        if (!backend_.capabilities().isSupported(Feature::IndirectDrawing)) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (state_.activeProgram() == 0) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (boundBuffer(GL_DRAW_INDIRECT_BUFFER) == 0) {
            setError(GLError::InvalidOperation);
            return;
        }
        flushState();
        backend_.drawElementsIndirect(mode, type, offset);
    }

    void Context::dispatchCompute(uint32_t x, uint32_t y, uint32_t z) {
        if (!backend_.capabilities().isSupported(Feature::ComputeShaders)) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (state_.activeProgram() == 0) {
            setError(GLError::InvalidOperation);
            return;
        }
        flushState();
        backend_.dispatchCompute(x, y, z);
    }

    void Context::dispatchComputeIndirect(uintptr_t offset) {
        if (!backend_.capabilities().isSupported(Feature::ComputeShaders)) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (state_.activeProgram() == 0) {
            setError(GLError::InvalidOperation);
            return;
        }
        if (boundBuffer(GL_DISPATCH_INDIRECT_BUFFER) == 0) {
            setError(GLError::InvalidOperation);
            return;
        }
        flushState();
        backend_.dispatchComputeIndirect(offset);
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

void Context::getShaderiv(GLObjectName shader, uint32_t pname, GLint* params) {
    const ShaderObject* s = getShader(shader);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    GLint result = 0;
    switch (pname) {
    case GL_SHADER_TYPE: result = static_cast<GLint>(s->stage); break;
    case GL_COMPILE_STATUS: result = s->compiled ? GL_TRUE : GL_FALSE; break;
    case GL_DELETE_STATUS: result = GL_FALSE; break; // frontend does not flag pending delete
    case GL_INFO_LOG_LENGTH: result = static_cast<GLint>(s->infoLog.size() + 1); break;
    case GL_SHADER_SOURCE_LENGTH: result = static_cast<GLint>(s->source.size() + 1); break;
    default:
        setError(GLError::InvalidEnum);
        return;
    }
    if (params) *params = result;
}

void Context::getProgramiv(GLObjectName program, uint32_t pname, GLint* params) {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    GLint result = 0;
    switch (pname) {
    case GL_LINK_STATUS: result = p->linked ? GL_TRUE : GL_FALSE; break;
    case GL_DELETE_STATUS: result = GL_FALSE; break;
    case GL_ATTACHED_SHADERS:
        result = static_cast<GLint>(p->attachedShaders.size());
        break;
    case GL_INFO_LOG_LENGTH: result = static_cast<GLint>(p->infoLog.size() + 1); break;
    case GL_ACTIVE_UNIFORMS:
        result = p->backend ? p->backend->activeUniformCount() : 0;
        break;
    case GL_ACTIVE_ATTRIBUTES:
        result = p->backend ? p->backend->activeAttributeCount() : 0;
        break;
    case GL_ACTIVE_UNIFORM_BLOCKS:
        result = p->backend ? p->backend->activeUniformBlockCount() : 0;
        break;
    case GL_PROGRAM_SEPARABLE: result = p->separable ? GL_TRUE : GL_FALSE; break;
    case GL_PROGRAM_BINARY_LENGTH:
        result = static_cast<GLint>(p->binary.size());
        break;
    case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
        result = static_cast<GLint>(p->tfBufferMode);
        break;
    case GL_TRANSFORM_FEEDBACK_VARYINGS:
        result = static_cast<GLint>(p->tfVaryings.size());
        break;
    default:
        setError(GLError::InvalidEnum);
        return;
    }
    if (params) *params = result;
}

GLint Context::getShaderiv(GLObjectName shader, uint32_t pname) {
    GLint v = 0;
    getShaderiv(shader, pname, &v);
    return v;
}

GLint Context::getProgramiv(GLObjectName program, uint32_t pname) {
    GLint v = 0;
    getProgramiv(program, pname, &v);
    return v;
}

// Table-6.1 buffer bind targets accepted by the buffer-parameter pointer queries.
static bool isBufferBindTarget(uint32_t target) {
    switch (target) {
    case GL_ARRAY_BUFFER:
    case GL_ATOMIC_COUNTER_BUFFER:
    case GL_COPY_READ_BUFFER:
    case GL_COPY_WRITE_BUFFER:
    case GL_DRAW_INDIRECT_BUFFER:
    case GL_DISPATCH_INDIRECT_BUFFER:
    case GL_ELEMENT_ARRAY_BUFFER:
    case GL_PIXEL_PACK_BUFFER:
    case GL_PIXEL_UNPACK_BUFFER:
    case GL_QUERY_BUFFER:
    case GL_SHADER_STORAGE_BUFFER:
    case GL_TEXTURE_BUFFER:
    case GL_TRANSFORM_FEEDBACK_BUFFER:
    case GL_UNIFORM_BUFFER:
        return true;
    default:
        return false;
    }
}

void Context::getAttachedShaders(GLObjectName program, int32_t maxCount,
                                 int32_t* count, GLObjectName* shaders) {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        // A non-program (shader or unused) name is rejected (SPEC §7.3.4).
        setError(GLError::InvalidOperation);
        return;
    }
    if (maxCount < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    int32_t actual = static_cast<int32_t>(p->attachedShaders.size());
    if (count != nullptr) *count = actual;
    if (shaders != nullptr) {
        int32_t n = std::min(maxCount, actual);
        for (int32_t i = 0; i < n; ++i) shaders[i] = p->attachedShaders[i];
    }
}

void Context::getShaderSource(GLObjectName shader, int32_t bufSize,
                              int32_t* length, char* source) {
    const ShaderObject* s = getShader(shader);
    if (s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    int32_t srcLen = static_cast<int32_t>(s->source.size());
    if (length != nullptr) *length = srcLen;
    if (source != nullptr && bufSize > 0) {
        int32_t copy = std::min(bufSize - 1, srcLen);
        if (copy > 0) std::memcpy(source, s->source.data(), static_cast<size_t>(copy));
        source[copy] = '\0';
    }
}

void Context::getBufferPointerv(uint32_t target, uint32_t pname, void** params) {
    if (!isBufferBindTarget(target)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (pname != GL_BUFFER_MAP_POINTER) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
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
    *params = obj->mapPointer;
}

void Context::getNamedBufferPointerv(GLObjectName buffer, uint32_t pname,
                                     void** params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (pname != GL_BUFFER_MAP_POINTER) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    BufferObject* obj = getBuffer(buffer);
    if (obj == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated name
        return;
    }
    *params = obj->mapPointer;
}

namespace {

uint64_t objectLabelKey(uint32_t identifier, uint32_t name) {
    return (static_cast<uint64_t>(identifier) << 32) | static_cast<uint64_t>(name);
}

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

bool isKnownProgramInterfacePname(uint32_t pname) {
    switch (pname) {
    case GL_ACTIVE_RESOURCES:
    case GL_MAX_RESOURCE_NAME_LENGTH:
    case GL_MAX_NUM_ACTIVE_VARIABLES:
    case GL_MAX_NUM_COMPATIBLE_SUBROUTINES:
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

void Context::getProgramInterfaceiv(GLObjectName program, uint32_t programInterface,
                                    uint32_t pname, int32_t* params) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidProgramInterface(programInterface)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isKnownProgramInterfacePname(pname)) {
        setError(GLError::InvalidEnum);
        return;
    }
    p->backend->getProgramInterfaceiv(programInterface, pname, params);
}

namespace {

// Map a glGetActiveUniformBlockiv pname to its GetProgramResourceiv property
// equivalent (SPEC §7.6 table 7.7). Returns false for an unsupported pname.
bool mapUniformBlockPname(uint32_t pname, uint32_t& prop) {
    switch (pname) {
    case GL_UNIFORM_BLOCK_BINDING:
        prop = GL_BUFFER_BINDING; return true;
    case GL_UNIFORM_BLOCK_DATA_SIZE:
        prop = GL_BUFFER_DATA_SIZE; return true;
    case GL_UNIFORM_BLOCK_NAME_LENGTH:
        prop = GL_NAME_LENGTH; return true;
    case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
        prop = GL_NUM_ACTIVE_VARIABLES; return true;
    case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
        prop = GL_ACTIVE_VARIABLES; return true;
    case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
        prop = GL_REFERENCED_BY_VERTEX_SHADER; return true;
    case GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER:
        prop = GL_REFERENCED_BY_TESS_CONTROL_SHADER; return true;
    case GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER:
        prop = GL_REFERENCED_BY_TESS_EVALUATION_SHADER; return true;
    case GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER:
        prop = GL_REFERENCED_BY_GEOMETRY_SHADER; return true;
    case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
        prop = GL_REFERENCED_BY_FRAGMENT_SHADER; return true;
    case GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER:
        prop = GL_REFERENCED_BY_COMPUTE_SHADER; return true;
    default:
        return false;
    }
}

// Map a glGetActiveUniformsiv pname to its GetProgramResource property
// (SPEC §7.3.1, table 7.6).
bool mapActiveUniformPname(uint32_t pname, uint32_t& prop) {
    switch (pname) {
    case GL_UNIFORM_TYPE:
        prop = GL_TYPE; return true;
    case GL_UNIFORM_SIZE:
        prop = GL_ARRAY_SIZE; return true;
    case GL_UNIFORM_NAME_LENGTH:
        prop = GL_NAME_LENGTH; return true;
    case GL_UNIFORM_BLOCK_INDEX:
        prop = GL_BLOCK_INDEX; return true;
    case GL_UNIFORM_OFFSET:
        prop = GL_OFFSET; return true;
    case GL_UNIFORM_ARRAY_STRIDE:
        prop = GL_ARRAY_STRIDE; return true;
    case GL_UNIFORM_MATRIX_STRIDE:
        prop = GL_MATRIX_STRIDE; return true;
    case GL_UNIFORM_IS_ROW_MAJOR:
        prop = GL_IS_ROW_MAJOR; return true;
    case GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX:
        prop = GL_ATOMIC_COUNTER_BUFFER_INDEX; return true;
    default:
        return false;
    }
}

} // namespace

void Context::getActiveUniform(GLObjectName program, uint32_t index, int32_t bufSize,
                               int32_t* length, int32_t* size, uint32_t* type,
                               char* name) {
    // Equivalent (SPEC §7.6) to GetProgramResourceName(UNIFORM, index) +
    // GetProgramResourceiv(UNIFORM, ARRAY_SIZE) + GetProgramResourceiv(UNIFORM, TYPE).
    getProgramResourceName(program, GL_UNIFORM, index, bufSize, length, name);
    if (size) {
        int32_t sz = 0;
        uint32_t props[] = { GL_ARRAY_SIZE };
        getProgramResourceiv(program, GL_UNIFORM, index, 1, props, 1, nullptr, &sz);
        *size = sz;
    }
    if (type) {
        int32_t t = 0;
        uint32_t props[] = { GL_TYPE };
        getProgramResourceiv(program, GL_UNIFORM, index, 1, props, 1, nullptr,
                             reinterpret_cast<int32_t*>(type));
    }
}

void Context::getActiveAttrib(GLObjectName program, uint32_t index, int32_t bufSize,
                              int32_t* length, int32_t* size, uint32_t* type,
                              char* name) {
    // Equivalent (SPEC §11.1) to GetProgramResourceName(PROGRAM_INPUT, index) +
    // GetProgramResourceiv(PROGRAM_INPUT, ARRAY_SIZE / TYPE).
    getProgramResourceName(program, GL_PROGRAM_INPUT, index, bufSize, length, name);
    if (size) {
        int32_t sz = 0;
        uint32_t props[] = { GL_ARRAY_SIZE };
        getProgramResourceiv(program, GL_PROGRAM_INPUT, index, 1, props, 1, nullptr,
                             &sz);
        *size = sz;
    }
    if (type) {
        int32_t t = 0;
        uint32_t props[] = { GL_TYPE };
        getProgramResourceiv(program, GL_PROGRAM_INPUT, index, 1, props, 1, nullptr,
                             reinterpret_cast<int32_t*>(type));
    }
}

void Context::getActiveUniformName(GLObjectName program, uint32_t uniformIndex,
                                    int32_t bufSize, int32_t* length, char* name) {
    // SPEC §7.3.1 glGetActiveUniformName: equivalent to
    // GetProgramResourceName(UNIFORM, uniformIndex, ...).
    getProgramResourceName(program, GL_UNIFORM, uniformIndex, bufSize, length, name);
}

void Context::getActiveUniformsiv(GLObjectName program, int32_t uniformCount,
                                  const uint32_t* uniformIndices, uint32_t pname,
                                  int32_t* params) {
    // SPEC §7.3.1 glGetActiveUniformsiv.
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (uniformCount < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (uniformCount > 0 && (uniformIndices == nullptr || params == nullptr)) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t prop;
    if (!mapActiveUniformPname(pname, prop)) {
        setError(GLError::InvalidEnum);
        return;
    }
    uint32_t count = p->backend->programResourceCount(GL_UNIFORM);
    for (int32_t i = 0; i < uniformCount; ++i) {
        if (uniformIndices[i] >= count) {
            setError(GLError::InvalidValue);
            return;
        }
    }
    for (int32_t i = 0; i < uniformCount; ++i) {
        getProgramResourceiv(program, GL_UNIFORM, uniformIndices[i], 1, &prop, 1,
                             nullptr, &params[i]);
    }
}

uint32_t Context::getUniformBlockIndex(GLObjectName program, const std::string& name) {
    // Equivalent (SPEC §7.6) to GetProgramResourceIndex(UNIFORM_BLOCK, name).
    // Returns GL_INVALID_INDEX honestly when the block is absent (no error).
    return getProgramResourceIndex(program, GL_UNIFORM_BLOCK, name);
}

void Context::getActiveUniformBlockiv(GLObjectName program, uint32_t index,
                                      uint32_t pname, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    uint32_t prop;
    if (!mapUniformBlockPname(pname, prop)) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (pname == GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES) {
        // Writes an array of NUM_ACTIVE_VARIABLES indices; size the buffer first.
        int32_t numVars = 0;
        uint32_t cntProp = GL_NUM_ACTIVE_VARIABLES;
        getProgramResourceiv(program, GL_UNIFORM_BLOCK, index, 1, &cntProp, 1,
                             nullptr, &numVars);
        if (numVars <= 0) {
            *params = 0;
            return;
        }
        std::vector<int32_t> tmp(static_cast<size_t>(numVars));
        uint32_t actProp = GL_ACTIVE_VARIABLES;
        getProgramResourceiv(program, GL_UNIFORM_BLOCK, index, 1, &actProp, numVars,
                             nullptr, tmp.data());
        for (int32_t i = 0; i < numVars; ++i) params[i] = tmp[i];
        return;
    }
    getProgramResourceiv(program, GL_UNIFORM_BLOCK, index, 1, &prop, 1, nullptr,
                         params);
}

void Context::getActiveUniformBlockName(GLObjectName program, uint32_t index,
                                         int32_t bufSize, int32_t* length, char* name) {
    // Equivalent (SPEC §7.6) to GetProgramResourceName(UNIFORM_BLOCK, index).
    getProgramResourceName(program, GL_UNIFORM_BLOCK, index, bufSize, length, name);
}

namespace {
// GL 4.6 guarantees at least this many uniform-buffer binding points
// (table 23.47). Used only as a floor for the blockBinding upper bound;
// real drivers report >= this via MAX_UNIFORM_BUFFER_BINDINGS.
constexpr uint32_t kMaxUniformBufferBindings = 36;
} // namespace

void Context::uniformBlockBinding(GLObjectName program, uint32_t blockIndex,
                                  uint32_t blockBinding) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr || !p->linked || !p->backend) {
        setError(GLError::InvalidOperation);
        return;
    }
    const uint32_t count = static_cast<uint32_t>(p->backend->activeUniformBlockCount());
    if (blockIndex >= count) {
        setError(GLError::InvalidValue);
        return;
    }
    if (blockBinding >= kMaxUniformBufferBindings) {
        setError(GLError::InvalidValue);
        return;
    }
    p->backend->uniformBlockBinding(blockIndex, blockBinding);
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

void Context::getProgramStageiv(GLObjectName program, uint32_t shadertype,
                                uint32_t pname, int32_t* values) {
    if (!backend_.capabilities().isSupported(Feature::Subroutines)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!isValidSubroutineStage(shadertype)) {
        setError(GLError::InvalidOperation);
        return;
    }
    switch (pname) {
    case GL_ACTIVE_SUBROUTINES:
    case GL_ACTIVE_SUBROUTINE_UNIFORMS:
    case GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS:
    case GL_ACTIVE_SUBROUTINE_MAX_LENGTH:
    case GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH:
    case GL_MAX_SUBROUTINES:
    case GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS:
        break;
    default:
        setError(GLError::InvalidValue);
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
    p->backend->getProgramStageiv(shadertype, pname, values);
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

void Context::getInteger64v(uint32_t pname, int64_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    // The tracked integer state is all representable in GLint; widen it to GLint64
    // per SPEC §22.1 (glGetInteger64v returns the same values as glGetIntegerv).
    int32_t buf[4] = {0, 0, 0, 0};
    int n = state_.getInteger(static_cast<GLenum>(pname), buf);
    if (n == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    for (int i = 0; i < n; ++i) params[i] = static_cast<int64_t>(buf[i]);
}

namespace {
bool isIndexableQueryCap(GLenum cap) {
    return cap == 0x0BE2 /* GL_BLEND */ || cap == 0x0C11 /* GL_SCISSOR_TEST */;
}
constexpr uint32_t kMaxIndexedQueryBuffers = 16; // matches GLStateTracker indexed cap range
} // namespace

void Context::getIntegeri_v(uint32_t pname, uint32_t index, int32_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (pname == GL_TRANSFORM_FEEDBACK_BUFFER_BINDING) {
        if (index >= kMaxTransformFeedbackBuffers) {
            setError(GLError::InvalidValue);
            return;
        }
        const TransformFeedbackObject::TfBufferBinding* slot =
            (boundTransformFeedback_ == 0)
                ? &defaultTransformFeedbackBuffers_[index]
                : tfBufferBindingSlot(boundTransformFeedback_, index);
        params[0] = slot ? static_cast<int32_t>(slot->buffer) : 0;
        return;
    }
    if (pname == GL_VIEWPORT || pname == GL_SCISSOR_BOX) {
        if (index >= GLStateTracker::kMaxViewports) {
            setError(GLError::InvalidValue);
            return;
        }
        if (pname == GL_VIEWPORT) {
            const auto& v = state_.viewport(index);
            params[0] = v.x;
            params[1] = v.y;
            params[2] = v.width;
            params[3] = v.height;
        } else {
            const auto& v = state_.scissor(index);
            params[0] = v.x;
            params[1] = v.y;
            params[2] = v.width;
            params[3] = v.height;
        }
        return;
    }
    if (!isIndexableQueryCap(static_cast<GLenum>(pname))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedQueryBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    bool enabled = false;
    state_.isIndexedCapabilityEnabled(static_cast<GLenum>(pname), index, &enabled);
    params[0] = enabled ? 1 : 0;
}

void Context::getBooleani_v(uint32_t pname, uint32_t index, unsigned char* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isIndexableQueryCap(static_cast<GLenum>(pname))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedQueryBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    bool enabled = false;
    state_.isIndexedCapabilityEnabled(static_cast<GLenum>(pname), index, &enabled);
    params[0] = enabled ? 0x01 : 0x00;
}

void Context::getFloati_v(uint32_t pname, uint32_t index, float* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (pname == GL_VIEWPORT || pname == GL_SCISSOR_BOX) {
        if (index >= GLStateTracker::kMaxViewports) {
            setError(GLError::InvalidValue);
            return;
        }
        if (pname == GL_VIEWPORT) {
            const auto& v = state_.viewport(index);
            params[0] = static_cast<float>(v.x);
            params[1] = static_cast<float>(v.y);
            params[2] = static_cast<float>(v.width);
            params[3] = static_cast<float>(v.height);
        } else {
            const auto& v = state_.scissor(index);
            params[0] = static_cast<float>(v.x);
            params[1] = static_cast<float>(v.y);
            params[2] = static_cast<float>(v.width);
            params[3] = static_cast<float>(v.height);
        }
        return;
    }
    if (!isIndexableQueryCap(static_cast<GLenum>(pname))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedQueryBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    bool enabled = false;
    state_.isIndexedCapabilityEnabled(static_cast<GLenum>(pname), index, &enabled);
    params[0] = enabled ? 1.0f : 0.0f;
}

void Context::getDoublei_v(uint32_t pname, uint32_t index, double* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (pname == GL_VIEWPORT || pname == GL_SCISSOR_BOX) {
        if (index >= GLStateTracker::kMaxViewports) {
            setError(GLError::InvalidValue);
            return;
        }
        if (pname == GL_VIEWPORT) {
            const auto& v = state_.viewport(index);
            params[0] = static_cast<double>(v.x);
            params[1] = static_cast<double>(v.y);
            params[2] = static_cast<double>(v.width);
            params[3] = static_cast<double>(v.height);
        } else {
            const auto& v = state_.scissor(index);
            params[0] = static_cast<double>(v.x);
            params[1] = static_cast<double>(v.y);
            params[2] = static_cast<double>(v.width);
            params[3] = static_cast<double>(v.height);
        }
        return;
    }
    if (!isIndexableQueryCap(static_cast<GLenum>(pname))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedQueryBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    bool enabled = false;
    state_.isIndexedCapabilityEnabled(static_cast<GLenum>(pname), index, &enabled);
    params[0] = enabled ? 1.0 : 0.0;
}

void Context::getInteger64i_v(uint32_t pname, uint32_t index, int64_t* params) {
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (pname == GL_TRANSFORM_FEEDBACK_BUFFER_BINDING) {
        if (index >= kMaxTransformFeedbackBuffers) {
            setError(GLError::InvalidValue);
            return;
        }
        const TransformFeedbackObject::TfBufferBinding* slot =
            (boundTransformFeedback_ == 0)
                ? &defaultTransformFeedbackBuffers_[index]
                : tfBufferBindingSlot(boundTransformFeedback_, index);
        params[0] = slot ? static_cast<int64_t>(slot->buffer) : 0;
        return;
    }
    if (!isIndexableQueryCap(static_cast<GLenum>(pname))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedQueryBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    bool enabled = false;
    state_.isIndexedCapabilityEnabled(static_cast<GLenum>(pname), index, &enabled);
    params[0] = enabled ? 1 : 0;
}

GLenum Context::getGraphicsResetStatus() {
    // No reset-detection path exists in this frontend (SPEC §22.5); report the
    // steady-state value.
    return GL_NO_ERROR;
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
// Only these capabilities are defined as indexable by the GL spec (SPEC §10.3.1,
// §22.3). Indexed enables for any other cap are GL_INVALID_ENUM.
bool isIndexableCap(GLenum cap) {
    return cap == 0x0BE2 /* GL_BLEND */ || cap == 0x0C11 /* GL_SCISSOR_TEST */;
}
constexpr uint32_t kMaxIndexedBuffers = 16; // covers MAX_DRAW_BUFFERS / MAX_VIEWPORTS
} // namespace

void Context::enableIndexed(uint32_t cap, uint32_t index) {
    if (!isIndexableCap(static_cast<GLenum>(cap))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setIndexedCapability(static_cast<GLenum>(cap), index, true);
}

void Context::disableIndexed(uint32_t cap, uint32_t index) {
    if (!isIndexableCap(static_cast<GLenum>(cap))) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (index >= kMaxIndexedBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setIndexedCapability(static_cast<GLenum>(cap), index, false);
}

bool Context::isEnabledIndexed(uint32_t cap, uint32_t index) {
    if (!isIndexableCap(static_cast<GLenum>(cap))) {
        setError(GLError::InvalidEnum);
        return false;
    }
    if (index >= kMaxIndexedBuffers) {
        setError(GLError::InvalidValue);
        return false;
    }
    bool enabled = false;
    state_.isIndexedCapabilityEnabled(static_cast<GLenum>(cap), index, &enabled);
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

bool Context::isShader(GLObjectName name) const {
    return shaders_.find(name) != shaders_.end();
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

void Context::detachShader(GLObjectName program, GLObjectName shader) {
    ProgramObject* p = getProgram(program);
    ShaderObject* s = getShader(shader);
    if (p == nullptr || s == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    // SPEC §7.4: detach does not undo an already-successful link, but it removes
    // the frontend association so a subsequent link will not include the shader.
    auto& attached = p->attachedShaders;
    for (auto it = attached.begin(); it != attached.end();) {
        if (*it == shader)
            it = attached.erase(it);
        else
            ++it;
    }
    if (p->backend && s->backend) p->backend->detach(*s->backend);
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
    // Apply any pre-link attribute bindings (SPEC §7.3.7 glBindAttribLocation).
    for (const auto& b : p->attribBindings) {
        if (p->backend) p->backend->bindAttribLocation(b.first, b.second);
    }
    // Apply any pre-link fragment-output bindings (SPEC §7.3.7 / §15.1.2
    // glBindFragDataLocation / glBindFragDataLocationIndexed).
    for (const auto& b : p->fragDataBindings) {
        int idx = 0;
        auto it = p->fragDataIndexBindings.find(b.first);
        if (it != p->fragDataIndexBindings.end()) idx = it->second;
        if (p->backend) p->backend->bindFragDataLocation(b.first, b.second, idx);
    }
    // Apply any pre-link transform-feedback varying capture setup (SPEC §13.3.1
    // glTransformFeedbackVaryings).
    if (p->backend && !p->tfVaryings.empty()) {
        p->backend->transformFeedbackVaryings(p->tfVaryings, p->tfBufferMode);
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

void Context::programParameteri(GLObjectName program, uint32_t pname, int32_t value) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    switch (pname) {
    case GL_PROGRAM_SEPARABLE:
        // SPEC §7.3: this parameter must be set before linking. Once the program
        // is linked, changing it is an error.
        if (p->linked) {
            setError(GLError::InvalidOperation);
            return;
        }
        p->separable = (value != 0);
        return;
    case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
        p->binaryRetrievableHint = (value != 0);
        return;
    default:
        setError(GLError::InvalidEnum);
        return;
    }
}

void Context::programBinary(GLObjectName program, uint32_t binaryFormat,
                            const void* binary, GLsizei length) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (length < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (binaryFormat == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    // A precompiled binary fully defines the program; mark it linked and store the
    // authoritative frontend mirror (SPEC §7.3 / §19.1).
    p->binary.assign(static_cast<const uint8_t*>(binary),
                     static_cast<const uint8_t*>(binary) + length);
    p->binaryFormat = binaryFormat;
    p->linked = true;
    if (p->backend) p->backend->loadBinary(binaryFormat, binary, length);
}

void Context::getProgramBinary(GLObjectName program, GLsizei bufSize, GLsizei* length,
                               uint32_t* binaryFormat, void* binary) {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    // The binary is only retrievable when one was loaded (the mock has no native
    // compiler to produce a driver binary). Per SPEC §19.1: GL_INVALID_OPERATION
    // when the program's binary is not retrievable.
    if (p->binary.empty()) {
        setError(GLError::InvalidOperation);
        return;
    }
    const GLsizei len = static_cast<GLsizei>(p->binary.size());
    // bufSize is only validated when a non-null destination is supplied.
    if (binary != nullptr && bufSize < len) {
        setError(GLError::InvalidValue);
        return;
    }
    if (length != nullptr) *length = len;
    if (binaryFormat != nullptr) *binaryFormat = p->binaryFormat;
    if (binary != nullptr) {
        std::memcpy(binary, p->binary.data(), static_cast<size_t>(std::min(len, bufSize)));
    }
}

void Context::shaderBinary(GLsizei count, const GLuint* shaders, uint32_t binaryFormat,
                           const void* binary, GLsizei length) {
    if (count < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (binaryFormat == 0) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (length < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        ShaderObject* s = getShader(shaders[i]);
        if (s == nullptr) {
            setError(GLError::InvalidOperation);
            return;
        }
        s->binary.assign(static_cast<const uint8_t*>(binary),
                         static_cast<const uint8_t*>(binary) + length);
        s->binaryFormat = binaryFormat;
        s->compiled = true;
        if (s->backend) s->backend->loadBinary(binaryFormat, binary, length);
    }
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

int Context::getFragDataLocation(GLObjectName program, const std::string& name) const {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        const_cast<Context*>(this)->setError(GLError::InvalidOperation);
        return -1;
    }
    if (p->backend) return p->backend->getFragDataLocation(name);
    return -1;
}

int Context::getFragDataIndex(GLObjectName program, const std::string& name) const {
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        const_cast<Context*>(this)->setError(GLError::InvalidOperation);
        return -1;
    }
    if (p->backend) return p->backend->getFragDataIndex(name);
    return -1;
}

void Context::getTransformFeedbackVarying(GLObjectName program, uint32_t index,
                                          int bufSize, int* length, int* size,
                                          uint32_t* type, char* name) const {
    if (bufSize < 0) {
        const_cast<Context*>(this)->setError(GLError::InvalidValue);
        return;
    }
    const ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        const_cast<Context*>(this)->setError(GLError::InvalidOperation);
        return;
    }
    if (p->backend &&
        p->backend->getTransformFeedbackVarying(index, bufSize, length, size, type,
                                                name)) {
        return;
    }
    // Backend has no introspection or `index` is out of range -> INVALID_VALUE
    // (SPEC §13.3.1: out-of-range index generates INVALID_VALUE; a backend
    // without transform-feedback introspection cannot distinguish, so it maps
    // to the same error, honest for e.g. GLES).
    const_cast<Context*>(this)->setError(GLError::InvalidValue);
}

void Context::bindAttribLocation(GLObjectName program, uint32_t index,
                                 const std::string& name) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    // Record the binding; it is applied to the backend program at the next link
    // (SPEC §7.3.7: bindAttribLocation only takes effect on subsequent link).
    p->attribBindings[name] = static_cast<int>(index);
}

void Context::bindFragDataLocationIndexed(GLObjectName program,
                                          uint32_t colorNumber, uint32_t index,
                                          const std::string& name) {
    // SPEC §15.1.2: a shader-object name reports GL_INVALID_OPERATION; a name that
    // is neither a program nor shader (e.g. 0 / ungenerated) reports
    // GL_INVALID_VALUE.
    if (getShader(program) != nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (index > 1) {
        setError(GLError::InvalidValue);
        return;
    }
    if (colorNumber >= GLStateTracker::kMaxDrawBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!name.empty() && name.rfind("gl_", 0) == 0) {
        setError(GLError::InvalidOperation);
        return;
    }
    // Recorded frontend-side; applied to the backend program at the next link
    // (SPEC §7.3.7: bindFragDataLocation* only takes effect on subsequent link).
    p->fragDataBindings[name] = static_cast<int>(colorNumber);
    p->fragDataIndexBindings[name] = static_cast<int>(index);
}

void Context::bindFragDataLocation(GLObjectName program, uint32_t colorNumber,
                                   const std::string& name) {
    bindFragDataLocationIndexed(program, colorNumber, 0, name);
}

void Context::transformFeedbackVaryings(GLObjectName program, GLsizei count,
                                        const char* const* varyings,
                                        uint32_t bufferMode) {
    ProgramObject* p = getProgram(program);
    if (p == nullptr) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (count < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (bufferMode != GL_INTERLEAVED_ATTRIBS && bufferMode != GL_SEPARATE_ATTRIBS) {
        setError(GLError::InvalidEnum);
        return;
    }
    // SPEC §13.3.1: transform feedback varyings must be specified before linking.
    if (p->linked) {
        setError(GLError::InvalidOperation);
        return;
    }
    // Record the request; it is applied to the backend program at the next link
    // (SPEC §13.3.1: transformFeedbackVaryings only takes effect on subsequent
    // link).
    p->tfVaryings.clear();
    for (GLsizei i = 0; i < count; ++i) {
        p->tfVaryings.push_back(varyings && varyings[i] ? std::string(varyings[i])
                                                        : std::string());
    }
    p->tfBufferMode = bufferMode;
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

bool Context::isProgram(GLObjectName name) const {
    return programs_.find(name) != programs_.end();
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

void Context::createProgramPipelines(uint32_t n, GLObjectName* names) {
    if (names == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    genProgramPipelines(n, names);
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

// --- Current generic vertex attribute values (SPEC §10.2) ---

// GL guarantees at least 16 vertex attributes; the flush only iterates recorded
// ones, so a fixed cap here merely guards the required GL_INVALID_VALUE path.
static constexpr uint32_t kMaxVertexAttribs = 16;

namespace {
void setAttribCurrent(VertexArrayObject::AttribState& a, const double v[4],
                     uint32_t type) {
    for (int i = 0; i < 4; ++i) a.currentValue[i] = v[i];
    a.currentType = type;
}
}  // namespace

void Context::vertexAttrib1f(uint32_t index, float x) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    double v[4] = {x, 0.0, 0.0, 1.0};
    setAttribCurrent(getVertexArray(boundVertexArray_)->attrib(index), v, GL_FLOAT);
}

void Context::vertexAttrib2f(uint32_t index, float x, float y) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    double v[4] = {x, y, 0.0, 1.0};
    setAttribCurrent(getVertexArray(boundVertexArray_)->attrib(index), v, GL_FLOAT);
}

void Context::vertexAttrib3f(uint32_t index, float x, float y, float z) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    double v[4] = {x, y, z, 1.0};
    setAttribCurrent(getVertexArray(boundVertexArray_)->attrib(index), v, GL_FLOAT);
}

void Context::vertexAttrib4f(uint32_t index, float x, float y, float z, float w) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    double v[4] = {x, y, z, w};
    setAttribCurrent(getVertexArray(boundVertexArray_)->attrib(index), v, GL_FLOAT);
}

void Context::vertexAttrib1fv(uint32_t index, const float* v) {
    if (v == nullptr) { setError(GLError::InvalidValue); return; }
    vertexAttrib1f(index, v[0]);
}

void Context::vertexAttrib2fv(uint32_t index, const float* v) {
    if (v == nullptr) { setError(GLError::InvalidValue); return; }
    vertexAttrib2f(index, v[0], v[1]);
}

void Context::vertexAttrib3fv(uint32_t index, const float* v) {
    if (v == nullptr) { setError(GLError::InvalidValue); return; }
    vertexAttrib3f(index, v[0], v[1], v[2]);
}

void Context::vertexAttrib4fv(uint32_t index, const float* v) {
    if (v == nullptr) { setError(GLError::InvalidValue); return; }
    vertexAttrib4f(index, v[0], v[1], v[2], v[3]);
}

void Context::vertexAttribI4i(uint32_t index, int32_t x, int32_t y, int32_t z,
                              int32_t w) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    double v[4] = {double(x), double(y), double(z), double(w)};
    setAttribCurrent(getVertexArray(boundVertexArray_)->attrib(index), v, GL_INT);
}

void Context::vertexAttribI4ui(uint32_t index, uint32_t x, uint32_t y, uint32_t z,
                               uint32_t w) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    double v[4] = {double(x), double(y), double(z), double(w)};
    setAttribCurrent(getVertexArray(boundVertexArray_)->attrib(index), v,
                     GL_UNSIGNED_INT);
}

void Context::vertexAttribI4iv(uint32_t index, const int32_t* v) {
    if (v == nullptr) { setError(GLError::InvalidValue); return; }
    vertexAttribI4i(index, v[0], v[1], v[2], v[3]);
}

void Context::vertexAttribI4uiv(uint32_t index, const uint32_t* v) {
    if (v == nullptr) { setError(GLError::InvalidValue); return; }
    vertexAttribI4ui(index, v[0], v[1], v[2], v[3]);
}

namespace {
// Fills `out` (int32_t[4]) for the integer/boolean vertex-attribute pnames and
// returns the element count (1 or 4). Returns 0 for unsupported pnames so the
// caller can raise GL_INVALID_ENUM.
int getVertexAttribIntParams(const VertexArrayObject::AttribState& a,
                             GLenum pname, int32_t out[4]) {
    switch (pname) {
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            out[0] = a.enabled ? 1 : 0; return 1;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            out[0] = a.size; return 1;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            out[0] = a.stride; return 1;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            out[0] = static_cast<int32_t>(a.type); return 1;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            out[0] = a.normalized ? 1 : 0; return 1;
        case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
            out[0] = (a.currentType != GL_FLOAT) ? 1 : 0; return 1;
        case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
            out[0] = static_cast<int32_t>(a.divisor); return 1;
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            out[0] = static_cast<int32_t>(a.buffer); return 1;
        case GL_CURRENT_VERTEX_ATTRIB:
            for (int i = 0; i < 4; ++i)
                out[i] = static_cast<int32_t>(a.currentValue[i]);
            return 4;
        default:
            return 0;
    }
}
}  // namespace

void Context::getVertexAttribfv(uint32_t index, GLenum pname, float* params) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    if (params == nullptr) { setError(GLError::InvalidValue); return; }
    if (pname != GL_CURRENT_VERTEX_ATTRIB) { setError(GLError::InvalidEnum); return; }
    const auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    for (int i = 0; i < 4; ++i) params[i] = static_cast<float>(a.currentValue[i]);
}

void Context::getVertexAttribdv(uint32_t index, GLenum pname, double* params) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    if (params == nullptr) { setError(GLError::InvalidValue); return; }
    if (pname != GL_CURRENT_VERTEX_ATTRIB) { setError(GLError::InvalidEnum); return; }
    const auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    for (int i = 0; i < 4; ++i) params[i] = a.currentValue[i];
}

void Context::getVertexAttribiv(uint32_t index, GLenum pname, int32_t* params) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    if (params == nullptr) { setError(GLError::InvalidValue); return; }
    const auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    int32_t out[4] = {0, 0, 0, 0};
    int n = getVertexAttribIntParams(a, pname, out);
    if (n == 0) { setError(GLError::InvalidEnum); return; }
    for (int i = 0; i < n; ++i) params[i] = out[i];
}

void Context::getVertexAttribIiv(uint32_t index, GLenum pname, int32_t* params) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    if (params == nullptr) { setError(GLError::InvalidValue); return; }
    if (pname != GL_CURRENT_VERTEX_ATTRIB &&
        pname != GL_VERTEX_ATTRIB_ARRAY_INTEGER) {
        setError(GLError::InvalidEnum); return;
    }
    const auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    if (pname == GL_VERTEX_ATTRIB_ARRAY_INTEGER) {
        params[0] = (a.currentType != GL_FLOAT) ? 1 : 0;
        return;
    }
    for (int i = 0; i < 4; ++i)
        params[i] = static_cast<int32_t>(a.currentValue[i]);
}

void Context::getVertexAttribIuiv(uint32_t index, GLenum pname, uint32_t* params) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    if (params == nullptr) { setError(GLError::InvalidValue); return; }
    if (pname != GL_CURRENT_VERTEX_ATTRIB &&
        pname != GL_VERTEX_ATTRIB_ARRAY_INTEGER) {
        setError(GLError::InvalidEnum); return;
    }
    const auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    if (pname == GL_VERTEX_ATTRIB_ARRAY_INTEGER) {
        params[0] = (a.currentType != GL_FLOAT) ? 1u : 0u;
        return;
    }
    for (int i = 0; i < 4; ++i)
        params[i] = static_cast<uint32_t>(a.currentValue[i]);
}

void Context::getVertexAttribPointerv(uint32_t index, GLenum pname, void** params) {
    if (boundVertexArray_ == 0) { setError(GLError::InvalidOperation); return; }
    if (index >= kMaxVertexAttribs) { setError(GLError::InvalidValue); return; }
    if (params == nullptr) { setError(GLError::InvalidValue); return; }
    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) { setError(GLError::InvalidEnum); return; }
    const auto& a = getVertexArray(boundVertexArray_)->attrib(index);
    *params = reinterpret_cast<void*>(a.offset);
}

void Context::getVertexArrayiv(GLObjectName vao, uint32_t pname, int32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vaoObj = getVertexArray(vao);
    if (vaoObj == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated VAO name
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    switch (pname) {
        case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            *params = static_cast<int32_t>(vaoObj->elementBuffer);
            return;
        default:
            setError(GLError::InvalidEnum);
            *params = 0;
            return;
    }
}

void Context::getVertexArrayIndexediv(GLObjectName vao, uint32_t index,
                                      uint32_t pname, int32_t* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vaoObj = getVertexArray(vao);
    if (vaoObj == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated VAO name
        return;
    }
    if (index >= kMaxVertexAttribs) {
        setError(GLError::InvalidValue);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    const auto& a = vaoObj->attrib(index);
    switch (pname) {
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = a.enabled ? 1 : 0; return;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = a.size; return;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = a.stride; return;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = static_cast<int32_t>(a.type); return;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = a.normalized ? 1 : 0; return;
        case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
            *params = (a.currentType != GL_FLOAT) ? 1 : 0; return;
        case GL_VERTEX_ATTRIB_ARRAY_LONG:
            *params = 0; return;
        case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
            *params = static_cast<int32_t>(a.divisor); return;
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = static_cast<int32_t>(a.buffer); return;
        default:
            setError(GLError::InvalidEnum);
            *params = 0;
            return;
    }
}

void Context::getVertexArrayIndexed64iv(GLObjectName vao, uint32_t index,
                                       uint32_t pname, int64_t* params) {
    if (!backend_.capabilities().isSupported(Feature::DirectStateAccess)) {
        setError(GLError::InvalidOperation);
        return;
    }
    VertexArrayObject* vaoObj = getVertexArray(vao);
    if (vaoObj == nullptr) {
        setError(GLError::InvalidOperation); // ungenerated VAO name
        return;
    }
    if (index >= kMaxVertexAttribs) {
        setError(GLError::InvalidValue);
        return;
    }
    if (params == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    const auto& a = vaoObj->attrib(index);
    switch (pname) {
        case GL_VERTEX_ATTRIB_BINDING:
            *params = static_cast<int64_t>(a.binding); return;
        case GL_VERTEX_ATTRIB_RELATIVE_OFFSET:
            *params = static_cast<int64_t>(a.relativeoffset); return;
        default:
            setError(GLError::InvalidEnum);
            *params = 0;
            return;
    }
}

// --- Hints (SPEC §21.1.1) ---

namespace {
bool isValidHintTarget(GLenum target) {
    switch (target) {
        case GL_PERSPECTIVE_CORRECTION_HINT:
        case GL_POINT_SMOOTH_HINT:
        case GL_LINE_SMOOTH_HINT:
        case GL_POLYGON_SMOOTH_HINT:
        case GL_FOG_HINT:
        case GL_TEXTURE_COMPRESSION_HINT:
        case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
        case GL_GENERATE_MIPMAP_HINT:
            return true;
        default:
            return false;
    }
}
bool isValidHintMode(GLenum mode) {
    return mode == GL_DONT_CARE || mode == GL_FASTEST || mode == GL_NICEST;
}
}  // namespace

void Context::hint(uint32_t target, uint32_t mode) {
    if (!isValidHintTarget(target)) { setError(GLError::InvalidEnum); return; }
    if (!isValidHintMode(mode)) { setError(GLError::InvalidEnum); return; }
    state_.setHint(target, mode);
}

uint32_t Context::getHint(uint32_t target) {
    if (!isValidHintTarget(target)) { setError(GLError::InvalidEnum); return 0; }
    return state_.getHint(target);
}

// --- Conditional rendering (SPEC §10.11) ---

bool Context::isConditionalRenderQueryType(uint32_t target) const {
    switch (target) {
    case GL_SAMPLES_PASSED:
    case GL_ANY_SAMPLES_PASSED:
    case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
    case GL_PRIMITIVES_GENERATED:
        return true;
    default:
        return false;
    }
}

void Context::beginConditionalRender(GLObjectName id, uint32_t mode) {
    if (!backend_.capabilities().isSupported(Feature::ConditionalRendering)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (conditionalRenderActive_) {
        setError(GLError::InvalidOperation); // already in a region
        return;
    }
    QueryObject* q = getQuery(id);
    if (!q) {
        setError(GLError::InvalidOperation); // not a query object
        return;
    }
    if (q->active) {
        setError(GLError::InvalidOperation); // query still active
        return;
    }
    if (!isConditionalRenderQueryType(q->target)) {
        setError(GLError::InvalidOperation); // wrong query type
        return;
    }
    switch (mode) {
    case GL_QUERY_WAIT:
    case GL_QUERY_NO_WAIT:
    case GL_QUERY_BY_REGION_WAIT:
    case GL_QUERY_BY_REGION_NO_WAIT:
    case GL_QUERY_WAIT_INVERTED:
    case GL_QUERY_NO_WAIT_INVERTED:
    case GL_QUERY_BY_REGION_WAIT_INVERTED:
    case GL_QUERY_BY_REGION_NO_WAIT_INVERTED:
        break;
    default:
        setError(GLError::InvalidEnum);
        return;
    }
    if (GLStateSink* sink = backend_.stateSink())
        sink->beginConditionalRender(id, mode);
    conditionalRenderActive_ = true;
    conditionalRenderQuery_ = id;
}

void Context::endConditionalRender() {
    if (!backend_.capabilities().isSupported(Feature::ConditionalRendering)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (!conditionalRenderActive_) {
        setError(GLError::InvalidOperation); // not in a region
        return;
    }
    if (GLStateSink* sink = backend_.stateSink()) sink->endConditionalRender();
    conditionalRenderActive_ = false;
    conditionalRenderQuery_ = 0;
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

namespace {
// SPEC §10.3 implementation limits. GL guarantees these minimum maxima
// (MAX_VERTEX_ATTRIB_BINDINGS >= 16, MAX_VERTEX_ATTRIB_STRIDE >= 2048,
// MAX_VERTEX_ATTRIB_RELATIVE_OFFSET >= 2047); the frontend enforces the
// guaranteed minimums so state accepted here is portable across backends.
constexpr uint32_t kMaxVertexAttribBindings = 16;
constexpr int32_t kMaxVertexAttribStride = 2048;

// SPEC §10.3.2: BindVertexBuffers with a null `buffers` array resets every
// touched binding point to no buffer, offset 0 and stride 16.
constexpr int32_t kNullVertexBufferStride = 16;

// Errors shared by BindVertexBuffer / VertexArrayVertexBuffer and their
// multi-bind forms (SPEC §10.3.2). GLError::NoError means the parameters are
// legal.
GLError validateVertexBufferParams(uint32_t bindingindex, intptr_t offset,
                                   int32_t stride) {
    if (bindingindex >= kMaxVertexAttribBindings) return GLError::InvalidValue;
    if (offset < 0 || stride < 0 || stride > kMaxVertexAttribStride)
        return GLError::InvalidValue;
    return GLError::NoError;
}
} // namespace

void Context::bindVertexBufferImpl(VertexArrayObject& vao,
                                   uint32_t bindingindex, GLObjectName buffer,
                                   intptr_t offset, int32_t stride) {
    const GLError err = validateVertexBufferParams(bindingindex, offset, stride);
    if (err != GLError::NoError) {
        setError(err);
        return;
    }
    if (buffer != 0 && buffers_.find(buffer) == buffers_.end()) {
        setError(GLError::InvalidOperation); // ungenerated buffer name
        return;
    }
    auto& b = vao.bindings[bindingindex];
    b.buffer = buffer;
    b.offset = offset;
    b.stride = stride;
    vertexStateDirty_ = true;
}

void Context::bindVertexBuffersImpl(VertexArrayObject& vao, uint32_t first,
                                    GLsizei count, const GLObjectName* buffers,
                                    const intptr_t* offsets,
                                    const int32_t* strides) {
    if (count < 0) {
        setError(GLError::InvalidValue); // SPEC §10.3.2: count negative
        return;
    }
    const uint32_t n = static_cast<uint32_t>(count);
    if (first > kMaxVertexAttribBindings ||
        n > kMaxVertexAttribBindings - first) {
        // SPEC §10.3.2: first + count past MAX_VERTEX_ATTRIB_BINDINGS is
        // GL_INVALID_OPERATION.
        setError(GLError::InvalidOperation);
        return;
    }
    // A null `buffers` array resets the whole range (offsets/strides ignored).
    if (buffers == nullptr) {
        for (uint32_t i = 0; i < n; ++i) {
            auto& b = vao.bindings[first + i];
            b.buffer = 0;
            b.offset = 0;
            b.stride = kNullVertexBufferStride;
        }
        if (n != 0) vertexStateDirty_ = true;
        return;
    }
    if (offsets == nullptr || strides == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    // SPEC §10.3.2: values are checked separately per binding point. An invalid
    // entry leaves that binding point unchanged and generates an error; the
    // other entries still apply.
    GLError firstError = GLError::NoError;
    bool changed = false;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t bindingindex = first + i;
        GLError err =
            validateVertexBufferParams(bindingindex, offsets[i], strides[i]);
        if (err == GLError::NoError && buffers[i] != 0 &&
            buffers_.find(buffers[i]) == buffers_.end()) {
            err = GLError::InvalidOperation; // ungenerated buffer name
        }
        if (err != GLError::NoError) {
            if (firstError == GLError::NoError) firstError = err;
            continue; // this binding point stays unchanged
        }
        auto& b = vao.bindings[bindingindex];
        b.buffer = buffers[i];
        b.offset = offsets[i];
        b.stride = strides[i];
        changed = true;
    }
    if (changed) vertexStateDirty_ = true;
    if (firstError != GLError::NoError) setError(firstError);
}

void Context::vertexAttribBindingImpl(VertexArrayObject& vao,
                                      uint32_t attribindex,
                                      uint32_t bindingindex) {
    if (attribindex >= kMaxVertexAttribs ||
        bindingindex >= kMaxVertexAttribBindings) {
        setError(GLError::InvalidValue);
        return;
    }
    vao.attrib(attribindex).binding = bindingindex;
    vertexStateDirty_ = true;
}

void Context::vertexBindingDivisorImpl(VertexArrayObject& vao,
                                       uint32_t bindingindex,
                                       uint32_t divisor) {
    if (bindingindex >= kMaxVertexAttribBindings) {
        setError(GLError::InvalidValue);
        return;
    }
    vao.bindings[bindingindex].divisor = divisor;
    vertexStateDirty_ = true;
}

VertexArrayObject* Context::boundVertexArrayForEdit() {
    VertexArrayObject* vao = getVertexArray(boundVertexArray_);
    if (vao == nullptr) setError(GLError::InvalidOperation);
    return vao;
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
    bindVertexBufferImpl(*vao, bindingindex, buffer, offset, stride);
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
    bindVertexBuffersImpl(*vao, first, static_cast<GLsizei>(count), buffers,
                          offsets, strides);
}

void Context::bindVertexBuffer(uint32_t bindingindex, GLObjectName buffer,
                               intptr_t offset, int32_t stride) {
    if (VertexArrayObject* vao = boundVertexArrayForEdit())
        bindVertexBufferImpl(*vao, bindingindex, buffer, offset, stride);
}

void Context::bindVertexBuffers(uint32_t first, GLsizei count,
                                const GLObjectName* buffers,
                                const intptr_t* offsets,
                                const int32_t* strides) {
    if (VertexArrayObject* vao = boundVertexArrayForEdit())
        bindVertexBuffersImpl(*vao, first, count, buffers, offsets, strides);
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
    vertexAttribBindingImpl(*vao, attribindex, bindingindex);
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
    vertexBindingDivisorImpl(*vao, bindingindex, divisor);
}

// Non-DSA separate attribute-format commands (SPEC §10.3.2/§10.3.4): identical
// to the glVertexArray* forms above except that the vertex array object is the
// one bound to GL_VERTEX_ARRAY_BINDING.
void Context::vertexAttribFormat(uint32_t attribindex, int32_t size,
                                 uint32_t type, bool normalized,
                                 uint32_t relativeoffset) {
    VertexArrayObject* vao = boundVertexArrayForEdit();
    if (vao == nullptr) return;
    if (attribindex >= kMaxVertexAttribs || size < 1 || size > 4) {
        setError(GLError::InvalidValue);
        return;
    }
    setAttribFormat(*vao, attribindex, size, type, normalized, relativeoffset);
    vertexStateDirty_ = true;
}

void Context::vertexAttribIFormat(uint32_t attribindex, int32_t size,
                                  uint32_t type, uint32_t relativeoffset) {
    VertexArrayObject* vao = boundVertexArrayForEdit();
    if (vao == nullptr) return;
    if (attribindex >= kMaxVertexAttribs || size < 1 || size > 4) {
        setError(GLError::InvalidValue);
        return;
    }
    // Integer attributes are never normalized (SPEC §10.3.2).
    setAttribFormat(*vao, attribindex, size, type, false, relativeoffset);
    vertexStateDirty_ = true;
}

void Context::vertexAttribLFormat(uint32_t attribindex, int32_t size,
                                  uint32_t type, uint32_t relativeoffset) {
    VertexArrayObject* vao = boundVertexArrayForEdit();
    if (vao == nullptr) return;
    if (attribindex >= kMaxVertexAttribs || size < 1 || size > 4) {
        setError(GLError::InvalidValue);
        return;
    }
    // Double-precision attributes are never normalized (SPEC §10.3.2).
    setAttribFormat(*vao, attribindex, size, type, false, relativeoffset);
    vertexStateDirty_ = true;
}

void Context::vertexAttribBinding(uint32_t attribindex, uint32_t bindingindex) {
    if (VertexArrayObject* vao = boundVertexArrayForEdit())
        vertexAttribBindingImpl(*vao, attribindex, bindingindex);
}

void Context::vertexBindingDivisor(uint32_t bindingindex, uint32_t divisor) {
    if (VertexArrayObject* vao = boundVertexArrayForEdit())
        vertexBindingDivisorImpl(*vao, bindingindex, divisor);
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

void Context::setViewportIndexed(GLuint index, GLint x, GLint y, GLsizei width,
                                  GLsizei height) {
    if (width < 0 || height < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (index >= GLStateTracker::kMaxViewports) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setViewportIndexed(index, x, y, width, height);
}

void Context::setScissorIndexed(GLuint index, GLint x, GLint y, GLsizei width,
                                 GLsizei height) {
    if (width < 0 || height < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (index >= GLStateTracker::kMaxViewports) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setScissorIndexed(index, x, y, width, height);
}

void Context::setViewportArrayv(GLuint first, GLsizei count, const GLfloat* v) {
    if (count <= 0 || first + static_cast<uint32_t>(count) >
                          GLStateTracker::kMaxViewports ||
        v == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        if (v[4 * i + 2] < 0.0f || v[4 * i + 3] < 0.0f) {
            setError(GLError::InvalidValue);
            return;
        }
    }
    state_.setViewportIndexedv(first, static_cast<uint32_t>(count), v);
}

void Context::setScissorArrayv(GLuint first, GLsizei count, const GLint* v) {
    if (count <= 0 || first + static_cast<uint32_t>(count) >
                          GLStateTracker::kMaxViewports ||
        v == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        if (v[4 * i + 2] < 0 || v[4 * i + 3] < 0) {
            setError(GLError::InvalidValue);
            return;
        }
    }
    state_.setScissorIndexedv(first, static_cast<uint32_t>(count), v);
}

void Context::setDepthRangeIndexed(GLuint index, GLdouble nearVal,
                                   GLdouble farVal) {
    if (index >= GLStateTracker::kMaxViewports) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setDepthRangeIndexed(index, static_cast<double>(nearVal),
                                static_cast<double>(farVal));
}

void Context::setDepthRangeArrayv(GLuint first, GLsizei count,
                                  const GLdouble* v) {
    if (count <= 0 || first + static_cast<uint32_t>(count) >
                          GLStateTracker::kMaxViewports ||
        v == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        state_.setDepthRangeIndexed(first + static_cast<uint32_t>(i),
                                    static_cast<double>(v[2 * i]),
                                    static_cast<double>(v[2 * i + 1]));
    }
}

void Context::setColorMaski(GLuint buf, GLboolean red, GLboolean green,
                            GLboolean blue, GLboolean alpha) {
    if (buf >= GLStateTracker::kMaxDrawBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    state_.setColorMaski(buf, red != GL_FALSE, green != GL_FALSE,
                         blue != GL_FALSE, alpha != GL_FALSE);
}

void Context::setClearColor(float r, float g, float b, float a) {
    state_.setClearColor(r, g, b, a);
}

namespace {

bool isValidBlendFactor(GLenum f) {
    switch (f) {
    case 0x0000: // GL_ZERO
    case 0x0001: // GL_ONE
    case 0x0300: // GL_SRC_COLOR
    case 0x0301: // GL_ONE_MINUS_SRC_COLOR
    case 0x0306: // GL_DST_COLOR
    case 0x0307: // GL_ONE_MINUS_DST_COLOR
    case 0x0302: // GL_SRC_ALPHA
    case 0x0303: // GL_ONE_MINUS_SRC_ALPHA
    case 0x0304: // GL_DST_ALPHA
    case 0x0305: // GL_ONE_MINUS_DST_ALPHA
    case 0x8001: // GL_CONSTANT_COLOR
    case 0x8002: // GL_ONE_MINUS_CONSTANT_COLOR
    case 0x8003: // GL_CONSTANT_ALPHA
    case 0x8004: // GL_ONE_MINUS_CONSTANT_ALPHA
    case 0x0308: // GL_SRC_ALPHA_SATURATE
    case 0x88F9: // GL_SRC1_COLOR
    case 0x88FA: // GL_ONE_MINUS_SRC1_COLOR
    case 0x8589: // GL_SRC1_ALPHA
    case 0x88FB: // GL_ONE_MINUS_SRC1_ALPHA
        return true;
    }
    return false;
}

bool isValidBlendEquation(GLenum m) {
    switch (m) {
    case 0x8006: // GL_FUNC_ADD
    case 0x800A: // GL_FUNC_SUBTRACT
    case 0x800B: // GL_FUNC_REVERSE_SUBTRACT
    case 0x8007: // GL_MIN
    case 0x8008: // GL_MAX
        return true;
    }
    return false;
}

} // namespace

void Context::setBlendFunci(GLuint buf, GLenum src, GLenum dst) {
    if (buf >= GLStateTracker::kMaxDrawBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isValidBlendFactor(src) || !isValidBlendFactor(dst)) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setBlendFuncSeparatei(buf, src, dst, src, dst);
}

void Context::setBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB,
                                    GLenum srcAlpha, GLenum dstAlpha) {
    if (buf >= GLStateTracker::kMaxDrawBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isValidBlendFactor(srcRGB) || !isValidBlendFactor(dstRGB) ||
        !isValidBlendFactor(srcAlpha) || !isValidBlendFactor(dstAlpha)) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void Context::setBlendEquationi(GLuint buf, GLenum mode) {
    if (buf >= GLStateTracker::kMaxDrawBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isValidBlendEquation(mode)) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setBlendEquationSeparatei(buf, mode, mode);
}

void Context::setBlendEquationSeparatei(GLuint buf, GLenum modeRGB,
                                        GLenum modeAlpha) {
    if (buf >= GLStateTracker::kMaxDrawBuffers) {
        setError(GLError::InvalidValue);
        return;
    }
    if (!isValidBlendEquation(modeRGB) || !isValidBlendEquation(modeAlpha)) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}

void Context::setClearDepth(double d) {
    state_.setClearDepth(d);
}

void Context::setClearStencil(int s) {
    state_.setClearStencil(s);
}

void Context::setPolygonOffsetClamp(float factor, float units, float clamp) {
    state_.setPolygonOffsetClamp(factor, units, clamp);
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

void Context::provokingVertex(GLenum mode) {
    if (mode != GL_FIRST_VERTEX_CONVENTION && mode != GL_LAST_VERTEX_CONVENTION) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setProvokingVertex(mode);
}

void Context::clampColor(GLenum target, GLenum mode) {
    if (target != GL_CLAMP_READ_COLOR) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (mode != GL_TRUE && mode != GL_FALSE && mode != GL_FIXED_ONLY) {
        setError(GLError::InvalidEnum);
        return;
    }
    state_.setClampColor(target, mode);
}

void Context::pointParameteri(GLenum pname, GLint param) {
    switch (pname) {
        case GL_POINT_SIZE_MIN:
        case GL_POINT_SIZE_MAX:
        case GL_POINT_FADE_THRESHOLD_SIZE:
            if (param < 0) { setError(GLError::InvalidValue); return; }
            break;
        case GL_POINT_SPRITE_COORD_ORIGIN:
            if (param != static_cast<GLint>(GL_LOWER_LEFT) &&
                param != static_cast<GLint>(GL_UPPER_LEFT)) {
                setError(GLError::InvalidEnum); return;
            }
            break;
        default:
            setError(GLError::InvalidEnum); return;
    }
    state_.setPointParameteri(pname, param);
}

void Context::pointParameterf(GLenum pname, GLfloat param) {
    switch (pname) {
        case GL_POINT_SIZE_MIN:
        case GL_POINT_SIZE_MAX:
        case GL_POINT_FADE_THRESHOLD_SIZE:
            if (param < 0.0f) { setError(GLError::InvalidValue); return; }
            break;
        case GL_POINT_SPRITE_COORD_ORIGIN:
            if (static_cast<GLenum>(static_cast<int>(param)) != GL_LOWER_LEFT &&
                static_cast<GLenum>(static_cast<int>(param)) != GL_UPPER_LEFT) {
                setError(GLError::InvalidEnum); return;
            }
            break;
        default:
            setError(GLError::InvalidEnum); return;
    }
    state_.setPointParameterf(pname, param);
}

void Context::pointParameteriv(GLenum pname, const GLint* params) {
    if (params == nullptr) return;
    pointParameteri(pname, params[0]);
}

void Context::pointParameterfv(GLenum pname, const GLfloat* params) {
    if (params == nullptr) return;
    pointParameterf(pname, params[0]);
}

void Context::patchParameteri(GLenum pname, GLint value) {
    if (pname != GL_PATCH_VERTICES) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (value <= 0) {
        setError(GLError::InvalidValue);
        return;
    }
    // MAX_PATCH_VERTICES upper bound is not enforced: the tracker does not hold
    // the limit, and overflow is forwarded honestly to the backend (which will
    // emit GL_INVALID_VALUE if unsupported). Mirrors other range checks here.
    state_.setPatchParameteri(pname, value);
}

void Context::patchParameterfv(GLenum pname, const GLfloat* values) {
    if (pname != GL_PATCH_DEFAULT_OUTER_LEVEL &&
        pname != GL_PATCH_DEFAULT_INNER_LEVEL) {
        setError(GLError::InvalidEnum);
        return;
    }
    if (values == nullptr) return;
    state_.setPatchParameterfv(pname, values);
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

void Context::memoryBarrier(uint32_t barriers) {
    backend_.memoryBarrier(barriers);
}

void Context::memoryBarrierByRegion(uint32_t barriers) {
    backend_.memoryBarrierByRegion(barriers);
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

// --- Object labels (SPEC §22.2) ---

bool Context::objectHasType(uint32_t identifier, GLObjectName name) const {
    switch (identifier) {
    case GL_BUFFER: return buffers_.count(name) != 0;
    case GL_SHADER: return shaders_.count(name) != 0;
    case GL_PROGRAM: return programs_.count(name) != 0;
    case GL_VERTEX_ARRAY: return vertexArrays_.count(name) != 0;
    case GL_QUERY: return queries_.count(name) != 0;
    case GL_PROGRAM_PIPELINE: return pipelines_.count(name) != 0;
    case GL_TRANSFORM_FEEDBACK: return transformFeedbacks_.count(name) != 0;
    case GL_SAMPLER: return samplers_.count(name) != 0;
    case GL_TEXTURE: return textures_.count(name) != 0;
    case GL_RENDERBUFFER: return renderbuffers_.count(name) != 0;
    case GL_FRAMEBUFFER: return framebuffers_.count(name) != 0;
    default: return false;
    }
}

void Context::objectLabel(uint32_t identifier, GLObjectName name, int32_t length,
                         const char* label) {
    switch (identifier) {
    case GL_BUFFER: case GL_SHADER: case GL_PROGRAM: case GL_VERTEX_ARRAY:
    case GL_QUERY: case GL_PROGRAM_PIPELINE: case GL_TRANSFORM_FEEDBACK:
    case GL_SAMPLER: case GL_TEXTURE: case GL_RENDERBUFFER: case GL_FRAMEBUFFER:
        break;
    default:
        setError(GLError::InvalidEnum);
        return;
    }
    if (!objectHasType(identifier, name)) {
        setError(GLError::InvalidOperation);
        return;
    }
    if (label == nullptr) {
        // Clearing a label is always allowed (SPEC §22.2).
        objectLabels_.erase(objectLabelKey(identifier, name));
        return;
    }
    size_t len = (length < 0) ? std::strlen(label) : static_cast<size_t>(length);
    if (len > static_cast<size_t>(kMaxObjectLabelLength)) {
        setError(GLError::InvalidValue);
        return;
    }
    objectLabels_[objectLabelKey(identifier, name)] = std::string(label, len);
}

void Context::getObjectLabel(uint32_t identifier, GLObjectName name, int32_t bufSize,
                            int32_t* length, char* label) {
    switch (identifier) {
    case GL_BUFFER: case GL_SHADER: case GL_PROGRAM: case GL_VERTEX_ARRAY:
    case GL_QUERY: case GL_PROGRAM_PIPELINE: case GL_TRANSFORM_FEEDBACK:
    case GL_SAMPLER: case GL_TEXTURE: case GL_RENDERBUFFER: case GL_FRAMEBUFFER:
        break;
    default:
        setError(GLError::InvalidEnum);
        return;
    }
    if (!objectHasType(identifier, name)) {
        setError(GLError::InvalidOperation);
        return;
    }
    auto it = objectLabels_.find(objectLabelKey(identifier, name));
    const std::string s = (it != objectLabels_.end()) ? it->second : std::string();
    if (label == nullptr) {
        // Query-only mode: length includes the nul terminator (SPEC §22.2).
        if (length) *length = static_cast<int32_t>(s.size() + 1);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (bufSize > 0) {
        int32_t copy = std::min(bufSize - 1, static_cast<int32_t>(s.size()));
        if (copy > 0) std::memcpy(label, s.data(), static_cast<size_t>(copy));
        label[copy] = '\0';
    }
    if (length) *length = static_cast<int32_t>(s.size());
}

void Context::objectPtrLabel(const void* ptr, int32_t length, const char* label) {
    if (ptr == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    if (label == nullptr) {
        ptrLabels_.erase(ptr);
        return;
    }
    size_t len = (length < 0) ? std::strlen(label) : static_cast<size_t>(length);
    if (len > static_cast<size_t>(kMaxObjectLabelLength)) {
        setError(GLError::InvalidValue);
        return;
    }
    ptrLabels_[ptr] = std::string(label, len);
}

void Context::getObjectPtrLabel(const void* ptr, int32_t bufSize, int32_t* length,
                               char* label) {
    if (ptr == nullptr) {
        setError(GLError::InvalidValue);
        return;
    }
    auto it = ptrLabels_.find(ptr);
    const std::string s = (it != ptrLabels_.end()) ? it->second : std::string();
    if (label == nullptr) {
        if (length) *length = static_cast<int32_t>(s.size() + 1);
        return;
    }
    if (bufSize < 0) {
        setError(GLError::InvalidValue);
        return;
    }
    if (bufSize > 0) {
        int32_t copy = std::min(bufSize - 1, static_cast<int32_t>(s.size()));
        if (copy > 0) std::memcpy(label, s.data(), static_cast<size_t>(copy));
        label[copy] = '\0';
    }
    if (length) *length = static_cast<int32_t>(s.size());
}

} // namespace glcompat
