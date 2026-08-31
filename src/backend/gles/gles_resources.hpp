#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/backend_resources.hpp"

#include <vector>

namespace glcompat {

// OpenGL ES has no 1D textures. Map GL_TEXTURE_1D to GL_TEXTURE_2D so the
// emulated 1D surface is stored as a 2D texture with height=1 on real backends.
// GL_TEXTURE_1D_ARRAY maps to GL_TEXTURE_2D_ARRAY (an array of 1D layers is an
// array of 1xW layers); GL_TEXTURE_RECTANGLE maps to GL_TEXTURE_2D.
inline uint32_t glesActualTarget(uint32_t target) {
    switch (target) {
    case 0x0DE0u: return GL_TEXTURE_2D;       // GL_TEXTURE_1D
    case 0x8C18u: return GL_TEXTURE_2D_ARRAY;  // GL_TEXTURE_1D_ARRAY
    case 0x84F5u: return GL_TEXTURE_2D;        // GL_TEXTURE_RECTANGLE
    default:      return target;
    }
}

// GLES 3.x requires a *sized* internal format for texture/renderbuffer storage to
// be color-/depth-renderable (and therefore FBO-complete). Desktop OpenGL accepts
// unsized formats (GL_RGBA, GL_DEPTH_COMPONENT, ...); the GLES backend promotes them
// to their sized equivalents so frontend callers (and the compatibility surface)
// need not know about GLES specifics (SPEC §2.1: backend-specific decisions behind
// the abstraction). Already-sized formats and formats with no sized mapping pass
// through unchanged.
inline uint32_t glesSizedInternalFormat(uint32_t internalFormat) {
    switch (internalFormat) {
        case 0x1908: return 0x8058; // GL_RGBA  -> GL_RGBA8
        case 0x1907: return 0x8051; // GL_RGB   -> GL_RGB8
        case 0x8227: return 0x822B; // GL_RG    -> GL_RG8
        case 0x1903: return 0x8229; // GL_RED   -> GL_R8
        case 0x1902: return 0x81A5; // GL_DEPTH_COMPONENT -> GL_DEPTH_COMPONENT16
        case 0x1906: return 0x8058; // GL_ALPHA -> GL_RGBA8 (no ALPHA in GLES3)
        case 0x1909: return 0x8229; // GL_LUMINANCE -> GL_R8
        case 0x190A: return 0x822B; // GL_LUMINANCE_ALPHA -> GL_RG8
        default:     return internalFormat;
    }
}

// Backend resource handles wrapping a real GLES object name. Deletion goes
// through the loader so the GLES object is freed when the frontend releases
// the (opaque) frontend object. The loader is held by shared_ptr so handles
// stay valid even if the backend is torn down after the resources.
struct GLESBackendBuffer : BackendBuffer {
    GLESBackendBuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendBuffer() override {
        if (lib && lib->driverLive() && lib->glDeleteBuffers) lib->glDeleteBuffers(1, &handle);
    }
    void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                     const void* data) override {
        if (lib && lib->driverLive() && lib->glBufferData) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glBufferData(target, size, data, usage);
        }
    }
    void bufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                       const void* data) override {
        if (lib && lib->driverLive() && lib->glBufferSubData) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glBufferSubData(target, offset, size, data);
        }
    }
    void bufferStorage(uint32_t target, intptr_t size, uint32_t flags,
                       const void* data) override {
        if (lib && lib->driverLive() && lib->glBufferStorage) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glBufferStorage(target, size, data,
                                 static_cast<GLenum>(flags));
        }
    }
    void namedBufferData(intptr_t size, uint32_t usage, const void* data) override {
        if (lib && lib->driverLive() && lib->glNamedBufferData) {
            lib->glNamedBufferData(handle, size, data, usage);
        }
    }
    void namedBufferSubData(intptr_t offset, intptr_t size, const void* data) override {
        if (lib && lib->driverLive() && lib->glNamedBufferSubData) {
            lib->glNamedBufferSubData(handle, offset, size, data);
        }
    }
    void namedBufferStorage(intptr_t size, uint32_t flags, const void* data) override {
        if (lib && lib->driverLive() && lib->glNamedBufferStorage) {
            lib->glNamedBufferStorage(handle, size, data,
                                      static_cast<GLbitfield>(flags));
        }
    }
    void copySubData(uint32_t readTarget, uint32_t writeTarget,
                     intptr_t readOffset, intptr_t writeOffset,
                     intptr_t size) override {
        if (lib && lib->driverLive() && lib->glCopyBufferSubData) {
            if (lib->glBindBuffer) {
                lib->glBindBuffer(readTarget, 0);
                lib->glBindBuffer(writeTarget, 0);
            }
            lib->glCopyBufferSubData(readTarget, writeTarget, readOffset,
                                     writeOffset, size);
        }
    }
    void* mapBufferRange(uint32_t target, intptr_t offset, intptr_t length,
                         uint32_t access) override {
        if (lib && lib->driverLive() && lib->glMapBufferRange) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            return lib->glMapBufferRange(target, offset, length,
                                         static_cast<GLbitfield>(access));
        }
        return nullptr;
    }
    void unmapBuffer(uint32_t target) override {
        if (lib && lib->driverLive() && lib->glUnmapBuffer) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glUnmapBuffer(target);
        }
    }
    void flushMappedBufferRange(uint32_t target, intptr_t offset,
                                intptr_t length) override {
        if (lib && lib->driverLive() && lib->glFlushMappedBufferRange) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glFlushMappedBufferRange(target, offset, length);
        }
    }
    void* mapNamedBufferRange(intptr_t offset, intptr_t length,
                              uint32_t access) override {
        if (lib && lib->driverLive() && lib->glMapNamedBufferRange) {
            return lib->glMapNamedBufferRange(handle, offset, length,
                                              static_cast<GLbitfield>(access));
        }
        return nullptr;
    }
    void unmapNamedBuffer() override {
        if (lib && lib->driverLive() && lib->glUnmapNamedBuffer) {
            lib->glUnmapNamedBuffer(handle);
        }
    }
    void flushMappedNamedBufferRange(intptr_t offset, intptr_t length) override {
        if (lib && lib->driverLive() && lib->glFlushMappedNamedBufferRange) {
            lib->glFlushMappedNamedBufferRange(handle, offset, length);
        }
    }
    void invalidateBufferData(uint32_t target) override {
        (void)target;
        if (lib && lib->driverLive() && lib->glInvalidateBufferData) {
            lib->glInvalidateBufferData(handle);
        }
    }
    void invalidateBufferSubData(uint32_t target, intptr_t offset,
                                intptr_t length) override {
        (void)target;
        if (lib && lib->driverLive() && lib->glInvalidateBufferSubData) {
            lib->glInvalidateBufferSubData(handle, offset, length);
        }
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendTexture : BackendTexture {
    GLESBackendTexture(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendTexture() override {
        if (lib && lib->driverLive() && lib->glDeleteTextures) lib->glDeleteTextures(1, &handle);
    }
    void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                     int width, int height, uint32_t format, uint32_t type,
                     const void* data) override {
        // GL_TEXTURE_1D_ARRAY has no GLES equivalent; emulate it as a 2D array
        // where each 1D layer is a 1xW slice (the height argument is the layer
        // count). The driver allocates it through the 3D entry point.
        if (target == 0x8C18u) { // GL_TEXTURE_1D_ARRAY
            if (!lib || !lib->driverLive() || !lib->glTexImage3D) return;
            if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
            lib->glTexImage3D(GL_TEXTURE_2D_ARRAY, level,
                              static_cast<GLint>(glesSizedInternalFormat(internalFormat)),
                              static_cast<GLsizei>(width), 1,
                              static_cast<GLsizei>(height), 0,
                              format, type, data);
            return;
        }
        if (!lib || !lib->driverLive() || !lib->glTexImage2D) return;
        const uint32_t t = glesActualTarget(target);
        if (lib->glBindTexture) lib->glBindTexture(t, handle);
        lib->glTexImage2D(t, level,
                          static_cast<GLint>(glesSizedInternalFormat(internalFormat)),
                          static_cast<GLsizei>(width),
                          static_cast<GLsizei>(height), 0,
                          format, type, data);
    }
    void texImage1D(uint32_t target, int level, uint32_t internalFormat,
                    int width, uint32_t format, uint32_t type,
                    const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glTexImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D, handle);
        lib->glTexImage2D(GL_TEXTURE_2D, level,
                          static_cast<GLint>(glesSizedInternalFormat(internalFormat)),
                          static_cast<GLsizei>(width), 1, 0,
                          format, type, data);
    }
    void texImage3D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, int depth, uint32_t format,
                    uint32_t type, const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glTexImage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexImage3D(target, level,
                          static_cast<GLint>(glesSizedInternalFormat(internalFormat)),
                          static_cast<GLsizei>(width),
                          static_cast<GLsizei>(height),
                          static_cast<GLsizei>(depth), 0,
                          format, type, data);
    }
    void texParameteri(uint32_t target, uint32_t pname, int param) override {
        if (!lib || !lib->driverLive() || !lib->glTexParameteri) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glTexParameteri(glesActualTarget(target), pname, param);
    }
    void texParameterf(uint32_t target, uint32_t pname, float param) override {
        if (!lib || !lib->driverLive() || !lib->glTexParameterf) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glTexParameterf(glesActualTarget(target), pname, param);
    }
    void texParameterfv(uint32_t target, uint32_t pname, const float* params,
                        int count) override {
        if (!lib || !lib->driverLive() || !lib->glTexParameterfv || !params) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glTexParameterfv(glesActualTarget(target), pname, params, count);
    }
    void texParameteriv(uint32_t target, uint32_t pname, const int* params,
                        int count) override {
        if (!lib || !lib->driverLive() || !lib->glTexParameteriv || !params) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glTexParameteriv(glesActualTarget(target), pname, params, count);
    }
    void texParameterIiv(uint32_t target, uint32_t pname, const int32_t* params,
                        int count) override {
        if (!lib || !lib->driverLive() || !lib->glTexParameterIiv || !params) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glTexParameterIiv(glesActualTarget(target), pname, params, count);
    }
    void texParameterIuiv(uint32_t target, uint32_t pname, const uint32_t* params,
                         int count) override {
        if (!lib || !lib->driverLive() || !lib->glTexParameterIuiv || !params) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glTexParameterIuiv(glesActualTarget(target), pname, params, count);
    }
    void invalidateTexImage(uint32_t target, int level) override {
        if (!lib || !lib->driverLive() || !lib->glInvalidateTexImage) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glInvalidateTexImage(glesActualTarget(target), level);
    }
    void invalidateTexSubImage(uint32_t target, int level, int xoffset, int yoffset,
                            int zoffset, int width, int height, int depth) override {
        if (!lib || !lib->driverLive() || !lib->glInvalidateTexSubImage) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glInvalidateTexSubImage(glesActualTarget(target), level, xoffset, yoffset,
                                   zoffset, width, height, depth);
    }
    void texSubImage1D(uint32_t target, int level, int xoffset, int width,
                       uint32_t format, uint32_t type, const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glTexSubImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D, handle);
        lib->glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, width, 1,
                             format, type, data);
    }
    void texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                        int width, int height, uint32_t format, uint32_t type,
                        const void* data) override {
        // GL_TEXTURE_1D_ARRAY sub-uploads emulate as 2D-array sub-uploads; the
        // height argument is the layer count and the y offset stays 0.
        if (target == 0x8C18u) { // GL_TEXTURE_1D_ARRAY
            if (!lib || !lib->driverLive() || !lib->glTexSubImage3D) return;
            if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
            lib->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, 0, 0,
                                 width, 1, height, format, type, data);
            return;
        }
        if (!lib || !lib->driverLive() || !lib->glTexSubImage2D) return;
        const uint32_t t = glesActualTarget(target);
        if (lib->glBindTexture) lib->glBindTexture(t, handle);
        lib->glTexSubImage2D(t, level, xoffset, yoffset, width, height,
                             format, type, data);
    }
    void texSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                       int zoffset, int width, int height, int depth,
                       uint32_t format, uint32_t type, const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glTexSubImage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width,
                             height, depth, format, type, data);
    }
    void copyTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                        int x, int y, int width, int border) override {
        if (!lib || !lib->driverLive() || !lib->glCopyTexImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D, handle);
        lib->glCopyTexImage2D(GL_TEXTURE_2D, level,
                              glesSizedInternalFormat(internalFormat),
                              x, y, width, 1, border);
    }
    void copyTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                         int x, int y, int width, int height, int border) override {
        if (!lib || !lib->driverLive() || !lib->glCopyTexImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCopyTexImage2D(target, level, glesSizedInternalFormat(internalFormat),
                              x, y, width, height, border);
    }
    void copyTexSubImage1D(uint32_t target, int level, int xoffset, int x, int y,
                           int width) override {
        if (!lib || !lib->driverLive() || !lib->glCopyTexSubImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D, handle);
        lib->glCopyTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, x, y, width, 1);
    }
    void copyTexSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                           int x, int y, int width, int height) override {
        if (!lib || !lib->driverLive() || !lib->glCopyTexSubImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width,
                                height);
    }
    void copyTexSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                           int zoffset, int x, int y, int width,
                           int height) override {
        if (!lib || !lib->driverLive() || !lib->glCopyTexSubImage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y,
                                width, height);
    }
    void compressedTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                             int width, int border, int imageSize,
                             const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glCompressedTexImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D, handle);
        lib->glCompressedTexImage2D(GL_TEXTURE_2D, level,
                                    internalFormat, static_cast<GLsizei>(width), 1,
                                    border, static_cast<GLsizei>(imageSize), data);
    }
    void compressedTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                             int width, int height, int border, int imageSize,
                             const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glCompressedTexImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCompressedTexImage2D(target, level, internalFormat,
                                    static_cast<GLsizei>(width),
                                    static_cast<GLsizei>(height), border,
                                    static_cast<GLsizei>(imageSize), data);
    }
    void compressedTexImage3D(uint32_t target, int level, uint32_t internalFormat,
                             int width, int height, int depth, int border,
                             int imageSize, const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glCompressedTexImage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCompressedTexImage3D(target, level, internalFormat,
                                    static_cast<GLsizei>(width),
                                    static_cast<GLsizei>(height),
                                    static_cast<GLsizei>(depth), border,
                                    static_cast<GLsizei>(imageSize), data);
    }
    void compressedTexSubImage1D(uint32_t target, int level, int xoffset, int width,
                                uint32_t format, int imageSize,
                                const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glCompressedTexSubImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(GL_TEXTURE_2D, handle);
        lib->glCompressedTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, width, 1,
                                       format, static_cast<GLsizei>(imageSize), data);
    }
    void compressedTexSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                                int width, int height, uint32_t format, int imageSize,
                                const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glCompressedTexSubImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height,
                                       format, static_cast<GLsizei>(imageSize), data);
    }
    void compressedTexSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                                int zoffset, int width, int height, int depth,
                                uint32_t format, int imageSize,
                                const void* data) override {
        if (!lib || !lib->driverLive() || !lib->glCompressedTexSubImage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset,
                                       width, height, depth, format,
                                       static_cast<GLsizei>(imageSize), data);
    }
    void storage1D(uint32_t target, int levels, uint32_t internalFormat,
                   int width) override {
        // OpenGL ES has no 1D textures; the call is a no-op on this backend.
        if (!lib || !lib->driverLive() || !lib->glTexStorage1D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexStorage1D(target, static_cast<GLsizei>(levels),
                            glesSizedInternalFormat(internalFormat),
                            static_cast<GLsizei>(width));
    }
    void storage2D(uint32_t target, int levels, uint32_t internalFormat,
                   int width, int height) override {
        if (!lib || !lib->driverLive() || !lib->glTexStorage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexStorage2D(target, static_cast<GLsizei>(levels),
                            glesSizedInternalFormat(internalFormat),
                            static_cast<GLsizei>(width),
                            static_cast<GLsizei>(height));
    }
    void storage3D(uint32_t target, int levels, uint32_t internalFormat,
                   int width, int height, int depth) override {
        if (!lib || !lib->driverLive() || !lib->glTexStorage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexStorage3D(target, static_cast<GLsizei>(levels),
                            glesSizedInternalFormat(internalFormat),
                            static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                            static_cast<GLsizei>(depth));
    }
    void generateMipmap(uint32_t target) override {
        if (!lib || !lib->driverLive() || !lib->glGenerateMipmap) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glGenerateMipmap(target);
    }
    void textureBuffer(uint32_t target, uint32_t internalFormat,
                       uint32_t bufferNativeId) override {
        if (!lib || !lib->driverLive() || !lib->glTexBuffer) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexBuffer(target, glesSizedInternalFormat(internalFormat),
                        bufferNativeId);
    }
    void textureBufferRange(uint32_t target, uint32_t internalFormat,
                             uint32_t bufferNativeId, intptr_t offset,
                             intptr_t size) override {
        if (!lib || !lib->driverLive() || !lib->glTexBufferRange) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexBufferRange(target, glesSizedInternalFormat(internalFormat),
                              bufferNativeId, offset, size);
    }
    void view(uint32_t target, uint32_t origTextureNativeId, uint32_t internalFormat,
              uint32_t minLevel, uint32_t numLevels, uint32_t minLayer,
              uint32_t numLayers) override {
        if (!lib || !lib->driverLive() || !lib->glTextureView) return;
        lib->glTextureView(handle, target, origTextureNativeId,
                           glesSizedInternalFormat(internalFormat), minLevel,
                           numLevels, minLayer, numLayers);
    }
    void storage2DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                              int width, int height, bool fixedSampleLocations) override {
        if (!lib || !lib->driverLive() || !lib->glTexStorage2DMultisample) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexStorage2DMultisample(target, static_cast<GLsizei>(samples),
                                       glesSizedInternalFormat(internalFormat),
                                       static_cast<GLsizei>(width),
                                       static_cast<GLsizei>(height),
                                       fixedSampleLocations ? GL_TRUE : GL_FALSE);
    }
    void storage3DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                              int width, int height, int depth,
                              bool fixedSampleLocations) override {
        if (!lib || !lib->driverLive() || !lib->glTexStorage3DMultisample) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexStorage3DMultisample(target, static_cast<GLsizei>(samples),
                                       glesSizedInternalFormat(internalFormat),
                                       static_cast<GLsizei>(width),
                                       static_cast<GLsizei>(height),
                                       static_cast<GLsizei>(depth),
                                       fixedSampleLocations ? GL_TRUE : GL_FALSE);
    }
    void texImage2DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                               int width, int height, bool fixedSampleLocations) override {
        if (!lib || !lib->driverLive() || !lib->glTexImage2DMultisample) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexImage2DMultisample(target, static_cast<GLsizei>(samples),
                                     glesSizedInternalFormat(internalFormat),
                                     static_cast<GLsizei>(width),
                                     static_cast<GLsizei>(height),
                                     fixedSampleLocations ? GL_TRUE : GL_FALSE);
    }
    void texImage3DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                               int width, int height, int depth,
                               bool fixedSampleLocations) override {
        if (!lib || !lib->driverLive() || !lib->glTexImage3DMultisample) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexImage3DMultisample(target, static_cast<GLsizei>(samples),
                                     glesSizedInternalFormat(internalFormat),
                                     static_cast<GLsizei>(width),
                                     static_cast<GLsizei>(height),
                                     static_cast<GLsizei>(depth),
                                     fixedSampleLocations ? GL_TRUE : GL_FALSE);
    }
    void getLevelParameteriv(uint32_t target, int level, uint32_t pname,
                             int32_t* params) override {
        if (!lib || !lib->driverLive() || !lib->glGetTexLevelParameteriv || !params)
            return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glGetTexLevelParameteriv(glesActualTarget(target), level, pname, params);
    }
    void getLevelParameterfv(uint32_t target, int level, uint32_t pname,
                             float* params) override {
        if (!lib || !lib->driverLive() || !lib->glGetTexLevelParameterfv || !params)
            return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glGetTexLevelParameterfv(glesActualTarget(target), level, pname, params);
    }
    void getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                      void* pixels) override {
        if (!lib || !lib->driverLive() || !lib->glGetTexImage) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        lib->glGetTexImage(glesActualTarget(target), level, format, type, pixels);
    }
    void getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                     int bufSize, void* pixels) override {
        if (!lib || !lib->driverLive()) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        if (lib->glGetnTexImage)
            lib->glGetnTexImage(glesActualTarget(target), level, format, type, bufSize,
                                pixels);
        else if (lib->glGetTexImage)
            lib->glGetTexImage(glesActualTarget(target), level, format, type, pixels);
    }
    void getCompressedTexImage(uint32_t target, int level, int bufSize,
                               void* pixels) override {
        if (!lib || !lib->driverLive()) return;
        if (lib->glBindTexture) lib->glBindTexture(glesActualTarget(target), handle);
        if (lib->glGetnCompressedTexImage)
            lib->glGetnCompressedTexImage(glesActualTarget(target), level, bufSize,
                                          pixels);
        else if (lib->glGetCompressedTexImage)
            lib->glGetCompressedTexImage(glesActualTarget(target), level, pixels);
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendRenderbuffer : BackendRenderbuffer {
    GLESBackendRenderbuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendRenderbuffer() override {
        if (lib && lib->driverLive() && lib->glDeleteRenderbuffers)
            lib->glDeleteRenderbuffers(1, &handle);
    }
    void renderbufferStorage(uint32_t target, uint32_t internalFormat, int width,
                             int height) override {
        if (!lib || !lib->driverLive() || !lib->glRenderbufferStorage) return;
        // The renderbuffer must be bound to the target before storage is set.
        if (lib->glBindRenderbuffer) lib->glBindRenderbuffer(target, handle);
        lib->glRenderbufferStorage(target, static_cast<GLenum>(internalFormat),
                                  static_cast<GLsizei>(width),
                                  static_cast<GLsizei>(height));
    }
    void renderbufferStorageMultisample(uint32_t target, int samples,
                                       uint32_t internalFormat, int width,
                                       int height) override {
        if (!lib || !lib->driverLive() || !lib->glRenderbufferStorageMultisample)
            return;
        if (lib->glBindRenderbuffer) lib->glBindRenderbuffer(target, handle);
        lib->glRenderbufferStorageMultisample(
            target, static_cast<GLsizei>(samples),
            static_cast<GLenum>(internalFormat), static_cast<GLsizei>(width),
            static_cast<GLsizei>(height));
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendFramebuffer : BackendFramebuffer {
    GLESBackendFramebuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendFramebuffer() override {
        if (lib && lib->driverLive() && lib->glDeleteFramebuffers)
            lib->glDeleteFramebuffers(1, &handle);
    }
    void framebufferTexture2D(uint32_t target, uint32_t attachment,
                              uint32_t texTarget, uint32_t nativeTexture,
                              int level) override {
        if (lib && lib->driverLive() && lib->glFramebufferTexture2D)
            lib->glFramebufferTexture2D(target, attachment, texTarget, nativeTexture,
                                      level);
    }
    void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                 uint32_t rbTarget,
                                 uint32_t nativeRenderbuffer) override {
        if (lib && lib->driverLive() && lib->glFramebufferRenderbuffer)
            lib->glFramebufferRenderbuffer(target, attachment, rbTarget,
                                          nativeRenderbuffer);
    }
    void framebufferTextureLayer(uint32_t target, uint32_t attachment,
                                uint32_t nativeTexture, int level,
                                int layer) override {
        if (lib && lib->driverLive() && lib->glFramebufferTextureLayer)
            lib->glFramebufferTextureLayer(target, attachment, nativeTexture, level,
                                          layer);
    }
    void framebufferParameteri(uint32_t target, uint32_t pname,
                              int param) override {
        if (lib && lib->driverLive() && lib->glFramebufferParameteri)
            lib->glFramebufferParameteri(target, pname, param);
    }
    uint32_t checkStatus(uint32_t target) const override {
        if (lib && lib->driverLive() && lib->glCheckFramebufferStatus)
            return lib->glCheckFramebufferStatus(target);
        return 0x8CD5; // GL_FRAMEBUFFER_COMPLETE
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendVertexArray : BackendVertexArray {
    GLESBackendVertexArray(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendVertexArray() override {
        if (lib && lib->driverLive() && lib->glDeleteVertexArrays)
            lib->glDeleteVertexArrays(1, &handle);
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

// Real GLES sampler object (SPEC §8.2). Created lazily at construction; the
// scalar parameters are driven through the loader.
struct GLESBackendSampler : BackendSampler {
    GLESBackendSampler(GLESLibPtr lib) : lib(lib) {
        if (lib && lib->driverLive() && lib->glGenSamplers)
            lib->glGenSamplers(1, &handle);
    }
    ~GLESBackendSampler() override {
        if (lib && lib->driverLive() && lib->glDeleteSamplers && handle)
            lib->glDeleteSamplers(1, &handle);
    }
    void samplerParameteri(uint32_t pname, int param) override {
        if (lib && lib->driverLive() && lib->glSamplerParameteri && handle)
            lib->glSamplerParameteri(handle, pname, param);
    }
    void samplerParameterf(uint32_t pname, float param) override {
        if (lib && lib->driverLive() && lib->glSamplerParameterf && handle)
            lib->glSamplerParameterf(handle, pname, param);
    }
    void samplerParameterfv(uint32_t pname, const float* params, int count) override {
        if (lib && lib->driverLive() && lib->glSamplerParameterfv && handle && params && count > 0)
            lib->glSamplerParameterfv(handle, pname, params);
    }
    void samplerParameteriv(uint32_t pname, const int32_t* params, int count) override {
        if (lib && lib->driverLive() && lib->glSamplerParameteriv && handle && params && count > 0)
            lib->glSamplerParameteriv(handle, pname, params);
    }
    void samplerParameterIiv(uint32_t pname, const int32_t* params) override {
        if (lib && lib->driverLive() && lib->glSamplerParameterIiv && handle && params)
            lib->glSamplerParameterIiv(handle, pname, params);
    }
    void samplerParameterIuiv(uint32_t pname, const uint32_t* params) override {
        if (lib && lib->driverLive() && lib->glSamplerParameterIuiv && handle && params)
            lib->glSamplerParameterIuiv(handle, pname, params);
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

// Real GLES transform-feedback object (SPEC §13.3). The native TF object is
// created lazily at construction; capture state is driven through the loader.
struct GLESBackendTransformFeedback : BackendTransformFeedback {
    GLESBackendTransformFeedback(GLESLibPtr lib) : lib(lib) {
        if (lib && lib->driverLive() && lib->glGenTransformFeedbacks)
            lib->glGenTransformFeedbacks(1, &handle);
    }
    ~GLESBackendTransformFeedback() override {
        if (lib && lib->driverLive() && lib->glDeleteTransformFeedbacks && handle)
            lib->glDeleteTransformFeedbacks(1, &handle);
    }
    void begin(uint32_t mode) override {
        if (lib && lib->driverLive() && lib->glBeginTransformFeedback)
            lib->glBeginTransformFeedback(mode);
    }
    void end() override {
        if (lib && lib->driverLive() && lib->glEndTransformFeedback)
            lib->glEndTransformFeedback();
    }
    void pause() override {
        if (lib && lib->driverLive() && lib->glPauseTransformFeedback)
            lib->glPauseTransformFeedback();
    }
    void resume() override {
        if (lib && lib->driverLive() && lib->glResumeTransformFeedback)
            lib->glResumeTransformFeedback();
    }
    uint32_t nativeHandle() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

// Real GLES query object (SPEC §4 / §19). The native query is generated lazily
// at construction; begin/end drive the driver and queryResult reads the counter
// (via ui64v when available, falling back to uiv).
struct GLESBackendQuery : BackendQuery {
    GLESBackendQuery(GLESLibPtr lib) : lib(lib) {
        if (lib && lib->driverLive() && lib->glGenQueries)
            lib->glGenQueries(1, &handle);
    }
    ~GLESBackendQuery() override {
        if (lib && lib->driverLive() && lib->glDeleteQueries && handle)
            lib->glDeleteQueries(1, &handle);
    }
    void begin(uint32_t target) override {
        activeTarget = target;
        if (lib && lib->driverLive() && lib->glBeginQuery && handle)
            lib->glBeginQuery(target, handle);
    }
    void end() override {
        if (lib && lib->driverLive() && lib->glEndQuery)
            lib->glEndQuery(activeTarget); // target must match begin
    }
    void queryCounter(uint32_t target) override {
        if (lib && lib->driverLive() && lib->glQueryCounter && handle)
            lib->glQueryCounter(handle, target);
    }
    void queryResult(int64_t* value, bool* available) override {
        *value = 0;
        *available = false;
        if (!lib || !lib->driverLive() || handle == 0) return;
        if (lib->glGetQueryObjectui64v) {
            GLuint64 v = 0;
            lib->glGetQueryObjectui64v(handle, GL_QUERY_RESULT_AVAILABLE, &v);
            *available = (v != 0);
            lib->glGetQueryObjectui64v(handle, GL_QUERY_RESULT, &v);
            *value = static_cast<int64_t>(v);
        } else if (lib->glGetQueryObjectuiv) {
            GLuint v = 0;
            lib->glGetQueryObjectuiv(handle, GL_QUERY_RESULT_AVAILABLE, &v);
            *available = (v != 0);
            lib->glGetQueryObjectuiv(handle, GL_QUERY_RESULT, &v);
            *value = static_cast<int64_t>(v);
        }
    }
    GLESLibPtr lib;
    GLuint handle = 0;
    uint32_t activeTarget = 0;
};

// Real GLES shader object. Created at compile time and kept alive until the
// frontend releases the owning shader object.
struct GLESBackendShader : BackendShader {
    GLESBackendShader(GLESLibPtr lib, GLenum stage) : lib(lib) {
        if (lib && lib->driverLive() && lib->glCreateShader)
            handle = lib->glCreateShader(stage);
    }
    ~GLESBackendShader() override {
        if (lib && lib->driverLive() && lib->glDeleteShader && handle)
            lib->glDeleteShader(handle);
    }
    bool compile(const std::string& source, std::string& log) override {
        if (!lib || !lib->driverLive() || handle == 0) {
            log = "GLES shader not created";
            return false;
        }
        const char* src = source.c_str();
        GLint len = static_cast<GLint>(source.size());
        lib->glShaderSource(handle, 1, &src, &len);
        lib->glCompileShader(handle);
        GLint ok = 0;
        lib->glGetShaderiv(handle, GL_COMPILE_STATUS, &ok);
        if (ok == 0) {
            GLint logLen = 0;
            lib->glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> buf(logLen > 0 ? logLen : 1, 0);
            lib->glGetShaderInfoLog(handle, logLen, nullptr, buf.data());
            log = std::string(buf.data());
            return false;
        }
        log.clear();
        return true;
    }
    GLESLibPtr lib;
    GLuint handle = 0;
};

// Real GLES program object. Links attached shaders and exposes attribute
// locations through the driver.
struct GLESBackendProgram : BackendProgram {
    GLESBackendProgram(GLESLibPtr lib) : lib(lib) {
        if (lib && lib->driverLive() && lib->glCreateProgram)
            handle = lib->glCreateProgram();
    }
    ~GLESBackendProgram() override {
        if (lib && lib->driverLive() && lib->glDeleteProgram && handle)
            lib->glDeleteProgram(handle);
    }
    void attach(BackendShader& shader) override {
        if (auto* gs = dynamic_cast<GLESBackendShader*>(&shader))
            lib->glAttachShader(handle, gs->handle);
    }
    void detach(BackendShader& shader) override {
        if (auto* gs = dynamic_cast<GLESBackendShader*>(&shader))
            lib->glDetachShader(handle, gs->handle);
    }
    bool link(std::string& log) override {
        if (!lib || !lib->driverLive() || handle == 0) {
            log = "GLES program not created";
            return false;
        }
        lib->glLinkProgram(handle);
        GLint ok = 0;
        lib->glGetProgramiv(handle, GL_LINK_STATUS, &ok);
        if (ok == 0) {
            GLint logLen = 0;
            lib->glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> buf(logLen > 0 ? logLen : 1, 0);
            lib->glGetProgramInfoLog(handle, logLen, nullptr, buf.data());
            log = std::string(buf.data());
            return false;
        }
        log.clear();
        return true;
    }
    int getAttribLocation(const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0) return -1;
        return static_cast<int>(lib->glGetAttribLocation(handle, name.c_str()));
    }
    void bindAttribLocation(const std::string& name, int index) override {
        if (lib && lib->driverLive() && handle != 0 && lib->glBindAttribLocation)
            lib->glBindAttribLocation(handle, static_cast<GLuint>(index),
                                     name.c_str());
    }
    // Fragment-output location binding (SPEC §7.3.7 / §15.1.2). Core GLES has no
    // direct equivalent (only GL_EXT_blend_func_extended, which is optional and
    // not wired here), so this is an honest no-op; getFragDataLocation likewise
    // returns -1 for GLES.
    void bindFragDataLocation(const std::string& name, int colorNumber,
                              int index) override {
        (void)name;
        (void)colorNumber;
        (void)index;
    }
    void uniformBlockBinding(uint32_t blockIndex, uint32_t blockBinding) override {
        if (lib && lib->driverLive() && handle != 0 && lib->glUniformBlockBinding)
            lib->glUniformBlockBinding(handle, blockIndex, blockBinding);
    }
    void shaderStorageBlockBinding(uint32_t blockIndex, uint32_t blockBinding) override {
        if (lib && lib->driverLive() && handle != 0 && lib->glShaderStorageBlockBinding)
            lib->glShaderStorageBlockBinding(handle, blockIndex, blockBinding);
    }
    void transformFeedbackVaryings(const std::vector<std::string>& varyings,
                                   uint32_t bufferMode) override {
        if (lib && lib->driverLive() && handle != 0 && lib->glTransformFeedbackVaryings) {
            std::vector<const char*> names(varyings.size());
            for (size_t i = 0; i < varyings.size(); ++i) names[i] = varyings[i].c_str();
            lib->glTransformFeedbackVaryings(handle, static_cast<GLsizei>(names.size()),
                                            names.data(),
                                            static_cast<GLenum>(bufferMode));
        }
    }
    uint32_t nativeId() const override { return handle; }

    int getUniformLocation(const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0) return -1;
        return static_cast<int>(lib->glGetUniformLocation(handle, name.c_str()));
    }
    // Program-interface reflection (SPEC §7.3.11). These call into the
    // ES 3.0+ driver entry points where resolved; otherwise they fall back to
    // the honest BackendProgram defaults (name not found / -1 / 0).
    uint32_t programResourceCount(uint32_t programInterface) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramInterfaceiv)
            return 0;
        GLint count = 0;
        lib->glGetProgramInterfaceiv(handle, static_cast<GLenum>(programInterface),
                                    GL_ACTIVE_RESOURCES, &count);
        return static_cast<uint32_t>(count);
    }
    uint32_t getProgramResourceIndex(uint32_t programInterface,
                                     const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramResourceIndex)
            return 0xFFFFFFFFu;  // GL_INVALID_INDEX
        return lib->glGetProgramResourceIndex(
            handle, static_cast<GLenum>(programInterface), name.c_str());
    }
    void getProgramResourceName(uint32_t programInterface, uint32_t index,
                                int32_t bufSize, int32_t* length,
                                char* name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramResourceName || bufSize <= 0 || name == nullptr)
            return;
        lib->glGetProgramResourceName(handle,
                                      static_cast<GLenum>(programInterface),
                                      index, bufSize, length, name);
    }
    void getProgramResourceiv(uint32_t programInterface, uint32_t index,
                              int32_t propCount, const uint32_t* props,
                              int32_t bufSize, int32_t* length,
                              int32_t* params) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramResourceiv || propCount <= 0 || props == nullptr ||
            bufSize <= 0 || params == nullptr)
            return;
        lib->glGetProgramResourceiv(handle, static_cast<GLenum>(programInterface),
                                    index, propCount,
                                    reinterpret_cast<const GLenum*>(props),
                                    bufSize, length, params);
    }
    int32_t getProgramResourceLocation(uint32_t programInterface,
                                       const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramResourceLocation)
            return -1;
        return lib->glGetProgramResourceLocation(
            handle, static_cast<GLenum>(programInterface), name.c_str());
    }
    int32_t getProgramResourceLocationIndex(uint32_t programInterface,
                                            const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramResourceLocationIndex)
            return -1;
        return lib->glGetProgramResourceLocationIndex(
            handle, static_cast<GLenum>(programInterface), name.c_str());
    }
    void getProgramInterfaceiv(uint32_t programInterface, uint32_t pname,
                               int32_t* params) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramInterfaceiv || params == nullptr)
            return;
        lib->glGetProgramInterfaceiv(handle, static_cast<GLenum>(programInterface),
                                     static_cast<GLenum>(pname), params);
    }
    // Subroutine reflection + selection (SPEC §7.9). ES 3.1+ driver entry points,
    // resolved optionally; otherwise the honest BackendProgram defaults apply.
    uint32_t getSubroutineIndex(uint32_t shadertype,
                                const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0 || !lib->glGetSubroutineIndex)
            return 0xFFFFFFFFu;  // GL_INVALID_INDEX
        return lib->glGetSubroutineIndex(handle, static_cast<GLenum>(shadertype),
                                         name.c_str());
    }
    int32_t getSubroutineUniformLocation(uint32_t shadertype,
                                         const std::string& name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetSubroutineUniformLocation)
            return -1;
        return lib->glGetSubroutineUniformLocation(
            handle, static_cast<GLenum>(shadertype), name.c_str());
    }
    void getActiveSubroutineUniformiv(uint32_t shadertype, uint32_t index,
                                     uint32_t pname, int32_t* values) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetActiveSubroutineUniformiv || values == nullptr)
            return;
        lib->glGetActiveSubroutineUniformiv(handle,
                                            static_cast<GLenum>(shadertype), index,
                                            static_cast<GLenum>(pname), values);
    }
    void getActiveSubroutineUniformName(uint32_t shadertype, uint32_t index,
                                       int32_t bufSize, int32_t* length,
                                       char* name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetActiveSubroutineUniformName || bufSize <= 0 || name == nullptr)
            return;
        lib->glGetActiveSubroutineUniformName(handle,
                                              static_cast<GLenum>(shadertype),
                                              index, bufSize, length, name);
    }
    void getActiveSubroutineName(uint32_t shadertype, uint32_t index,
                                int32_t bufSize, int32_t* length,
                                char* name) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetActiveSubroutineName || bufSize <= 0 || name == nullptr)
            return;
        lib->glGetActiveSubroutineName(handle, static_cast<GLenum>(shadertype),
                                      index, bufSize, length, name);
    }
    void uniformSubroutinesuiv(uint32_t shadertype, int32_t count,
                              const uint32_t* indices) override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glUniformSubroutinesuiv || count <= 0 || indices == nullptr)
            return;
        lib->glUniformSubroutinesuiv(static_cast<GLenum>(shadertype), count,
                                    indices);
    }
    void getUniformSubroutineuiv(uint32_t shadertype, int32_t location,
                                 uint32_t* params) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetUniformSubroutineuiv || params == nullptr)
            return;
        lib->glGetUniformSubroutineuiv(static_cast<GLenum>(shadertype), location,
                                      params);
    }
    void getProgramStageiv(uint32_t shadertype, uint32_t pname,
                           int32_t* values) const override {
        if (!lib || !lib->driverLive() || handle == 0 ||
            !lib->glGetProgramStageiv || values == nullptr)
            return;
        lib->glGetProgramStageiv(handle, static_cast<GLenum>(shadertype),
                                 static_cast<GLenum>(pname), values);
    }
    void uniform1f(int loc, float v0) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform1f(loc, v0);
    }
    void uniform2f(int loc, float v0, float v1) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform2f(loc, v0, v1);
    }
    void uniform3f(int loc, float v0, float v1, float v2) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform3f(loc, v0, v1, v2);
    }
    void uniform4f(int loc, float v0, float v1, float v2, float v3) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform4f(loc, v0, v1, v2, v3);
    }
    void uniform1i(int loc, int v0) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform1i(loc, v0);
    }
    void uniform2i(int loc, int v0, int v1) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform2i(loc, v0, v1);
    }
    void uniform3i(int loc, int v0, int v1, int v2) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform3i(loc, v0, v1, v2);
    }
    void uniform4i(int loc, int v0, int v1, int v2, int v3) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0) return;
        bind();
        lib->glUniform4i(loc, v0, v1, v2, v3);
    }
    void uniform1fv(int loc, const float* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform1fv(loc, count, v);
    }
    void uniform1iv(int loc, const int* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform1iv(loc, count, v);
    }
    void uniformMatrix4fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0) return;
        bind();
        lib->glUniformMatrix4fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniform2fv(int loc, const float* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform2fv(loc, count, v);
    }
    void uniform3fv(int loc, const float* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform3fv(loc, count, v);
    }
    void uniform4fv(int loc, const float* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform4fv(loc, count, v);
    }
    void uniform2iv(int loc, const int* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform2iv(loc, count, v);
    }
    void uniform3iv(int loc, const int* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform3iv(loc, count, v);
    }
    void uniform4iv(int loc, const int* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0) return;
        bind();
        lib->glUniform4iv(loc, count, v);
    }
    void uniform1ui(int loc, uint32_t v0) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !lib->glUniform1ui) return;
        bind();
        lib->glUniform1ui(loc, v0);
    }
    void uniform2ui(int loc, uint32_t v0, uint32_t v1) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !lib->glUniform2ui) return;
        bind();
        lib->glUniform2ui(loc, v0, v1);
    }
    void uniform3ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !lib->glUniform3ui) return;
        bind();
        lib->glUniform3ui(loc, v0, v1, v2);
    }
    void uniform4ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !lib->glUniform4ui) return;
        bind();
        lib->glUniform4ui(loc, v0, v1, v2, v3);
    }
    void uniform1uiv(int loc, const uint32_t* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0
            || !lib->glUniform1uiv) return;
        bind();
        lib->glUniform1uiv(loc, count, v);
    }
    void uniform2uiv(int loc, const uint32_t* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0
            || !lib->glUniform2uiv) return;
        bind();
        lib->glUniform2uiv(loc, count, v);
    }
    void uniform3uiv(int loc, const uint32_t* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0
            || !lib->glUniform3uiv) return;
        bind();
        lib->glUniform3uiv(loc, count, v);
    }
    void uniform4uiv(int loc, const uint32_t* v, int count) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !v || count <= 0
            || !lib->glUniform4uiv) return;
        bind();
        lib->glUniform4uiv(loc, count, v);
    }
    void uniformMatrix2fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix2fv) return;
        bind();
        lib->glUniformMatrix2fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniformMatrix3fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix3fv) return;
        bind();
        lib->glUniformMatrix3fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    // Double-precision uniform setters: GLSL ES has no double uniforms, so these
    // are honest no-ops on the GLES backend.
    void uniform1d(int, double) override {}
    void uniform2d(int, double, double) override {}
    void uniform3d(int, double, double, double) override {}
    void uniform4d(int, double, double, double, double) override {}
    void uniform1dv(int, const double*, int) override {}
    void uniform2dv(int, const double*, int) override {}
    void uniform3dv(int, const double*, int) override {}
    void uniform4dv(int, const double*, int) override {}
    void uniformMatrix2dv(int, const double*, int, bool) override {}
    void uniformMatrix3dv(int, const double*, int, bool) override {}
    void uniformMatrix4dv(int, const double*, int, bool) override {}
    // Non-square matrix uniform setters (SPEC §7.6). ES 3.0 exposes the float
    // variants natively; the loader entry is checked because ES 2.0 drivers lack
    // them (then the call is dropped rather than faked).
    void uniformMatrix2x3fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix2x3fv) return;
        bind();
        lib->glUniformMatrix2x3fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniformMatrix2x4fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix2x4fv) return;
        bind();
        lib->glUniformMatrix2x4fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniformMatrix3x2fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix3x2fv) return;
        bind();
        lib->glUniformMatrix3x2fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniformMatrix3x4fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix3x4fv) return;
        bind();
        lib->glUniformMatrix3x4fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniformMatrix4x2fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix4x2fv) return;
        bind();
        lib->glUniformMatrix4x2fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    void uniformMatrix4x3fv(int loc, const float* m, int count, bool transpose) override {
        if (loc < 0 || !lib || !lib->loaded || handle == 0 || !m || count <= 0
            || !lib->glUniformMatrix4x3fv) return;
        bind();
        lib->glUniformMatrix4x3fv(loc, count, transpose ? GL_TRUE : GL_FALSE, m);
    }
    // GLSL ES has no double-precision uniforms, so the `dv` spellings stay honest
    // no-ops on this backend (same as uniformMatrix{2,3,4}dv above).
    void uniformMatrix2x3dv(int, const double*, int, bool) override {}
    void uniformMatrix2x4dv(int, const double*, int, bool) override {}
    void uniformMatrix3x2dv(int, const double*, int, bool) override {}
    void uniformMatrix3x4dv(int, const double*, int, bool) override {}
    void uniformMatrix4x2dv(int, const double*, int, bool) override {}
    void uniformMatrix4x3dv(int, const double*, int, bool) override {}
    void getUniformfv(int32_t location, float* params) const override {
        if (location < 0 || !lib || !lib->loaded || handle == 0 || !params) return;
        lib->glGetUniformfv(handle, location, params);
    }
    void getUniformiv(int32_t location, int32_t* params) const override {
        if (location < 0 || !lib || !lib->loaded || handle == 0 || !params) return;
        lib->glGetUniformiv(handle, location, params);
    }
    void getUniformuiv(int32_t location, uint32_t* params) const override {
        if (location < 0 || !lib || !lib->loaded || handle == 0 || !params) return;
        if (lib->glGetUniformuiv) lib->glGetUniformuiv(handle, location, params);
    }
    void getUniformdv(int32_t location, double* params) const override {
        if (location < 0 || !lib || !lib->loaded || handle == 0 || !params) return;
        // ES has no glGetUniformdv; widen the float query to double.
        if (lib->glGetUniformfv) {
            float f = 0;
            lib->glGetUniformfv(handle, location, &f);
            *params = static_cast<double>(f);
        }
    }

private:
    // Bind this program only when it is not already the bound driver program, so
    // back-to-back uniform calls on the same program skip redundant native binds
    // (SPEC §10). The shared loader's currentProgram is the single source of
    // truth, also updated by the backend's useProgram sink.
    void bind() {
        if (lib->currentProgram != handle) {
            if (lib->glUseProgram) lib->glUseProgram(handle);
            lib->currentProgram = handle;
        }
    }

    GLESLibPtr lib;
    GLuint handle = 0;
};

} // namespace glcompat
