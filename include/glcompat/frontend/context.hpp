#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/frontend/error.hpp"
#include "glcompat/frontend/objects.hpp"
#include "glcompat/state/gl_state.hpp"
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace glcompat {

// Frontend OpenGL context. Owns object identity, name allocation, binding
// state, and validation. Talks to the backend only through IGraphicsBackend,
// never to a native API. This is the foundation the OpenGL 4.6 API entry
// points will dispatch into (SPEC §2.1, §10, §11).
class Context {
public:
    explicit Context(IGraphicsBackend& backend) : backend_(backend) {
        for (auto& src : debugEnabled_)
            for (auto& ty : src)
                for (auto& sev : ty) sev = true;
    }

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

    // --- String queries (SPEC §22.2) ---
    // Returns VENDOR/RENDERER/VERSION/EXTENSIONS/SHADING_LANGUAGE_VERSION for the
    // current GL context. An unknown name sets GL_INVALID_ENUM and returns nullptr.
    const GLubyte* getString(GLenum name);
    // Indexed string query (SPEC §22.2). Only GL_EXTENSIONS is indexable; this
    // frontend exposes no extensions, so any index is out of range and yields
    // GL_INVALID_VALUE. Other names set GL_INVALID_ENUM.
    const GLubyte* getStringi(GLenum name, uint32_t index);

    // --- Buffers ---
    GLObjectName genBuffer();
    void genBuffers(uint32_t n, GLObjectName* names);
    void createBuffers(uint32_t n, GLObjectName* names);
    void bindBuffer(uint32_t target, GLObjectName name);
    GLObjectName boundBuffer(uint32_t target) const;
    void deleteBuffer(GLObjectName name);
    void deleteBuffers(uint32_t n, const GLObjectName* names);
    bool isBuffer(GLObjectName name) const;
    void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                     const void* data);
    // Update a sub-region of existing storage (SPEC §6 glBufferSubData). Requires
    // a bound buffer (else GL_INVALID_OPERATION) and a fully in-bounds region
    // (else GL_INVALID_VALUE). Allowed on both mutable and immutable storage.
    void bufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                       const void* data);
    // Allocate immutable storage (SPEC §6 glBufferStorage). Requires the
    // ImmutableBufferStorage capability; re-allocation of an already-immutable
    // buffer reports GL_INVALID_OPERATION. flags is the GL_MAP_* bitfield.
    void bufferStorage(uint32_t target, intptr_t size, const void* data,
                       uint32_t flags);
    // DSA buffer allocation (SPEC §6.1/§6.2 glNamedBufferData /
    // glNamedBufferSubData / glNamedBufferStorage). These operate on a named
    // buffer by object name (no bind required). They share the validation and
    // CPU-mirror bookkeeping of the target-based variants: an ungenerated name
    // reports GL_INVALID_OPERATION; sub-data must be in bounds (GL_INVALID_VALUE);
    // storage requires ImmutableBufferStorage and rejects re-allocation of an
    // already-immutable buffer (GL_INVALID_OPERATION), and a non-positive size for
    // storage reports GL_INVALID_VALUE.
    void namedBufferData(GLObjectName buffer, intptr_t size, uint32_t usage,
                          const void* data);
    void namedBufferSubData(GLObjectName buffer, intptr_t offset, intptr_t size,
                            const void* data);
    void namedBufferStorage(GLObjectName buffer, intptr_t size, const void* data,
                            uint32_t flags);
    // Copy a sub-region between two buffers (SPEC §6 glCopyBufferSubData). Both
    // read and write targets must be bound; the regions must be in bounds.
    void copyBufferSubData(uint32_t readTarget, uint32_t writeTarget,
                           intptr_t readOffset, intptr_t writeOffset,
                           intptr_t size);
    // DSA buffer copy (SPEC §6 glCopyNamedBufferSubData). Copies a region between
    // two named buffers by object name (no bind required). An ungenerated read or
    // write name reports GL_INVALID_OPERATION; an out-of-bounds region reports
    // GL_INVALID_VALUE. The frontend memcpy's its CPU mirror then pushes the
    // written region to the destination backend.
    void copyNamedBufferSubData(GLObjectName readBuffer, GLObjectName writeBuffer,
                                intptr_t readOffset, intptr_t writeOffset,
                                intptr_t size);
    // Query buffer parameters (SPEC §6 / §22 glGetBufferParameteriv). Reads the
    // frontend-owned state (size/usage/flags/mapped). A null `params` reports
    // GL_INVALID_VALUE; an unknown pname reports GL_INVALID_ENUM.
    void getBufferParameteriv(uint32_t target, uint32_t pname, int32_t* params);
    // 64-bit buffer parameter query (SPEC §6.1.1). GL_BUFFER_SIZE is genuinely
    // 64-bit; the other pnames widen the 32-bit form. Null params ->
    // GL_INVALID_VALUE; unbound target -> GL_INVALID_OPERATION; unknown pname ->
    // GL_INVALID_ENUM.
    void getBufferParameteri64v(uint32_t target, uint32_t pname, int64_t* params);
    // DSA variant (SPEC §6.1.1), capability-gated by DirectStateAccess.
    // Ungenerated name -> GL_INVALID_OPERATION.
    void getNamedBufferParameteri64v(GLObjectName buffer, uint32_t pname,
                                     int64_t* params);
    // DSA 32-bit variant (SPEC §6.1.1), capability-gated by DirectStateAccess.
    // Ungenerated name -> GL_INVALID_OPERATION.
    void getNamedBufferParameteriv(GLObjectName buffer, uint32_t pname,
                                   int32_t* params);
    // Mapped-buffer pointer query (SPEC §6.1.1, glGetBufferPointerv /
    // glGetNamedBufferPointerv). pname must be BUFFER_MAP_POINTER (GL_INVALID_ENUM
    // otherwise); null params -> GL_INVALID_VALUE; the returned pointer is the
    // current mapping into the frontend CPU mirror (nullptr when not mapped). The
    // named variant is capability-gated by DirectStateAccess; an ungenerated name
    // -> GL_INVALID_OPERATION. The target-based variant validates target against
    // the table-6.1 buffer targets (GL_INVALID_ENUM) and that a buffer is bound
    // (GL_INVALID_OPERATION).
    void getBufferPointerv(uint32_t target, uint32_t pname, void** params);
    void getNamedBufferPointerv(GLObjectName buffer, uint32_t pname, void** params);
    // Internal format queries (SPEC §22.3, glGetInternalformativ /
    // glGetInternalformati64v). Validates params != null (GL_INVALID_VALUE),
    // bufSize >= 0 (GL_INVALID_VALUE), and pname as a known internalformat-query
    // pname (GL_INVALID_ENUM), then forwards to the backend.
    void getInternalformativ(uint32_t target, uint32_t internalformat,
                            uint32_t pname, int32_t bufSize, int32_t* params);
    void getInternalformati64v(uint32_t target, uint32_t internalformat,
                               uint32_t pname, int32_t bufSize, int64_t* params);
    // Multisample sample-position query (SPEC §14.3.1, glGetMultisamplefv).
    // Validates val != null (GL_INVALID_VALUE), pname == SAMPLE_POSITION
    // (GL_INVALID_ENUM), and index < the backend's sample count
    // (GL_INVALID_VALUE), then forwards to the backend.
    void getMultisamplefv(uint32_t pname, uint32_t index, float* val);
    // Map a buffer for CPU access (SPEC §6 glMapBuffer / glMapBufferRange).
    // Returns a pointer into the frontend data store, or nullptr on error.
    // Mapping an already-mapped buffer reports GL_INVALID_OPERATION.
    void* mapBuffer(uint32_t target, uint32_t access);
    void* mapBufferRange(uint32_t target, intptr_t offset, intptr_t length,
                         uint32_t access);
    // Unmap a previously mapped buffer (SPEC §6 glUnmapBuffer). Returns false and
    // reports GL_INVALID_OPERATION when no buffer is mapped.
    bool unmapBuffer(uint32_t target);
    // Flush a mapped sub-region back to native storage (SPEC §6
    // glFlushMappedBufferRange). Requires a bound mapped buffer; the region must
    // be in bounds (else GL_INVALID_VALUE).
    void flushMappedBufferRange(uint32_t target, intptr_t offset, intptr_t length);
    // DSA buffer mapping (SPEC §6.1 glMapNamedBuffer / glMapNamedBufferRange /
    // glUnmapNamedBuffer / glFlushMappedNamedBufferRange). Operate on a named
    // buffer by object name (no bind required). They share the target-based
    // validation and CPU-mirror bookkeeping: an ungenerated name reports
    // GL_INVALID_OPERATION, an already-mapped buffer reports GL_INVALID_OPERATION,
    // and an out-of-bounds region reports GL_INVALID_VALUE. Unmap flushes the
    // CPU-mirror region back through the backend before clearing the mapping.
    void* mapNamedBuffer(GLObjectName buffer, uint32_t access);
    void* mapNamedBufferRange(GLObjectName buffer, intptr_t offset, intptr_t length,
                              uint32_t access);
    bool unmapNamedBuffer(GLObjectName buffer);
    void flushMappedNamedBufferRange(GLObjectName buffer, intptr_t offset,
                                     intptr_t length);
    // Read back a region of a buffer's data store (SPEC §6 glGetBufferSubData /
    // glGetNamedBufferSubData). Requires a bound/existing buffer; the region must
    // be in bounds (else GL_INVALID_VALUE) and the store must not be mapped
    // (unless mapped with MAP_PERSISTENT_BIT, else GL_INVALID_OPERATION). The
    // frontend's CPU mirror is the authoritative store, so the read is exact on
    // both the mock and real backends.
    void getBufferSubData(uint32_t target, intptr_t offset, intptr_t size, void* data);
    void getNamedBufferSubData(GLObjectName buffer, intptr_t offset, intptr_t size,
                              void* data);
    // Fill a buffer's data store (SPEC §6 glClearBufferData / glClearBufferSubData
    // and the *Named variants). The frontend converts the clear value into the
    // `internalformat`'s component layout, writes it into its CPU mirror, and
    // re-uploads the range to the backend (which has no native glClearBufferData).
    // `internalformat` must be a sized format from table 8.24 (GL_INVALID_ENUM),
    // `offset`/`size` must be non-negative, multiples of the element size, and in
    // bounds (GL_INVALID_VALUE), and the store must not be mapped (GL_INVALID_OPERATION).
    // A null `data` fills the range with zeros.
    void clearBufferData(uint32_t target, uint32_t internalformat, uint32_t format,
                        uint32_t type, const void* data);
    void clearNamedBufferData(GLObjectName buffer, uint32_t internalformat,
                             uint32_t format, uint32_t type, const void* data);
    void clearBufferSubData(uint32_t target, uint32_t internalformat, intptr_t offset,
                           intptr_t size, uint32_t format, uint32_t type,
                           const void* data);
    void clearNamedBufferSubData(GLObjectName buffer, uint32_t internalformat,
                               intptr_t offset, intptr_t size, uint32_t format,
                               uint32_t type, const void* data);
    // Discard a buffer's data store or sub-range (SPEC §6.5,
    // glInvalidateBufferData / glInvalidateBufferSubData). Both take the *buffer
    // object name* (the spec has no target-based form). A zero or ungenerated
    // name, a negative offset/length or a range past GL_BUFFER_SIZE report
    // GL_INVALID_VALUE; invalidating a non-persistently mapped buffer reports
    // GL_INVALID_OPERATION. The hint is forwarded to the backend, which drops any
    // cached copy.
    void invalidateBufferData(GLObjectName buffer);
    void invalidateBufferSubData(GLObjectName buffer, intptr_t offset,
                                 intptr_t length);
    BufferObject* getBuffer(GLObjectName name);

    // --- Indexed buffer bindings (SPEC §6.1.1) ---
    // Capability-guarded: a target without indexed binding points reports
    // GL_INVALID_ENUM, and a legal target the backend does not support (e.g.
    // SSBO on ES 3.0, UBO on ES 2.0) reports GL_INVALID_OPERATION honestly
    // instead of issuing an unsupported native call. `index` beyond the tracked
    // binding-point count and (for the range forms) a negative offset or a
    // non-positive size with a non-zero buffer report GL_INVALID_VALUE.
    void bindBufferBase(uint32_t target, uint32_t index, GLObjectName buffer);
    void bindBufferRange(uint32_t target, uint32_t index, GLObjectName buffer,
                         intptr_t offset, intptr_t size);
    // Multi-bind forms (`glBindBuffersBase`/`glBindBuffersRange`, SPEC §6.1.1 /
    // ARB_multi_bind). Bind consecutive binding points [first, first+count).
    // A null `buffers` array resets the whole range to unbound (offsets/sizes
    // ignored). Negative `count` reports GL_INVALID_VALUE, `first + count` past
    // the binding-point count reports GL_INVALID_OPERATION, and each entry is
    // validated separately so an invalid one leaves only its own binding point
    // unchanged while the rest still bind.
    void bindBuffersBase(uint32_t target, uint32_t first, GLsizei count,
                         const GLObjectName* buffers);
    void bindBuffersRange(uint32_t target, uint32_t first, GLsizei count,
                          const GLObjectName* buffers, const intptr_t* offsets,
                          const intptr_t* sizes);

    // --- Textures ---
    GLObjectName genTexture();
    void genTextures(uint32_t n, GLObjectName* names);
    // glActiveTexture selects the active texture image unit (texture = GL_TEXTURE0
    // + i). An out-of-range value reports GL_INVALID_ENUM honestly (SPEC §2.1).
    void activeTexture(GLenum texture);
    // glBindTexture binds `name` to `target` on the active texture unit. A non-zero
    // name that was not generated reports GL_INVALID_OPERATION (SPEC §2.1).
    void bindTexture(GLenum target, GLObjectName name);
    GLObjectName boundTextureForTarget(GLenum target) const;
    // DSA texture binding (SPEC §2.1, capability-gated by DirectStateAccess).
    // glBindTextureUnit binds `texture` to `target` on a specific `unit` (zero
    // based) without modifying the active-texture selector. glBindTextures
    // (SPEC §8.1 / ARB_multi_bind) binds an array of textures to consecutive
    // units [first, first+count); each texture goes to the target it was created
    // with, and a zero entry (or a null array) resets every target of that unit.
    // Both capability-gated by DirectStateAccess (GL_INVALID_OPERATION when
    // unsupported). glBindTextureUnit: out-of-range unit -> GL_INVALID_VALUE,
    // ungenerated name -> GL_INVALID_OPERATION. glBindTextures: negative count
    // -> GL_INVALID_VALUE, `first + count` past the unit count ->
    // GL_INVALID_OPERATION, and entries are validated per unit so an ungenerated
    // name leaves only that unit unchanged (GL_INVALID_OPERATION) while the
    // remaining valid entries still bind.
    void bindTextureUnit(uint32_t unit, GLObjectName texture);
    void bindTextures(uint32_t first, GLsizei count,
                      const GLObjectName* textures);
    GLObjectName boundTextureForUnitTarget(uint32_t unit, GLenum target) const;
    void deleteTexture(GLObjectName name);
    void deleteTextures(uint32_t n, const GLObjectName* names);
    bool isTexture(GLObjectName name) const;
    TextureObject* getTexture(GLObjectName name);

    // --- Textures (SPEC §2.1) ---
    // Operate on the currently bound texture. glTexImage2D allocates storage on
    // the backend resource; glTexParameteri records the parameter and pushes it to
    // the backend resource. Wrong/unknown targets are ignored like desktop GL.
    void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, uint32_t format, uint32_t type,
                    const void* data);
    void texImage1D(uint32_t target, int level, uint32_t internalFormat,
                    int width, uint32_t format, uint32_t type, const void* data);
    void texImage3D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, int depth, uint32_t format,
                    uint32_t type, const void* data);
    void texParameteri(uint32_t target, uint32_t pname, int param);
    // Texture parameter setters (SPEC §8). glTexParameterf sets a float scalar;
    // glTexParameterfv/iv set vector parameters (e.g. GL_TEXTURE_BORDER_COLOR).
    // Each records the value on the bound texture and forwards to the backend.
    void texParameterf(uint32_t target, uint32_t pname, float param);
    void texParameterfv(uint32_t target, uint32_t pname, const float* params,
                        int count);
    void texParameteriv(uint32_t target, uint32_t pname, const int* params,
                        int count);
    // Integer (signed / unsigned) texture parameter setters + queries (SPEC §8.1).
    // glTexParameterIiv / glTexParameterIuiv record the vector on the bound texture
    // and forward to the backend; glGetTexParameterIiv / glGetTexParameterIuiv read
    // the stored vector. A null `params` reports GL_INVALID_VALUE; a missing texture
    // reports GL_INVALID_OPERATION; unknown pnames return 0.
    void texParameterIiv(uint32_t target, uint32_t pname, const int32_t* params);
    void texParameterIuiv(uint32_t target, uint32_t pname, const uint32_t* params);
    void getTexParameterIiv(GLenum target, GLenum pname, int32_t* params);
    void getTexParameterIuiv(GLenum target, GLenum pname, uint32_t* params);
    // Regenerate the full mipmap chain for the bound texture (SPEC §8.1
    // glGenerateMipmap). Requires a bound texture (else GL_INVALID_OPERATION).
    void generateMipmap(uint32_t target);
    // Invalidate a texture's contents (SPEC §8.1 glInvalidateTexImage /
    // glInvalidateTexSubImage). invalidateTexImage discards the whole level;
    // invalidateTexSubImage discards a sub-region. Require a bound texture (else
    // GL_INVALID_OPERATION); level < 0 reports GL_INVALID_VALUE.
    void invalidateTexImage(uint32_t target, int level);
    void invalidateTexSubImage(uint32_t target, int level, int xoffset, int yoffset,
                              int zoffset, int width, int height, int depth);
    // Texture sub-image uploads (SPEC §8.6 TexSubImage*D). Require a bound texture
    // (else GL_INVALID_OPERATION) and a previously allocated `level` (else
    // GL_INVALID_OPERATION). Non-negative level/dimensions/offset are required
    // (else GL_INVALID_VALUE); the region must fit inside the allocated level
    // (else GL_INVALID_VALUE).
    void texSubImage1D(uint32_t target, int level, int xoffset, int width,
                       uint32_t format, uint32_t type, const void* data);
    void texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                       int width, int height, uint32_t format, uint32_t type,
                       const void* data);
    void texSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                       int zoffset, int width, int height, int depth,
                       uint32_t format, uint32_t type, const void* data);
    // Define texture image by copying from the framebuffer (SPEC §8.5
    // CopyTexImage*D). Require a bound texture (else GL_INVALID_OPERATION);
    // negative level/width/height or a non-zero border report GL_INVALID_VALUE.
    void copyTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                        int x, int y, int width, int border);
    void copyTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                         int x, int y, int width, int height, int border);
    // Define a texture sub-region by copying from the framebuffer (SPEC §8.5
    // glCopyTexSubImage*D). Require a bound texture of the matching 1D/2D/3D
    // target (else GL_INVALID_OPERATION); only the 1D/2D/3D targets are valid
    // (rectangle is an honest GL_INVALID_ENUM capability gap). A negative level or
    // offset, or a non-positive width/height, reports GL_INVALID_VALUE. The backend
    // reads from the currently bound read framebuffer.
    void copyTexSubImage1D(uint32_t target, int level, int xoffset, int x, int y,
                           int width);
    void copyTexSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                           int x, int y, int width, int height);
    void copyTexSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                           int zoffset, int x, int y, int width, int height);

    // Compressed texture image upload (SPEC §8.6 glCompressedTexImage*D). Require a
    // bound texture (else GL_INVALID_OPERATION). Rectangular/proxy targets report
    // GL_INVALID_ENUM; a non-zero border or negative level/dimension/imageSize
    // reports GL_INVALID_VALUE. The compressed data is recorded as the level's image
    // so level-parameter queries stay consistent; the backend forwards the native
    // compressed upload when present.
    void compressedTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                             int width, int border, int imageSize, const void* data);
    void compressedTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                             int width, int height, int border, int imageSize,
                             const void* data);
    void compressedTexImage3D(uint32_t target, int level, uint32_t internalFormat,
                             int width, int height, int depth, int border,
                             int imageSize, const void* data);
    // Compressed texture sub-image upload (SPEC §8.6 glCompressedTexSubImage*D).
    // Require a bound texture (else GL_INVALID_OPERATION) and a previously allocated
    // `level` (else GL_INVALID_OPERATION). Non-negative level/offset/dimension/
    // imageSize are required (else GL_INVALID_VALUE).
    void compressedTexSubImage1D(uint32_t target, int level, int xoffset, int width,
                                uint32_t format, int imageSize, const void* data);
    void compressedTexSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                                int width, int height, uint32_t format,
                                int imageSize, const void* data);
    void compressedTexSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                                int zoffset, int width, int height, int depth,
                                uint32_t format, int imageSize, const void* data);
    // Texture parameter queries (SPEC §8.1). getTexParameteriv reads the
    // currently bound texture for `target`; getTextureParameteriv is the DSA
    // variant that reads an explicit texture object (capability-gated by
    // DirectStateAccess). A null `params` reports GL_INVALID_VALUE; a missing
    // texture reports GL_INVALID_OPERATION. Unknown pnames return 0 (the GL
    // default), matching the driver's initial parameter state.
    void getTexParameteriv(GLenum target, GLenum pname, int32_t* params);
    void getTexParameterfv(GLenum target, GLenum pname, float* params);
    void getTextureParameteriv(GLObjectName texture, GLenum pname,
                                int32_t* params);

    // --- Direct State Access texture surface (SPEC §2.1 / §8.1) ---
    // Operate on an explicit, named texture object instead of the bound one.
    // Capability-gated by DirectStateAccess (Emulated: YAGLT emulates DSA via the
    // object's backend resource). createTextures generates names and records the
    // implied target; the *storage / *subImage / *parameter / *buffer entry
    // points validate the name and forward to the backend resource.
    void createTextures(uint32_t target, uint32_t n, GLObjectName* names);
    void textureStorage1D(GLObjectName texture, int levels, uint32_t internalFormat,
                          int width);
    void textureStorage2D(GLObjectName texture, int levels, uint32_t internalFormat,
                           int width, int height);
    void textureStorage3D(GLObjectName texture, int levels, uint32_t internalFormat,
                          int width, int height, int depth);
    // Create a texture view sharing immutable storage with `origtexture` (SPEC
    // §8.19 glTextureView). Validates both objects exist, that they differ, that
    // the source has immutable storage, and that the target/range are sane, then
    // records the view and forwards to the backend (which must have TextureViews).
    void textureView(GLObjectName texture, uint32_t target, GLObjectName origtexture,
                     uint32_t internalFormat, uint32_t minLevel, uint32_t numLevels,
                     uint32_t minLayer, uint32_t numLayers);
    void textureSubImage1D(GLObjectName texture, int level, int xoffset, int width,
                           uint32_t format, uint32_t type, const void* data);
    void textureSubImage2D(GLObjectName texture, int level, int xoffset, int yoffset,
                           int width, int height, uint32_t format, uint32_t type,
                           const void* data);
    void textureSubImage3D(GLObjectName texture, int level, int xoffset, int yoffset,
                           int zoffset, int width, int height, int depth,
                           uint32_t format, uint32_t type, const void* data);
    // Compressed DSA sub-image upload (SPEC §8.6 glCompressedTextureSubImage*D).
    // Capability-gated by DirectStateAccess (Emulated); require a generated texture
    // (else GL_INVALID_OPERATION) and a previously allocated `level` (else
    // GL_INVALID_OPERATION). Non-negative level/offset/dimension/imageSize are
    // required (else GL_INVALID_VALUE).
    void compressedTextureSubImage1D(GLObjectName texture, int level, int xoffset,
                                   int width, uint32_t format, int imageSize,
                                   const void* data);
    void compressedTextureSubImage2D(GLObjectName texture, int level, int xoffset,
                                   int yoffset, int width, int height,
                                   uint32_t format, int imageSize, const void* data);
    void compressedTextureSubImage3D(GLObjectName texture, int level, int xoffset,
                                    int yoffset, int zoffset, int width, int height,
                                    int depth, uint32_t format, int imageSize,
                                    const void* data);
    // Define a named texture sub-region by copying from the framebuffer (SPEC §8.5
    // glCopyTextureSubImage*D, DSA). Capability-gated by DirectStateAccess; an
    // ungenerated name reports GL_INVALID_OPERATION. A negative level or offset, or
    // a non-positive width/height, reports GL_INVALID_VALUE.
    void copyTextureSubImage1D(GLObjectName texture, int level, int xoffset, int x,
                               int y, int width);
    void copyTextureSubImage2D(GLObjectName texture, int level, int xoffset,
                               int yoffset, int x, int y, int width, int height);
    void copyTextureSubImage3D(GLObjectName texture, int level, int xoffset,
                               int yoffset, int zoffset, int x, int y, int width,
                               int height);

    void textureParameteri(GLObjectName texture, uint32_t pname, int param);
    void textureParameterf(GLObjectName texture, uint32_t pname, float param);
    void textureParameterfv(GLObjectName texture, uint32_t pname, const float* params,
                            int count);
    void textureParameteriv(GLObjectName texture, uint32_t pname, const int* params,
                            int count);
    void textureParameterIiv(GLObjectName texture, uint32_t pname, const int32_t* params);
    void textureParameterIuiv(GLObjectName texture, uint32_t pname, const uint32_t* params);
    void getTextureParameterIiv(GLObjectName texture, GLenum pname, int32_t* params);
    void getTextureParameterIuiv(GLObjectName texture, GLenum pname, uint32_t* params);
    void generateTextureMipmap(GLObjectName texture);
    void getTextureParameterfv(GLObjectName texture, GLenum pname, float* params);
    void getTextureLevelParameteriv(GLObjectName texture, int level, GLenum pname,
                                    int32_t* params);
    void getTextureLevelParameterfv(GLObjectName texture, int level, GLenum pname,
                                     float* params);
    // Classic (non-DSA) counterparts operating on the texture bound to `target`.
    void getTexLevelParameteriv(uint32_t target, int level, GLenum pname,
                               int32_t* params);
    void getTexLevelParameterfv(uint32_t target, int level, GLenum pname,
                               float* params);
    void getTextureImage(GLObjectName texture, int level, uint32_t format,
                         uint32_t type, void* pixels);
    void getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                     void* pixels);
    void getCompressedTextureImage(GLObjectName texture, int level, void* pixels);
    void getCompressedTexImage(uint32_t target, int level, void* pixels);
    // Robustness (ARB_robustness / GL 4.5) bounds-checked read-back variants.
    // `bufSize` is the byte capacity of `pixels`; a negative `bufSize` or `level`
    // yields GL_INVALID_VALUE, an unbound texture yields GL_INVALID_OPERATION.
    void getTextureImage(GLObjectName texture, int level, uint32_t format,
                         uint32_t type, int bufSize, void* pixels);
    void getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                     int bufSize, void* pixels);
    void getCompressedTextureImage(GLObjectName texture, int level, int bufSize,
                                   void* pixels);
    void getCompressedTexImage(uint32_t target, int level, int bufSize, void* pixels);
    void getTextureSubImage(GLObjectName texture, int level, int xoffset, int yoffset,
                            int zoffset, int width, int height, int depth,
                            uint32_t format, uint32_t type, int bufSize, void* pixels);
    void getCompressedTextureSubImage(GLObjectName texture, int level, int xoffset,
                                      int yoffset, int zoffset, int width, int height,
                                      int depth, int bufSize, void* pixels);
    void textureBuffer(GLObjectName texture, uint32_t internalFormat,
                       GLObjectName buffer);
    void textureBufferRange(GLObjectName texture, uint32_t internalFormat,
                             GLObjectName buffer, intptr_t offset, intptr_t size);

    // --- Non-DSA texture storage (SPEC §8.5) ---
    // Operate on the texture currently bound to `target` (resolved via the
    // tracked binding). glTexBuffer* bind a buffer object as a texel store.
    void texStorage1D(uint32_t target, int levels, uint32_t internalFormat, int width);
    void texStorage2D(uint32_t target, int levels, uint32_t internalFormat, int width,
                     int height);
    void texStorage3D(uint32_t target, int levels, uint32_t internalFormat, int width,
                     int height, int depth);
    void texBuffer(uint32_t target, uint32_t internalFormat, GLObjectName buffer);
    void texBufferRange(uint32_t target, uint32_t internalFormat, GLObjectName buffer,
                        intptr_t offset, intptr_t size);
    // --- Multisample texture storage (SPEC §8.19) ---
    void texStorage2DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                                 int width, int height, bool fixedSampleLocations);
    void texStorage3DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                                 int width, int height, int depth,
                                 bool fixedSampleLocations);
    void texImage2DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                              int width, int height, bool fixedSampleLocations);
    void texImage3DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                              int width, int height, int depth,
                              bool fixedSampleLocations);
    void textureStorage2DMultisample(GLObjectName texture, int samples,
                                     uint32_t internalFormat, int width, int height,
                                     bool fixedSampleLocations);
    void textureStorage3DMultisample(GLObjectName texture, int samples,
                                     uint32_t internalFormat, int width, int height,
                                     int depth, bool fixedSampleLocations);

    // --- Renderbuffers ---
    GLObjectName genRenderbuffer();
    void genRenderbuffers(uint32_t n, GLObjectName* names);
    void bindRenderbuffer(GLObjectName name);
    GLObjectName boundRenderbuffer() const;
    void deleteRenderbuffer(GLObjectName name);
    void deleteRenderbuffers(uint32_t n, const GLObjectName* names);
    bool isRenderbuffer(GLObjectName name) const;
    RenderbufferObject* getRenderbuffer(GLObjectName name);

    // --- Renderbuffers (SPEC §2.1) ---
    // Allocate storage on the currently bound renderbuffer. Negative dimensions
    // are GL_INVALID_VALUE; no bound renderbuffer is GL_INVALID_OPERATION; the
    // target must be GL_RENDERBUFFER. Capability-gated (RenderbufferObjects).
    void renderbufferStorage(uint32_t target, uint32_t internalFormat, int width,
                             int height);

    // --- Direct State Access renderbuffer surface (SPEC §8.2 / §9.2) ---
    // Operate on an explicit, named renderbuffer instead of the bound one.
    // Capability-gated by DirectStateAccess (Emulated: YAGLT emulates DSA via the
    // object's backend resource). createRenderbuffers generates names; the storage
    // entry points validate the name and forward to the backend resource.
    void createRenderbuffers(uint32_t n, GLObjectName* names);
    void namedRenderbufferStorage(GLObjectName renderbuffer, uint32_t internalFormat,
                                 int width, int height);
    void namedRenderbufferStorageMultisample(GLObjectName renderbuffer, int samples,
                                            uint32_t internalFormat, int width,
                                            int height);
    void getNamedRenderbufferParameteriv(GLObjectName renderbuffer, uint32_t pname,
                                        int32_t* params);
    // Classic (non-DSA) counterpart operating on the renderbuffer bound to
    // `target` (must be GL_RENDERBUFFER).
    void getRenderbufferParameteriv(uint32_t target, uint32_t pname,
                                    int32_t* params);

    // --- Framebuffers ---
    GLObjectName genFramebuffer();
    void genFramebuffers(uint32_t n, GLObjectName* names);
    void bindFramebuffer(GLObjectName name);
    GLObjectName boundFramebuffer() const;
    void deleteFramebuffer(GLObjectName name);
    void deleteFramebuffers(uint32_t n, const GLObjectName* names);
    bool isFramebuffer(GLObjectName name) const;
    FramebufferObject* getFramebuffer(GLObjectName name);

    // --- Framebuffers (SPEC §2.1) ---
    // Operate on the currently bound framebuffer. Attachment points are recorded
    // on the Frontend framebuffer object and forwarded to the backend resource.
    // Attaching a non-existent object reports GL_INVALID_OPERATION honestly.
    void framebufferTexture2D(uint32_t target, uint32_t attachment,
                              uint32_t texTarget, GLObjectName texture, int level);
    void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                 uint32_t rbTarget, GLObjectName renderbuffer);
    void framebufferTexture(uint32_t target, uint32_t attachment,
                            GLObjectName texture, int level);
    void framebufferTextureLayer(uint32_t target, uint32_t attachment,
                                 GLObjectName texture, int level, int layer);
    // Returns a GL_FRAMEBUFFER_* status code. Combines the structural check with
    // the backend resource's driver-level checkStatus().
    uint32_t checkFramebufferStatus(uint32_t target);

    // --- Direct State Access framebuffer surface (SPEC §9.2) ---
    // Operate on an explicit, named framebuffer instead of the bound one.
    // Capability-gated by DirectStateAccess (Emulated: YAGLT emulates DSA via the
    // object's backend resource). createFramebuffers generates names; the attach /
    // parameter / status / query entry points validate the name and forward to the
    // backend resource. Named blit/invalidate/clear bind the named framebuffer(s)
    // to the driver and reuse the whole-framebuffer backend ops.
    void createFramebuffers(uint32_t n, GLObjectName* names);
    void namedFramebufferRenderbuffer(GLObjectName framebuffer, uint32_t attachment,
                                      uint32_t renderbufferTarget,
                                      GLObjectName renderbuffer);
    void namedFramebufferTexture(GLObjectName framebuffer, uint32_t attachment,
                                GLObjectName texture, int level);
    void namedFramebufferTextureLayer(GLObjectName framebuffer, uint32_t attachment,
                                     GLObjectName texture, int level, int layer);
    uint32_t checkNamedFramebufferStatus(GLObjectName framebuffer, uint32_t target);
    void namedFramebufferParameteri(GLObjectName framebuffer, uint32_t pname,
                                    int param);
    void getNamedFramebufferParameteriv(GLObjectName framebuffer, uint32_t pname,
                                        int32_t* params);
    // Classic (non-DSA) counterpart operating on the framebuffer bound to
    // `target` (GL_FRAMEBUFFER / GL_READ_FRAMEBUFFER / GL_DRAW_FRAMEBUFFER).
    void getFramebufferParameteriv(uint32_t target, uint32_t pname,
                                   int32_t* params);
    // Classic (non-DSA) counterpart operating on the framebuffer bound to
    // `target` (GL_FRAMEBUFFER / GL_READ_FRAMEBUFFER / GL_DRAW_FRAMEBUFFER).
    // Generates GL_INVALID_OPERATION when the default framebuffer is bound.
    void framebufferParameteri(uint32_t target, uint32_t pname, int param);
    void getNamedFramebufferAttachmentParameteriv(GLObjectName framebuffer,
                                                  uint32_t attachment,
                                                  uint32_t pname, int32_t* params);
    // Classic (non-DSA) counterpart operating on the framebuffer bound to
    // `target` (GL_FRAMEBUFFER / GL_READ_FRAMEBUFFER / GL_DRAW_FRAMEBUFFER).
    void getFramebufferAttachmentParameteriv(uint32_t target, uint32_t attachment,
                                             uint32_t pname, int32_t* params);
    void blitNamedFramebuffer(GLObjectName readFb, GLObjectName drawFb,
                             int32_t srcX0, int32_t srcY0, int32_t srcX1,
                             int32_t srcY1, int32_t dstX0, int32_t dstY0,
                             int32_t dstX1, int32_t dstY1, uint32_t mask,
                             uint32_t filter);
    void invalidateNamedFramebufferData(GLObjectName framebuffer,
                                       int32_t numAttachments,
                                       const uint32_t* attachments);
    void invalidateNamedFramebufferSubData(GLObjectName framebuffer,
                                          int32_t numAttachments,
                                          const uint32_t* attachments, int32_t x,
                                          int32_t y, int32_t width, int32_t height);
    void clearNamedFramebufferiv(GLObjectName framebuffer, uint32_t buffer,
                                int drawbuffer, const int32_t* value);
    void clearNamedFramebufferuiv(GLObjectName framebuffer, uint32_t buffer,
                                 int drawbuffer, const uint32_t* value);
    void clearNamedFramebufferfv(GLObjectName framebuffer, uint32_t buffer,
                                int drawbuffer, const float* value);
    void clearNamedFramebufferfi(GLObjectName framebuffer, uint32_t buffer,
                                 int drawbuffer, float depth, int stencil);

    // --- Bound-framebuffer clears (SPEC §9.3.1 / §15.2.3) ---
    // Clear a single buffer of the *currently bound* draw framebuffer (classic
    // glClearBuffer*). Per-type clear values are pushed to the backend via the
    // state sink before the native clear (SPEC §10). `drawbuffer` selects the
    // color attachment; the depth/stencil clears apply to the single depth and
    // stencil attachments.
    void clearBufferiv(uint32_t buffer, int drawbuffer, const int32_t* value);
    void clearBufferuiv(uint32_t buffer, int drawbuffer, const uint32_t* value);
    void clearBufferfv(uint32_t buffer, int drawbuffer, const float* value);
    void clearBufferfi(uint32_t buffer, int drawbuffer, float depth, int stencil);

    // --- Clip control (SPEC §12.1, glClipControl) ---
    // Record the clip-volume origin (GL_LOWER_LEFT / GL_UPPER_LEFT) and depth mode
    // (GL_NEGATIVE_ONE_TO_ONE / GL_ZERO_TO_ONE) in the state tracker; pushed to the
    // backend on the next state flush (SPEC §10). Invalid enums are rejected here
    // (GL_INVALID_ENUM) and leave state untouched.
    void clipControl(uint32_t origin, uint32_t depth);


    // --- Viewport / scissor (SPEC §10) ---
    // Record viewport (glViewport) and scissor box (glScissor) state in the
    // tracker; pushed to the backend on the next state flush (SPEC §10). The
    // scissor *test* is a capability (GL_SCISSOR_TEST) toggled via glEnable/
    // glDisable, separate from the box itself.
    void setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    void setScissor(GLint x, GLint y, GLsizei width, GLsizei height);

    // Indexed viewport/scissor (SPEC §10.3.1). `index` selects the
    // viewport/scissor slot. `width`/`height` < 0 → GL_INVALID_VALUE; `index`
    // >= MAX_VIEWPORTS (16) → GL_INVALID_VALUE.
    void setViewportIndexed(GLuint index, GLint x, GLint y, GLsizei width,
                            GLsizei height);
    void setScissorIndexed(GLuint index, GLint x, GLint y, GLsizei width,
                           GLsizei height);

    // Contiguous viewport/scissor arrays (SPEC §13.5.2). `first` + `count` must
    // be <= MAX_VIEWPORTS (16), `count` > 0, and `v` non-null; otherwise
    // GL_INVALID_VALUE. Each viewport is four floats (x, y, w, h); each scissor
    // box is four ints. Negative width/height → GL_INVALID_VALUE.
    void setViewportArrayv(GLuint first, GLsizei count, const GLfloat* v);
    void setScissorArrayv(GLuint first, GLsizei count, const GLint* v);

    // Per-viewport depth range (SPEC §13.5.2). `index` >= MAX_VIEWPORTS (16) →
    // GL_INVALID_VALUE; glDepthRangeArrayv additionally requires `first`+`count`
    // <= MAX_VIEWPORTS, `count` > 0, and `v` non-null (GL_INVALID_VALUE).
    void setDepthRangeIndexed(GLuint index, GLdouble nearVal, GLdouble farVal);
    void setDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble* v);

    // Per-draw-buffer color write mask (SPEC §17.3.6, glColorMaski). `buf`
    // selects the draw-buffer slot; `buf` >= MAX_DRAW_BUFFERS (8) →
    // GL_INVALID_VALUE. The non-indexed glColorMask sets every draw buffer.
    void setColorMaski(GLuint buf, GLboolean red, GLboolean green,
                       GLboolean blue, GLboolean alpha);

    // Indexed blending (SPEC §15.3 / §17.3.4). `buf` selects the draw-buffer
    // slot; `buf` >= MAX_DRAW_BUFFERS (8) → GL_INVALID_VALUE, invalid blend
    // factors / equations → GL_INVALID_ENUM. Buffer 0 updates the same state as
    // the non-indexed glBlendFunc / glBlendEquation setters.
    void setBlendFunci(GLuint buf, GLenum src, GLenum dst);
    void setBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB,
                               GLenum srcAlpha, GLenum dstAlpha);
    void setBlendEquationi(GLuint buf, GLenum mode);
    void setBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha);

    // --- Clear values + clear (SPEC §2.1) ---
    // glClearColor / glClearDepth record the per-context clear values in the
    // tracker and are pushed to the backend on the next state flush. glClear
    // flushes tracked state first, then issues the native clear for the given
    // mask. An invalid mask (bits outside color/depth/stencil) reports
    // GL_INVALID_VALUE honestly.
    void setClearColor(float r, float g, float b, float a);
    void setClearDepth(double d);
    void setClearStencil(int s);
    void setPolygonOffsetClamp(float factor, float units, float clamp);
    void clear(uint32_t mask);

    // --- Texture clearing (SPEC §8.10) ---
    // DSA texture clears. The texture must exist and have storage
    // (GL_INVALID_OPERATION otherwise); `level` must be within [0, levels)
    // (GL_INVALID_VALUE). `format`/`type` are forwarded honestly. `data` is
    // normally null. glClearTexSubImage additionally validates the region.
    void clearTexImage(GLObjectName texture, int level, uint32_t format,
                      uint32_t type, const void* data);
    void clearTexSubImage(GLObjectName texture, int level, int x, int y, int z,
                         int w, int h, int d, uint32_t format, uint32_t type,
                         const void* data);

    // --- Image-to-image copy (SPEC §8.21) ---
    // Copy a texel sub-region between two image objects (textures or
    // renderbuffers). Validates targets, object existence, level ranges,
    // dimension signs, and sub-region bounds before forwarding to the backend.
    void copyImageSubData(GLObjectName srcName, uint32_t srcTarget, int srcLevel,
                         int srcX, int srcY, int srcZ, GLObjectName dstName,
                         uint32_t dstTarget, int dstLevel, int dstX, int dstY,
                         int dstZ, int srcWidth, int srcHeight, int srcDepth);

    // --- Whole-framebuffer buffer selection (SPEC §15 / §16) ---
    // Select the draw buffers for the currently bound framebuffer (glDrawBuffers)
    // and its read buffer (glReadBuffer). Pushed to the backend at the next state
    // flush. n must be positive (else GL_INVALID_VALUE); each buffer must be a
    // valid draw/read-buffer enum (else GL_INVALID_ENUM).
    void drawBuffers(int32_t n, const GLenum* bufs);
    void readBuffer(GLenum buf);

    // --- Color logic op (SPEC §17.3.4, glLogicOp) ---
    // Records the logic op mode in the tracker (pushed to the backend only when it
    // changes, SPEC §10); the driver applies it only while GL_COLOR_LOGIC_OP is
    // enabled. Capability-gated by LogicOp.
    void logicOp(uint32_t mode);

    // --- Primitive restart (SPEC §10.4, glPrimitiveRestartIndex) ---
    // Records the restart index in the tracker; pushed to the backend on change
    // (SPEC §10). Activation is via glEnable(GL_PRIMITIVE_RESTART), a normal cap.
    void primitiveRestartIndex(uint32_t index);

    // --- Rasterization controls (SPEC §11) ---
    // glPolygonMode sets the per-side render mode (GL_POINT/GL_LINE/GL_FILL); an
    // unsupported `face` or `mode` reports GL_INVALID_ENUM. glSampleMaski sets one
    // sample-mask word (maskNumber must be < MAX_SAMPLE_MASK_WORDS, else
    // GL_INVALID_VALUE). glMinSampleShading selects the minimum sample-shading
    // fraction in [0,1] (else GL_INVALID_VALUE). All are recorded in the tracker
    // and pushed to the backend on the next state flush (SPEC §10).
    void polygonMode(GLenum face, GLenum mode);
    void sampleMaski(uint32_t maskNumber, uint32_t mask);
    void minSampleShading(float value);
    // Provoking vertex convention (SPEC §11, glProvokingVertex). `mode` must be
    // GL_FIRST_VERTEX_CONVENTION or GL_LAST_VERTEX_CONVENTION (default LAST),
    // else GL_INVALID_ENUM. Recorded in the tracker and pushed on change (SPEC §10).
    void provokingVertex(GLenum mode);

    // Color clamping (SPEC §15.2.3, glClampColor). `target` must be
    // GL_CLAMP_READ_COLOR (else GL_INVALID_ENUM); `mode` must be GL_TRUE,
    // GL_FALSE or GL_FIXED_ONLY (else GL_INVALID_ENUM). Recorded and pushed on
    // change (SPEC §10).
    void clampColor(GLenum target, GLenum mode);

    // Point parameters (SPEC §10.2, glPointParameter{i,f,iv,fv}). Validates the
    // pname (GL_INVALID_ENUM otherwise) and, for the size/fade params, a negative
    // value (GL_INVALID_VALUE), then records the field in the tracker and pushes
    // it to the backend on the next state flush (SPEC §10). POINT_SPRITE_COORD_
    // ORIGIN must be GL_LOWER_LEFT / GL_UPPER_LEFT (else GL_INVALID_ENUM).
    void pointParameteri(GLenum pname, GLint param);
    void pointParameterf(GLenum pname, GLfloat param);
    void pointParameteriv(GLenum pname, const GLint* params);
    void pointParameterfv(GLenum pname, const GLfloat* params);

    // Patch parameters (SPEC §10.6, glPatchParameter{i,fv}). Validates the pname
    // (GL_INVALID_ENUM otherwise) and, for glPatchParameteri, a non-positive
    // vertex count (GL_INVALID_VALUE). glPatchParameterfv only accepts the two
    // default-level pnames; a null values pointer is ignored. The upper bound
    // MAX_PATCH_VERTICES is not enforced here (the tracker does not hold limits).
    void patchParameteri(GLenum pname, GLint value);
    void patchParameterfv(GLenum pname, const GLfloat* values);

    // --- Whole-framebuffer copy / invalidate (SPEC §15 / §16) ---
    // glBlitFramebuffer copies a rectangle of the bound read framebuffer into the
    // bound draw framebuffer; an invalid mask (bits outside color/depth/stencil)
    // reports GL_INVALID_VALUE honestly. glInvalidateFramebuffer /
    // glInvalidateSubFramebuffer discard the listed attachments (full or a
    // sub-rectangle); a null attachment pointer with a non-zero count reports
    // GL_INVALID_VALUE. The frontend flushes tracked state first.
    void blitFramebuffer(int32_t srcX0, int32_t srcY0, int32_t srcX1, int32_t srcY1,
                        int32_t dstX0, int32_t dstY0, int32_t dstX1, int32_t dstY1,
                        uint32_t mask, uint32_t filter);
    void invalidateFramebuffer(uint32_t target, int32_t numAttachments,
                             const uint32_t* attachments);
    void invalidateSubFramebuffer(uint32_t target, int32_t numAttachments,
                                 const uint32_t* attachments, int32_t x, int32_t y,
                                 int32_t width, int32_t height);

    // --- Command stream (SPEC §2.1) ---
    void flushCommands();
    void finishCommands();
    void memoryBarrier(uint32_t barriers);
    void memoryBarrierByRegion(uint32_t barriers);

    // --- Framebuffer readback (SPEC §2.1) ---
    // Reads pixels from the bound framebuffer (after a state flush). A non-positive
    // width/height reports GL_INVALID_VALUE honestly.
    void readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                    uint32_t format, uint32_t type, void* pixels);
    // Robust pixel readback (SPEC §18 / ARB_robustness): like readPixels but with
    // a byte-capacity guard on `pixels`. A non-positive width/height or a negative
    // `bufSize` reports GL_INVALID_VALUE; the backend truncates silently if the
    // buffer is smaller than the transfer (no error, per the robustness spec).
    void readnPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                     uint32_t format, uint32_t type, int32_t bufSize, void* pixels);

    // --- Pixel store (SPEC §10) ---
    // Records global pixel-store state in the tracker and pushes it to the
    // backend immediately (it affects subsequent texture/image uploads, SPEC §8.4).
    void pixelStorei(uint32_t pname, int param);
    void pixelStoref(uint32_t pname, float param);

    // --- Vertex arrays ---
    GLObjectName genVertexArray();
    void genVertexArrays(uint32_t n, GLObjectName* names);
    void bindVertexArray(GLObjectName name);
    GLObjectName boundVertexArray() const;
    void deleteVertexArray(GLObjectName name);
    void deleteVertexArrays(uint32_t n, const GLObjectName* names);
    VertexArrayObject* getVertexArray(GLObjectName name);
    bool isVertexArray(GLObjectName name) const;

    // Direct State Access vertex-array surface (SPEC §10.3.1). Capability-gated
    // by DirectStateAccess: each function operates on the named VAO's backend
    // resource directly, with no bound-VAO side effect. createVertexArrays is the
    // DSA allocator (equivalent to genVertexArrays here). The separate
    // attribute-format model (attrib <-> binding points) is emulated by recording
    // state on the frontend VAO and replaying it through the legacy flush path.
    void createVertexArrays(uint32_t n, GLObjectName* names);
    void vertexArrayElementBuffer(GLObjectName vao, GLObjectName buffer);
    void enableVertexArrayAttrib(GLObjectName vao, uint32_t index);
    void disableVertexArrayAttrib(GLObjectName vao, uint32_t index);
    void vertexArrayVertexBuffer(GLObjectName vao, uint32_t bindingindex,
                                 GLObjectName buffer, intptr_t offset,
                                 int32_t stride);
    void vertexArrayVertexBuffers(GLObjectName vao, uint32_t first, uint32_t count,
                                  const GLObjectName* buffers,
                                  const intptr_t* offsets, const int32_t* strides);
    void vertexArrayAttribFormat(GLObjectName vao, uint32_t attribindex,
                                 int32_t size, uint32_t type, bool normalized,
                                 uint32_t relativeoffset);
    void vertexArrayAttribIFormat(GLObjectName vao, uint32_t attribindex,
                                  int32_t size, uint32_t type,
                                  uint32_t relativeoffset);
    void vertexArrayAttribLFormat(GLObjectName vao, uint32_t attribindex,
                                  int32_t size, uint32_t type,
                                  uint32_t relativeoffset);
    void vertexArrayAttribBinding(GLObjectName vao, uint32_t attribindex,
                                  uint32_t bindingindex);
    void vertexArrayBindingDivisor(GLObjectName vao, uint32_t bindingindex,
                                   uint32_t divisor);

    // Separate attribute format on the *bound* VAO (SPEC §10.3.1/§10.3.2/§10.3.4,
    // ARB_vertex_attrib_binding). These are the non-DSA spellings of the
    // vertexArray* calls above and share their implementation; the vertex array
    // object is the one bound to GL_VERTEX_ARRAY_BINDING, so no VAO bound reports
    // GL_INVALID_OPERATION. Validation follows the spec: `attribindex` >=
    // MAX_VERTEX_ATTRIBS or `bindingindex` >= MAX_VERTEX_ATTRIB_BINDINGS ->
    // GL_INVALID_VALUE; negative `offset`/`stride` or `stride` >
    // MAX_VERTEX_ATTRIB_STRIDE -> GL_INVALID_VALUE. bindVertexBuffers validates
    // per binding point: an invalid entry leaves that binding point unchanged and
    // reports an error while the valid entries still apply; `count` < 0 ->
    // GL_INVALID_VALUE and `first + count` past MAX_VERTEX_ATTRIB_BINDINGS ->
    // GL_INVALID_OPERATION. A null `buffers` array resets the whole range to no
    // buffer with the spec's default offset 0 / stride 16.
    void bindVertexBuffer(uint32_t bindingindex, GLObjectName buffer,
                          intptr_t offset, int32_t stride);
    void bindVertexBuffers(uint32_t first, GLsizei count,
                           const GLObjectName* buffers, const intptr_t* offsets,
                           const int32_t* strides);
    void vertexAttribFormat(uint32_t attribindex, int32_t size, uint32_t type,
                            bool normalized, uint32_t relativeoffset);
    void vertexAttribIFormat(uint32_t attribindex, int32_t size, uint32_t type,
                             uint32_t relativeoffset);
    void vertexAttribLFormat(uint32_t attribindex, int32_t size, uint32_t type,
                             uint32_t relativeoffset);
    void vertexAttribBinding(uint32_t attribindex, uint32_t bindingindex);
    void vertexBindingDivisor(uint32_t bindingindex, uint32_t divisor);

    // --- Transform feedback (SPEC §13.3) ---
    // Capability-gated by TransformFeedback. gen/bind/delete manage the frontend
    // TF objects; begin/end/pause/resume drive capture and are validated (e.g.
    // begin while already active is GL_INVALID_OPERATION).
    GLObjectName genTransformFeedback();
    void genTransformFeedbacks(uint32_t n, GLObjectName* names);
    void createTransformFeedbacks(uint32_t n, GLObjectName* names);
    void bindTransformFeedback(GLObjectName name);
    GLObjectName boundTransformFeedback() const;
    void deleteTransformFeedback(GLObjectName name);
    void deleteTransformFeedbacks(uint32_t n, const GLObjectName* names);
    bool isTransformFeedback(GLObjectName name) const;
    TransformFeedbackObject* getTransformFeedback(GLObjectName name);
    void beginTransformFeedback(uint32_t primitiveMode);
    void endTransformFeedback();
    void pauseTransformFeedback();
    void resumeTransformFeedback();

    // Transform-feedback buffer bindings (SPEC §13.2.1 glTransformFeedbackBuffer-
    // Base/Range). `xfb == 0` operates on the currently bound TF object (the
    // default object when none is bound); a non-zero `xfb` is the name of a
    // generated TF object. The binding is recorded on the TF object and also
    // pushed to the backend as a GL_TRANSFORM_FEEDBACK_BUFFER base/range binding.
    void transformFeedbackBufferBase(GLObjectName xfb, uint32_t index,
                                    GLObjectName buffer);
    void transformFeedbackBufferRange(GLObjectName xfb, uint32_t index,
                                     GLObjectName buffer, intptr_t offset,
                                     intptr_t size);

    // --- Query objects (SPEC §4 / §19) ---
    // Capability-gated by Queries. gen/bind/delete manage the frontend query
    // objects; begin/end bracket a capture of the given target. A query cannot
    // be begun twice (already active) and end requires a matching active query.
    GLObjectName genQuery();
    void genQueries(uint32_t n, GLObjectName* names);
    void createQueries(uint32_t target, uint32_t n, GLObjectName* names);
    void deleteQuery(GLObjectName name);
    void deleteQueries(uint32_t n, const GLObjectName* names);
    bool isQuery(GLObjectName name) const;
    void beginQuery(uint32_t target, GLObjectName id);
    void endQuery(uint32_t target);
    void beginQueryIndexed(uint32_t target, uint32_t index, GLObjectName id);
    void endQueryIndexed(uint32_t target, uint32_t index);
    // Record a timestamp query that resolves once all prior GL commands have
    // completed (SPEC §4.2.1 glQueryCounter). `target` must be GL_TIMESTAMP; the
    // query must be a generated, non-active query object.
    void queryCounter(GLObjectName id, uint32_t target);
    QueryObject* getQuery(GLObjectName name);
    const QueryObject* getQuery(GLObjectName name) const;
    // Query parameter queries (SPEC §4 / §19, glGetQueryiv / glGetQueryObject*).
    // Frontend-owned values: CURRENT_QUERY reads the active query for `target`;
    // QUERY_RESULT / QUERY_RESULT_AVAILABLE read the cached result. Unknown
    // pname yields GL_INVALID_ENUM; an unknown query id yields GL_INVALID_OPERATION.
    void getQueryiv(uint32_t target, uint32_t pname, int32_t* params);
    void getQueryObjectiv(GLObjectName id, uint32_t pname, int32_t* params);
    void getQueryObjectuiv(GLObjectName id, uint32_t pname, uint32_t* params);
    void getQueryObjecti64v(GLObjectName id, uint32_t pname, int64_t* params);
    void getQueryObjectui64v(GLObjectName id, uint32_t pname, uint64_t* params);
    // Query result written into a buffer object (SPEC §4 / §19 /
    // ARB_query_buffer_object). The cached query result/availability is written
    // into the buffer's CPU mirror at `offset` (alignment + bounds checked) then
    // uploaded to the backend. Unknown pname -> GL_INVALID_ENUM; an ungenerated id
    // or buffer -> GL_INVALID_OPERATION; a misaligned or out-of-bounds offset ->
    // GL_INVALID_VALUE.
    void getQueryBufferObjectiv(GLObjectName id, GLObjectName buffer, uint32_t pname,
                                intptr_t offset);
    void getQueryBufferObjectuiv(GLObjectName id, GLObjectName buffer, uint32_t pname,
                                 intptr_t offset);
    void getQueryBufferObjecti64v(GLObjectName id, GLObjectName buffer, uint32_t pname,
                                  intptr_t offset);
    void getQueryBufferObjectui64v(GLObjectName id, GLObjectName buffer, uint32_t pname,
                                   intptr_t offset);

    // --- Sync objects (SPEC §4 / §20, ARB_sync) ---
    // Capability-gated by SyncObjects. fenceSync creates a GPU-commands-complete
    // fence and returns an opaque GLsync; clientWaitSync / waitSync order on it;
    // deleteSync / isSync / getSynciv manage it. The frontend owns the SyncObject
    // and never exposes the raw pointer to the backend (SPEC §3).
    GLsync fenceSync(uint32_t condition, uint32_t flags);
    GLenum clientWaitSync(GLsync sync, uint32_t flags, uint64_t timeout);
    void waitSync(GLsync sync, uint32_t flags, uint64_t timeout);
    void deleteSync(GLsync sync);
    bool isSync(GLsync sync) const;
    void getSynciv(GLsync sync, uint32_t pname, uint32_t bufSize, int32_t* length,
                   int32_t* values);

    // --- Sampler objects (SPEC §8.2) ---
    // Capability-gated by SamplerObjects. gen/bind/delete manage the frontend
    // sampler objects; samplerParameteri sets scalar sampler parameters and is
    // forwarded to the backend sampler resource. bindSampler binds a sampler to a
    // texture unit (pushed to the backend at flush time). An out-of-range unit
    // reports GL_INVALID_VALUE; an ungenerated sampler name reports
    // GL_INVALID_OPERATION honestly.
    GLObjectName genSampler();
    void genSamplers(uint32_t n, GLObjectName* names);
    void createSamplers(uint32_t n, GLObjectName* names);
    void bindSampler(uint32_t unit, GLObjectName sampler);
    // Multi-bind samplers (`glBindSamplers`, SPEC §8.2 / ARB_multi_bind). Binds
    // `count` samplers from `samplers` to consecutive units starting at `first`.
    // A null `samplers` array unbinds every touched unit. Capability-gated by
    // SamplerObjects; negative `count` reports GL_INVALID_VALUE and
    // `first + count` beyond MAX_COMBINED_TEXTURE_IMAGE_UNITS reports
    // GL_INVALID_OPERATION. Entries are validated per unit: an ungenerated
    // non-zero name leaves that unit unchanged and reports GL_INVALID_OPERATION
    // while the remaining valid entries are still bound.
    void bindSamplers(uint32_t first, GLsizei count, const GLObjectName* samplers);
    GLObjectName boundSampler(uint32_t unit) const;

    // Image units (SPEC §8.22 / §10.8.1, glBindImageTexture). Binds `texture` to
    // image unit `unit` (zero-based) with the given level/layered/layer/access/
    // format, validated and pushed at flush time. An out-of-range `unit` reports
    // GL_INVALID_VALUE; with a non-zero `texture`, a negative `level`/`layer`
    // reports GL_INVALID_VALUE, an invalid `access` reports GL_INVALID_ENUM (the
    // exhaustive `format` check is intentionally not performed). Binding 0 unbinds
    // the unit (the other params are then ignored). Capability-gated by ShaderImageLoadStore.
    void bindImageTexture(uint32_t unit, GLObjectName texture, GLint level,
                          GLboolean layered, GLint layer, GLenum access,
                          GLenum format);
    // Multi-bind image units (`glBindImageTextures`, SPEC §8.22 / ARB_multi_bind).
    // Binds `count` textures from `textures` to consecutive units starting at
    // `first`, each with the spec's multi-bind defaults. `count == 0` is a silent
    // no-op; a null `textures` array unbinds every touched unit; `first + count`
    // beyond MAX_IMAGE_UNITS reports GL_INVALID_VALUE.
    void bindImageTextures(uint32_t first, GLsizei count,
                           const GLObjectName* textures);
    GLObjectName boundImageTexture(uint32_t unit) const;
    void deleteSampler(GLObjectName name);
    void deleteSamplers(uint32_t n, const GLObjectName* names);
    SamplerObject* getSampler(GLObjectName name);
    const SamplerObject* getSampler(GLObjectName name) const;
    void samplerParameteri(GLObjectName sampler, uint32_t pname, int param);
    void samplerParameterf(GLObjectName sampler, uint32_t pname, float param);
    void samplerParameterfv(GLObjectName sampler, uint32_t pname,
                            const float* params, int count);
    void samplerParameterIiv(GLObjectName sampler, uint32_t pname,
                             const int32_t* params);
    void samplerParameterIuiv(GLObjectName sampler, uint32_t pname,
                              const uint32_t* params);
    void getSamplerParameteriv(GLObjectName sampler, uint32_t pname,
                               int32_t* params);
    void getSamplerParameterfv(GLObjectName sampler, uint32_t pname, float* params);
    void getSamplerParameterIiv(GLObjectName sampler, uint32_t pname,
                                int32_t* params);
    void getSamplerParameterIuiv(GLObjectName sampler, uint32_t pname,
                                 uint32_t* params);
    bool isSampler(GLObjectName name) const;
    // Capability-guarded: ShaderObjects / ProgramObjects must be supported by the
    // backend or these report GL_INVALID_OPERATION honestly. The desktop->backend
    // source translation (IShaderCompiler) runs inside compileShader so the
    // backend receives backend-compatible source.
    GLObjectName createShader(uint32_t stage);
    void shaderSource(GLObjectName shader, const std::string& src);
    void compileShader(GLObjectName shader);
    void specializeShader(GLObjectName shader, const std::string& entryPoint,
                          uint32_t numConstants, const uint32_t* constantIndex,
                          const uint32_t* constantValue);
    bool isShaderCompiled(GLObjectName shader) const;
    std::string shaderInfoLog(GLObjectName shader) const;
    // Query shader/program parameters (SPEC §7.3 / §7.14). Frontend-owned data
    // (type, source/info-log length, compile/link/delete status, attached shader
    // count) is returned directly; active uniform/attribute/block counts are
    // delegated to the backend resource. An unknown name sets GL_INVALID_ENUM.
    void getShaderiv(GLObjectName shader, uint32_t pname, GLint* params);
    void getProgramiv(GLObjectName program, uint32_t pname, GLint* params);
    // Convenience overloads returning the queried value directly.
    GLint getShaderiv(GLObjectName shader, uint32_t pname);
    GLint getProgramiv(GLObjectName program, uint32_t pname);
    // Reflection queries (SPEC §7.3.4 / §7.3.7).
    // glGetAttachedShaders fills `shaders` with up to `maxCount` attached shader
    // names and `count` with the actual number (clamped to maxCount). A negative
    // maxCount -> GL_INVALID_VALUE; a non-program object -> GL_INVALID_OPERATION.
    // glGetShaderSource returns the concatenated source (nul-terminated) in
    // `source`; `length` (optional) receives the character count excluding the
    // nul. A negative bufSize -> GL_INVALID_VALUE; a non-shader object ->
    // GL_INVALID_OPERATION.
    void getAttachedShaders(GLObjectName program, int32_t maxCount, int32_t* count,
                            GLObjectName* shaders);
    void getShaderSource(GLObjectName shader, int32_t bufSize, int32_t* length,
                         char* source);

    // --- Object labels (SPEC §22.2) ---
    // glObjectLabel assigns a debug label to the object `name` in the namespace
    // given by `identifier` (GL_BUFFER / GL_SHADER / GL_PROGRAM / GL_VERTEX_ARRAY /
    // GL_QUERY / GL_PROGRAM_PIPELINE / GL_TRANSFORM_FEEDBACK / GL_SAMPLER /
    // GL_TEXTURE / GL_RENDERBUFFER / GL_FRAMEBUFFER). `label` == nullptr clears the
    // label; a negative `length` means `label` is nul-terminated. The label is
    // limited to kMaxObjectLabelLength characters (GL_INVALID_VALUE beyond). An
    // unknown `identifier` is GL_INVALID_ENUM; a `name` that is not a live object of
    // that type is GL_INVALID_OPERATION.
    void objectLabel(uint32_t identifier, GLObjectName name, int32_t length,
                     const char* label);
    void getObjectLabel(uint32_t identifier, GLObjectName name, int32_t bufSize,
                        int32_t* length, char* label);
    // Pointer labels (SPEC §22.2, glObjectPtrLabel / glGetObjectPtrLabel) apply to
    // sync objects addressed by `ptr`. A null `ptr` is GL_INVALID_VALUE.
    void objectPtrLabel(const void* ptr, int32_t length, const char* label);
    void getObjectPtrLabel(const void* ptr, int32_t bufSize, int32_t* length,
                           char* label);

    // Program-interface reflection (SPEC §7.3.11). `programInterface` must be a
    // valid interface enum; `program` must be a linked program object. Name/Index
    // lookups that find nothing return GL_INVALID_INDEX / -1 honestly (no error);
    // out-of-range indices and bad buffers set GL_INVALID_VALUE; an unsupported
    // interface sets GL_INVALID_ENUM.
    uint32_t getProgramResourceIndex(GLObjectName program, uint32_t programInterface,
                                     const std::string& name);
    void getProgramResourceName(GLObjectName program, uint32_t programInterface,
                                uint32_t index, int32_t bufSize, int32_t* length,
                                char* name);
    void getProgramResourceiv(GLObjectName program, uint32_t programInterface,
                              uint32_t index, int32_t propCount, const uint32_t* props,
                              int32_t bufSize, int32_t* length, int32_t* params);
    int32_t getProgramResourceLocation(GLObjectName program, uint32_t programInterface,
                                       const std::string& name);
    int32_t getProgramResourceLocationIndex(GLObjectName program,
                                             uint32_t programInterface,
                                             const std::string& name);
    // Program-interface summary query (SPEC §7.3.1 glGetProgramInterfaceiv).
    // Writes the requested property for programInterface into params; an
    // unsupported interface sets GL_INVALID_ENUM, a null params sets
    // GL_INVALID_VALUE, and a program that is not linked / not a program object
    // sets GL_INVALID_OPERATION.
    void getProgramInterfaceiv(GLObjectName program, uint32_t programInterface,
                               uint32_t pname, int32_t* params);

    // Legacy uniform/attribute/uniform-block reflection (SPEC §7.6, §7.3.11).
    // These are defined by the spec as exact equivalents of the program-resource
    // queries above, so the frontend maps them onto the existing reflection
    // backend methods (UNIFORM / PROGRAM_INPUT / UNIFORM_BLOCK interfaces) without
    // needing new backend virtuals. `program` must be a linked program object
    // (else GL_INVALID_OPERATION); an out-of-range `index` reports
    // GL_INVALID_VALUE; a negative `bufSize` reports GL_INVALID_VALUE.
    void getActiveUniform(GLObjectName program, uint32_t index, int32_t bufSize,
                          int32_t* length, int32_t* size, uint32_t* type, char* name);
    void getActiveAttrib(GLObjectName program, uint32_t index, int32_t bufSize,
                         int32_t* length, int32_t* size, uint32_t* type, char* name);
    uint32_t getUniformBlockIndex(GLObjectName program, const std::string& name);
    void getActiveUniformBlockiv(GLObjectName program, uint32_t index, uint32_t pname,
                                 int32_t* params);
    void getActiveUniformName(GLObjectName program, uint32_t uniformIndex,
                               int32_t bufSize, int32_t* length, char* name);
    void getActiveUniformsiv(GLObjectName program, int32_t uniformCount,
                             const uint32_t* uniformIndices, uint32_t pname,
                             int32_t* params);
    void getActiveUniformBlockName(GLObjectName program, uint32_t index,
                                     int32_t bufSize, int32_t* length, char* name);
    void getUniformIndices(GLObjectName program, int32_t uniformCount,
                           const char* const* uniformNames, uint32_t* uniformIndices);
    // Bind a program's uniform block `blockIndex` to uniform-buffer binding point
    // `blockBinding` (SPEC §7.6.2 glUniformBlockBinding).
    void uniformBlockBinding(GLObjectName program, uint32_t blockIndex,
                             uint32_t blockBinding);

    // Associate a program's shader-storage block `blockIndex` with shader-storage-
    // buffer binding point `blockBinding` (SPEC §7.6.2 glShaderStorageBlockBinding).
    // Capability-gated by ShaderStorageBufferObjects.
    void shaderStorageBlockBinding(GLObjectName program, uint32_t blockIndex,
                                   uint32_t blockBinding);

    // Subroutine reflection + selection (SPEC §7.9). All require the `Subroutines`
    // capability; `program` (for the reflection getters) must be a linked program;
    // `shadertype` must be a valid subroutine stage. Name/Index lookups that find
    // nothing return GL_INVALID_INDEX / -1 honestly (no error).
    uint32_t getSubroutineIndex(GLObjectName program, uint32_t shadertype,
                                const std::string& name);
    int32_t getSubroutineUniformLocation(GLObjectName program, uint32_t shadertype,
                                         const std::string& name);
    void getActiveSubroutineUniformiv(GLObjectName program, uint32_t shadertype,
                                      uint32_t index, uint32_t pname,
                                      int32_t* values);
    void getActiveSubroutineUniformName(GLObjectName program, uint32_t shadertype,
                                       uint32_t index, int32_t bufSize,
                                       int32_t* length, char* name);
    void getActiveSubroutineName(GLObjectName program, uint32_t shadertype,
                                 uint32_t index, int32_t bufSize, int32_t* length,
                                 char* name);
    void uniformSubroutinesuiv(uint32_t shadertype, int32_t count,
                              const uint32_t* indices);
    void getUniformSubroutineuiv(uint32_t shadertype, int32_t location,
                                 uint32_t* params);
    void getProgramStageiv(GLObjectName program, uint32_t shadertype,
                           uint32_t pname, int32_t* values);

    // Retrieve the info/debug log (SPEC §7.3 / §7.14). Copies up to bufSize-1
    // characters into `infoLog` (nul-terminated); `*length` receives the number
    // of characters written, excluding the nul. An unknown object sets
    // GL_INVALID_OPERATION and writes nothing.
    void getShaderInfoLog(GLObjectName shader, uint32_t bufSize, int32_t* length,
                         char* infoLog);
    void getProgramInfoLog(GLObjectName program, uint32_t bufSize, int32_t* length,
                          char* infoLog);
    void deleteShader(GLObjectName shader);
    bool isShader(GLObjectName name) const;
    ShaderObject* getShader(GLObjectName name);
    const ShaderObject* getShader(GLObjectName name) const;

    GLObjectName createProgram();
    void attachShader(GLObjectName program, GLObjectName shader);
    // Detach `shader` from `program` (SPEC §7.4 glDetachShader). Does not undo a
    // successful link; removes the frontend association and forwards to the
    // backend program. A non-program or non-shader name reports GL_INVALID_OPERATION.
    void detachShader(GLObjectName program, GLObjectName shader);
    void linkProgram(GLObjectName program);
    // Validate the linked program against current GL state (SPEC §7.3
    // glValidateProgram). Marks the program validated and forwards to the backend.
    // A null/unknown or non-program object -> GL_INVALID_OPERATION.
    void validateProgram(GLObjectName program);
    // Set program parameters before/after linking (SPEC §7.3 / §7.4.2).
    // GL_PROGRAM_SEPARABLE must be set before linking (after link ->
    // GL_INVALID_OPERATION); GL_PROGRAM_BINARY_RETRIEVABLE_HINT may be set at
    // any time. Unknown pname -> GL_INVALID_ENUM; non-program -> GL_INVALID_OPERATION.
    void programParameteri(GLObjectName program, uint32_t pname, int32_t value);
    // Program binary (SPEC §7.3 / §19.1). glProgramBinary loads a precompiled blob
    // and marks the program linked; glGetProgramBinary retrieves the frontend's
    // authoritative binary mirror. An unknown program -> GL_INVALID_OPERATION;
    // length < 0 -> GL_INVALID_VALUE; binaryFormat == 0 -> GL_INVALID_ENUM.
    // glGetProgramBinary reports GL_INVALID_OPERATION if no binary is retrievable.
    void programBinary(GLObjectName program, uint32_t binaryFormat, const void* binary,
                       GLsizei length);
    void getProgramBinary(GLObjectName program, GLsizei bufSize, GLsizei* length,
                          uint32_t* binaryFormat, void* binary);
    // Shader binary (SPEC §7.2). Loads a SPIR-V / vendor binary into each named
    // shader and marks it compiled. count < 0 -> GL_INVALID_VALUE; binaryFormat ==
    // 0 -> GL_INVALID_ENUM; an unknown shader -> GL_INVALID_OPERATION.
    void shaderBinary(GLsizei count, const GLuint* shaders, uint32_t binaryFormat,
                      const void* binary, GLsizei length);
    bool isProgramLinked(GLObjectName program) const;
    std::string programInfoLog(GLObjectName program) const;
    int getAttribLocation(GLObjectName program, const std::string& name) const;
    // Fragment-output reflection (SPEC §7.3.6). Returns the location / dual-source
    // index bound to the fragment-shader output `name`. A non-program name reports
    // GL_INVALID_OPERATION; an unknown output returns -1 (no error). Delegated to the
    // backend program.
    int getFragDataLocation(GLObjectName program, const std::string& name) const;
    int getFragDataIndex(GLObjectName program, const std::string& name) const;
    void getTransformFeedbackVarying(GLObjectName program, uint32_t index, int bufSize,
                                     int* length, int* size, uint32_t* type,
                                     char* name) const;
    // Bind a generic vertex attribute index to an attribute variable name before
    // linking (SPEC §7.3.7 glBindAttribLocation). Records the request on the
    // program; applied to the backend at the next linkProgram. An unknown program
    // reports GL_INVALID_OPERATION honestly.
    void bindAttribLocation(GLObjectName program, uint32_t index,
                            const std::string& name);
    // Bind a user-defined fragment shader output variable name to a fragment color
    // number (and dual-source index for the indexed form) before linking (SPEC
    // §7.3.7 / §15.1.2 glBindFragDataLocation / glBindFragDataLocationIndexed).
    // Recorded on the program; applied to the backend at the next linkProgram.
    // Validation (mirrors the spec): a shader-object name -> GL_INVALID_OPERATION;
    // a name that is neither a program nor shader -> GL_INVALID_VALUE; index > 1 ->
    // GL_INVALID_VALUE; colorNumber >= GLStateTracker::kMaxDrawBuffers ->
    // GL_INVALID_VALUE; a name with the reserved "gl_" prefix -> GL_INVALID_OPERATION.
    void bindFragDataLocation(GLObjectName program, uint32_t colorNumber,
                             const std::string& name);
    void bindFragDataLocationIndexed(GLObjectName program, uint32_t colorNumber,
                                     uint32_t index, const std::string& name);
    // Specify the transform-feedback varyings captured when this program is the
    // active program of a transform-feedback begin (SPEC §13.3.1
    // glTransformFeedbackVaryings). Must be set before linking (after link ->
    // GL_INVALID_OPERATION). `count < 0` -> GL_INVALID_VALUE; `bufferMode` must be
    // GL_INTERLEAVED_ATTRIBS or GL_SEPARATE_ATTRIBS (else GL_INVALID_ENUM); an
    // unknown program -> GL_INVALID_OPERATION. Records the request and applies it
    // to the backend program at the next linkProgram.
    void transformFeedbackVaryings(GLObjectName program, GLsizei count,
                                   const char* const* varyings, uint32_t bufferMode);

    void deleteProgram(GLObjectName program);
    bool isProgram(GLObjectName name) const;
    ProgramObject* getProgram(GLObjectName name);
    const ProgramObject* getProgram(GLObjectName name) const;

    // --- Program pipelines (SPEC §7.4) ---
    // Capability-gated by ProgramPipelines. Gen/delete/is manage names
    // unconditionally (like other object families); the stateful operations
    // report GL_INVALID_OPERATION when the feature is unsupported. The frontend
    // owns the stage->program mapping and answers glGetProgramPipelineiv; the
    // bound pipeline is forwarded to the backend via GLStateSink so backends with
    // separable-program support can install it (GLES cannot consume it for
    // drawing, so its ProgramPipelines support is Emulated only where separable
    // programs exist, else Unsupported).
    GLObjectName createShaderProgramv(uint32_t type, int32_t count,
                                      const char* const* strings);
    void genProgramPipelines(uint32_t n, GLObjectName* names);
    void createProgramPipelines(uint32_t n, GLObjectName* names);
    void deleteProgramPipelines(uint32_t n, const GLObjectName* names);
    bool isProgramPipeline(GLObjectName name) const;
    void bindProgramPipeline(GLObjectName pipeline);
    void useProgramStages(GLObjectName pipeline, uint32_t stages,
                          GLObjectName program);
    void activeShaderProgram(GLObjectName pipeline, GLObjectName program);
    void getProgramPipelineiv(GLObjectName pipeline, uint32_t pname,
                              int32_t* params);
    void validateProgramPipeline(GLObjectName pipeline);
    void getProgramPipelineInfoLog(GLObjectName pipeline, uint32_t bufSize,
                                   int32_t* length, char* infoLog);
    ProgramPipelineObject* getProgramPipeline(GLObjectName name);
    const ProgramPipelineObject* getProgramPipeline(GLObjectName name) const;

    // --- Vertex attributes (SPEC §2.1) ---
    // Operate on the currently bound VAO (glBindVertexArray); with no VAO bound
    // they report GL_INVALID_OPERATION. State is recorded on the VAO and pushed
    // to the backend (via GLStateSink) at draw / flush time.
    void enableVertexAttribArray(uint32_t index);
    void disableVertexAttribArray(uint32_t index);
    void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                             bool normalized, int32_t stride, intptr_t offset);
    // Per-attribute divisor (SPEC §10, glVertexAttribDivisor). 0 advances once
    // per vertex (default); >0 advances once per `divisor` instances. Pushed at
    // flush time only when non-zero (SPEC §10: no redundant native call).
    void vertexAttribDivisor(uint32_t index, uint32_t divisor);

    // Current generic vertex attribute values (SPEC §10.2). These feed a shader
    // attribute when it is disabled (not array-sourced). Recorded on the bound
    // VAO; no VAO bound -> GL_INVALID_OPERATION. Index >= kMaxVertexAttribs ->
    // GL_INVALID_VALUE. Float family sets currentType = GL_FLOAT; integer family
    // (I) sets GL_INT / GL_UNSIGNED_INT and stores integral values.
    void vertexAttrib1f(uint32_t index, float x);
    void vertexAttrib2f(uint32_t index, float x, float y);
    void vertexAttrib3f(uint32_t index, float x, float y, float z);
    void vertexAttrib4f(uint32_t index, float x, float y, float z, float w);
    void vertexAttrib1fv(uint32_t index, const float* v);
    void vertexAttrib2fv(uint32_t index, const float* v);
    void vertexAttrib3fv(uint32_t index, const float* v);
    void vertexAttrib4fv(uint32_t index, const float* v);
    void vertexAttribI4i(uint32_t index, int32_t x, int32_t y, int32_t z, int32_t w);
    void vertexAttribI4ui(uint32_t index, uint32_t x, uint32_t y, uint32_t z, uint32_t w);
    void vertexAttribI4iv(uint32_t index, const int32_t* v);
    void vertexAttribI4uiv(uint32_t index, const uint32_t* v);
    // Query per-attribute state (SPEC §10.4), read from the bound VAO.
    // fv returns CURRENT_VERTEX_ATTRIB as float[4]; dv as double[4].
    // iv answers the integer/bool array pnames (ENABLED/SIZE/STRIDE/TYPE/
    //   NORMALIZED/INTEGER/DIVISOR/BUFFER_BINDING, plus CURRENT_VERTEX_ATTRIB).
    // Iiv/Iuiv return CURRENT_VERTEX_ATTRIB (signed/unsigned) and
    //   VERTEX_ATTRIB_ARRAY_INTEGER. Pointerv returns VERTEX_ATTRIB_ARRAY_POINTER.
    // Unknown pname -> GL_INVALID_ENUM; null params -> GL_INVALID_VALUE; no bound
    // VAO -> GL_INVALID_OPERATION; index >= max -> GL_INVALID_VALUE.
    void getVertexAttribfv(uint32_t index, GLenum pname, float* params);
    void getVertexAttribiv(uint32_t index, GLenum pname, int32_t* params);
    void getVertexAttribdv(uint32_t index, GLenum pname, double* params);
    void getVertexAttribIiv(uint32_t index, GLenum pname, int32_t* params);
    void getVertexAttribIuiv(uint32_t index, GLenum pname, uint32_t* params);
    void getVertexAttribPointerv(uint32_t index, GLenum pname, void** params);

    // DSA vertex-array queries (SPEC §10.3.1), operate on an explicit VAO name.
    // getVertexArrayiv reads VAO-level state (ELEMENT_ARRAY_BUFFER_BINDING).
    // getVertexArrayIndexediv reads per-attribute int state (ENABLED/SIZE/STRIDE/
    //   TYPE/NORMALIZED/INTEGER/LONG/DIVISOR/BUFFER_BINDING).
    // getVertexArrayIndexed64iv reads 64-bit per-attribute binding state
    //   (VERTEX_ATTRIB_BINDING, VERTEX_ATTRIB_RELATIVE_OFFSET).
    // Capability-gated by DirectStateAccess. Ungenerated VAO name ->
    //   GL_INVALID_OPERATION; index >= max -> GL_INVALID_VALUE; null params ->
    //   GL_INVALID_VALUE; unknown pname -> GL_INVALID_ENUM.
    void getVertexArrayiv(GLObjectName vao, uint32_t pname, int32_t* params);
    void getVertexArrayIndexediv(GLObjectName vao, uint32_t index, uint32_t pname,
                                 int32_t* params);
    void getVertexArrayIndexed64iv(GLObjectName vao, uint32_t index, uint32_t pname,
                                  int64_t* params);

    // --- Hints (SPEC §21.1.1) ---
    // Quality hint for a target. Invalid target or mode -> GL_INVALID_ENUM. If a
    // different mode is requested the change is pushed to the backend at flush.
    void hint(uint32_t target, uint32_t mode);
    uint32_t getHint(uint32_t target);

    // --- Conditional rendering (SPEC §10.11) ---
    // Begin/end a conditional-render region predicated on an existing query
    // object. Capability-gated by ConditionalRendering. Already-active region, a
    // non-query `id`, an active query, a query of disallowed type, or an invalid
    // `mode` -> GL_INVALID_OPERATION/GL_INVALID_ENUM. The region is forwarded to
    // the backend's GLStateSink immediately (it delimits a draw region, like
    // begin/end query, not deferred pipeline state).
    void beginConditionalRender(GLObjectName id, uint32_t mode);
    void endConditionalRender();
    bool conditionalRenderActive() const { return conditionalRenderActive_; }

    // --- Draw (SPEC §2.1) ---
    // Flush tracked pipeline state to the backend, then issue the draw. Drawing
    // with no active program is GL_INVALID_OPERATION (core profile). Instanced
    // draws consult the capability table and report unsupported honestly.
    void drawArrays(uint32_t mode, int32_t first, int32_t count);
    void drawElements(uint32_t mode, int32_t count, uint32_t type,
                      intptr_t indices);
    void drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                             int32_t primcount);
    void drawElementsInstanced(uint32_t mode, int32_t count, uint32_t type,
                                    intptr_t indices, int32_t primcount);
    void drawArraysInstancedBaseInstance(uint32_t mode, int32_t first, int32_t count,
                                        int32_t primcount, uint32_t baseinstance);
    void drawElementsInstancedBaseInstance(uint32_t mode, int32_t count, uint32_t type,
                                          intptr_t indices, int32_t primcount,
                                          uint32_t baseinstance);
    void drawElementsInstancedBaseVertexBaseInstance(uint32_t mode, int32_t count,
                                                    uint32_t type, intptr_t indices,
                                                    int32_t primcount, int32_t basevertex,
                                                    uint32_t baseinstance);
    // Draw expansion (SPEC §10). Each flushes tracked state first (like the
    // single-draw calls) and, for non-instanced variants, requires an active
    // program. `drawElementsBaseVertex` consults Feature::DrawElementsBaseVertex.
    void multiDrawArrays(uint32_t mode, const int32_t* firsts,
                         const int32_t* counts, int32_t drawcount);
    void multiDrawElements(uint32_t mode, const int32_t* counts, uint32_t type,
                          const intptr_t* indices, int32_t drawcount);
    // Multi-draw with per-draw instance counts and a per-draw base instance
    // (SPEC §10, GL 4.6). Require Feature::MultiDraw and Feature::BaseInstance (else
    // GL_INVALID_OPERATION), a non-negative `drawcount` (else GL_INVALID_VALUE), and
    // an active program (else GL_INVALID_OPERATION); flush tracked state, then issue
    // the backend draw. `instanceCounts` may be null (every draw is a single instance).
    void multiDrawArraysBaseInstance(uint32_t mode, const int32_t* firsts,
                                     const int32_t* counts,
                                     const int32_t* instanceCounts,
                                     const uint32_t* baseInstances,
                                     int32_t drawcount);
    void multiDrawElementsBaseInstance(uint32_t mode, const int32_t* counts,
                                      uint32_t type, const intptr_t* indices,
                                      const uint32_t* baseInstances,
                                      int32_t drawcount);
    void drawRangeElements(uint32_t mode, uint32_t start, uint32_t end,
                           int32_t count, uint32_t type, intptr_t indices);
    void drawElementsBaseVertex(uint32_t mode, int32_t count, uint32_t type,
                                intptr_t indices, int32_t basevertex);
    // Base-instance draws (SPEC §10, ARB_base_instance / GL 4.2). Extend the
    // instanced draws with `baseinstance` (per-instance attribute offset). Require
    // Feature::BaseInstance (else GL_INVALID_OPERATION) and an active program (else
    // GL_INVALID_OPERATION); flush tracked state, then issue the backend draw.
    // Indirect draw (SPEC §10). Requires Feature::IndirectDrawing (else
    // GL_INVALID_OPERATION), an active program (else GL_INVALID_OPERATION), and a
    // buffer bound to GL_DRAW_INDIRECT_BUFFER (else GL_INVALID_OPERATION). `offset`
    // is the byte offset into that bound buffer; the frontend flushes tracked state
    // then issues the native indirect draw.
    void drawArraysIndirect(uint32_t mode, const void* offset);
    void drawElementsIndirect(uint32_t mode, uint32_t type, const void* offset);
    // Multi-draw indirect (SPEC §10, ARB_multi_draw_indirect). Same preconditions as
    // the single indirect draws; issues `drawcount` indirect commands from the bound
    // GL_DRAW_INDIRECT_BUFFER at `offset`, each `stride` bytes apart (stride 0 = tightly
    // packed). Requires Feature::IndirectDrawing + an active program + the indirect buffer.
    void multiDrawArraysIndirect(uint32_t mode, const void* offset, int32_t drawcount,
                                int32_t stride);
    void multiDrawElementsIndirect(uint32_t mode, uint32_t type, const void* offset,
                                 int32_t drawcount, int32_t stride);

    // Transform-feedback draws (SPEC §13.3.3). Draw `id`'s captured vertex count
    // (resolved from the object's backend via getCapturedVertexCount). Requires an
    // active program (else GL_INVALID_OPERATION). `id` must name a transform-feedback
    // object (else GL_INVALID_OPERATION); drawing while feedback is active and not
    // paused is a feedback loop (GL_INVALID_OPERATION). The stream variants draw the
    // captured count of a specific feedback `stream`.
    void drawTransformFeedback(uint32_t mode, GLObjectName id);
    void drawTransformFeedbackInstanced(uint32_t mode, GLObjectName id,
                                       int32_t primcount);
    void drawTransformFeedbackStream(uint32_t mode, GLObjectName id, uint32_t stream);
    void drawTransformFeedbackStreamInstanced(uint32_t mode, GLObjectName id,
                                             uint32_t stream, int32_t primcount);

    // Compute dispatch (SPEC §7.4). Requires Feature::ComputeShaders (else
    // GL_INVALID_OPERATION) and an active program (else GL_INVALID_OPERATION).
    // `dispatchComputeIndirect` additionally requires a buffer bound to
    // GL_DISPATCH_INDIRECT_BUFFER (else GL_INVALID_OPERATION). `offset` is the byte
    // offset into that bound buffer; the frontend flushes tracked state then issues
    // the native dispatch.
    void dispatchCompute(uint32_t x, uint32_t y, uint32_t z);
    void dispatchComputeIndirect(uintptr_t offset);


    // --- Uniforms (SPEC §8) ---
    // Query a uniform location for an explicit program. Setting uniforms operates
    // on the currently active program (glUseProgram). No active program or a
    // non-linked program yields GL_INVALID_OPERATION; a -1 location is a silent
    // no-op (standard glUniform* semantics).
    int getUniformLocation(GLObjectName program, const std::string& name);
    void uniform1f(int loc, float v0);
    void uniform2f(int loc, float v0, float v1);
    void uniform3f(int loc, float v0, float v1, float v2);
    void uniform4f(int loc, float v0, float v1, float v2, float v3);
    void uniform1i(int loc, int v0);
    void uniform2i(int loc, int v0, int v1);
    void uniform3i(int loc, int v0, int v1, int v2);
    void uniform4i(int loc, int v0, int v1, int v2, int v3);
    void uniform1fv(int loc, const float* v, int count);
    void uniform1iv(int loc, const int* v, int count);
    void uniformMatrix4fv(int loc, const float* m, int count, bool transpose);
    // Double-precision uniform setters (SPEC §8). ES backends implement these as
    // no-ops (GLSL ES has no double uniforms); the frontend still validates.
    void uniform1d(int loc, double v0);
    void uniform2d(int loc, double v0, double v1);
    void uniform3d(int loc, double v0, double v1, double v2);
    void uniform4d(int loc, double v0, double v1, double v2, double v3);
    void uniform1dv(int loc, const double* v, int count);
    void uniform2dv(int loc, const double* v, int count);
    void uniform3dv(int loc, const double* v, int count);
    void uniform4dv(int loc, const double* v, int count);
    // Unsigned-integer uniform setters (SPEC §8).
    void uniform1ui(int loc, uint32_t v0);
    void uniform2ui(int loc, uint32_t v0, uint32_t v1);
    void uniform3ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2);
    void uniform4ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3);
    void uniform1uiv(int loc, const uint32_t* v, int count);
    void uniform2uiv(int loc, const uint32_t* v, int count);
    void uniform3uiv(int loc, const uint32_t* v, int count);
    void uniform4uiv(int loc, const uint32_t* v, int count);
    // Remaining vector setters (SPEC §8).
    void uniform2fv(int loc, const float* v, int count);
    void uniform3fv(int loc, const float* v, int count);
    void uniform4fv(int loc, const float* v, int count);
    void uniform2iv(int loc, const int* v, int count);
    void uniform3iv(int loc, const int* v, int count);
    void uniform4iv(int loc, const int* v, int count);
    void uniformMatrix2fv(int loc, const float* m, int count, bool transpose);
    void uniformMatrix3fv(int loc, const float* m, int count, bool transpose);
    void uniformMatrix2dv(int loc, const double* m, int count, bool transpose);
    void uniformMatrix3dv(int loc, const double* m, int count, bool transpose);
    void uniformMatrix4dv(int loc, const double* m, int count, bool transpose);

    // --- Program uniform value queries (SPEC §7.9 glGetUniform{f,i,ui,d}v) ---
    // Read back a uniform value from a successfully linked program. The program
    // must be linked (GL_INVALID_OPERATION otherwise); a -1 location generates
    // GL_INVALID_OPERATION; a null params generates GL_INVALID_VALUE.
    void getUniformfv(GLObjectName program, int loc, float* params);
    void getUniformiv(GLObjectName program, int loc, int32_t* params);
    void getUniformuiv(GLObjectName program, int loc, uint32_t* params);
    void getUniformdv(GLObjectName program, int loc, double* params);

    // Robust (GL4.5 ARB_robustness) bounds-checked variants. `bufSize` is the
    // maximum size in basic machine units of the params buffer; a negative
    // bufSize generates GL_INVALID_VALUE.
    void getnUniformfv(GLObjectName program, int loc, int32_t bufSize, float* params);
    void getnUniformiv(GLObjectName program, int loc, int32_t bufSize, int32_t* params);
    void getnUniformuiv(GLObjectName program, int loc, int32_t bufSize, uint32_t* params);
    void getnUniformdv(GLObjectName program, int loc, int32_t bufSize, double* params);

    // --- Program uniforms (SPEC §7.9, glProgramUniform*) ---
    // Like the glUniform* setters but target an explicit program rather than the
    // active one. The program must be a successfully linked program object
    // (GL_INVALID_OPERATION otherwise); a -1 location is a silent no-op.
    void programUniform1f(GLObjectName program, int loc, float v0);
    void programUniform2f(GLObjectName program, int loc, float v0, float v1);
    void programUniform3f(GLObjectName program, int loc, float v0, float v1, float v2);
    void programUniform4f(GLObjectName program, int loc, float v0, float v1, float v2, float v3);
    void programUniform1i(GLObjectName program, int loc, int v0);
    void programUniform2i(GLObjectName program, int loc, int v0, int v1);
    void programUniform3i(GLObjectName program, int loc, int v0, int v1, int v2);
    void programUniform4i(GLObjectName program, int loc, int v0, int v1, int v2, int v3);
    void programUniform1fv(GLObjectName program, int loc, const float* v, int count);
    void programUniform1iv(GLObjectName program, int loc, const int* v, int count);
    void programUniformMatrix4fv(GLObjectName program, int loc, const float* m, int count, bool transpose);
    void programUniform1d(GLObjectName program, int loc, double v0);
    void programUniform2d(GLObjectName program, int loc, double v0, double v1);
    void programUniform3d(GLObjectName program, int loc, double v0, double v1, double v2);
    void programUniform4d(GLObjectName program, int loc, double v0, double v1, double v2, double v3);
    void programUniform1dv(GLObjectName program, int loc, const double* v, int count);
    void programUniform2dv(GLObjectName program, int loc, const double* v, int count);
    void programUniform3dv(GLObjectName program, int loc, const double* v, int count);
    void programUniform4dv(GLObjectName program, int loc, const double* v, int count);
    void programUniform1ui(GLObjectName program, int loc, uint32_t v0);
    void programUniform2ui(GLObjectName program, int loc, uint32_t v0, uint32_t v1);
    void programUniform3ui(GLObjectName program, int loc, uint32_t v0, uint32_t v1, uint32_t v2);
    void programUniform4ui(GLObjectName program, int loc, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3);
    void programUniform1uiv(GLObjectName program, int loc, const uint32_t* v, int count);
    void programUniform2uiv(GLObjectName program, int loc, const uint32_t* v, int count);
    void programUniform3uiv(GLObjectName program, int loc, const uint32_t* v, int count);
    void programUniform4uiv(GLObjectName program, int loc, const uint32_t* v, int count);
    void programUniform2fv(GLObjectName program, int loc, const float* v, int count);
    void programUniform3fv(GLObjectName program, int loc, const float* v, int count);
    void programUniform4fv(GLObjectName program, int loc, const float* v, int count);
    void programUniform2iv(GLObjectName program, int loc, const int* v, int count);
    void programUniform3iv(GLObjectName program, int loc, const int* v, int count);
    void programUniform4iv(GLObjectName program, int loc, const int* v, int count);
    void programUniformMatrix2fv(GLObjectName program, int loc, const float* m, int count, bool transpose);
    void programUniformMatrix3fv(GLObjectName program, int loc, const float* m, int count, bool transpose);
    void programUniformMatrix2dv(GLObjectName program, int loc, const double* m, int count, bool transpose);
    void programUniformMatrix3dv(GLObjectName program, int loc, const double* m, int count, bool transpose);
    void programUniformMatrix4dv(GLObjectName program, int loc, const double* m, int count, bool transpose);

    // --- State queries (SPEC §22) ---
    // Read tracked pipeline state (the frontend owns these values, so glGet
    // never queries the backend driver, SPEC §10). An unknown pname sets
    // GL_INVALID_ENUM; a null `params` sets GL_INVALID_VALUE.
    void getBooleanv(uint32_t pname, unsigned char* params);
    void getIntegerv(uint32_t pname, int32_t* params);
    void getFloatv(uint32_t pname, float* params);
    void getDoublev(uint32_t pname, double* params);
    // 64-bit variant of getIntegerv (SPEC §22.1): returns the same frontend-owned
    // integer state widened to GLint64. Unknown pname -> GL_INVALID_ENUM; null
    // params -> GL_INVALID_VALUE.
    void getInteger64v(uint32_t pname, int64_t* params);
    // Indexed scalar queries (SPEC §22.1). Only the indexed capabilities
    // GL_BLEND / GL_SCISSOR_TEST are supported with `index` < kMaxIndexedBuffers;
    // other pnames set GL_INVALID_ENUM, an out-of-range `index` sets
    // GL_INVALID_VALUE, and a null `params` sets GL_INVALID_VALUE.
    void getIntegeri_v(uint32_t pname, uint32_t index, int32_t* params);
    void getBooleani_v(uint32_t pname, uint32_t index, unsigned char* params);
    // Typed variants of the indexed-state query (SPEC §22.3): glGetFloati_v /
    // glGetDoublei_v / glGetInteger64i_v. They share the indexed-enable-cap
    // protocol with getIntegeri_v / getBooleani_v (BLEND / SCISSOR_TEST per draw
    // buffer); an unknown pname -> GL_INVALID_ENUM, an out-of-range `index` ->
    // GL_INVALID_VALUE, and a null `params` -> GL_INVALID_VALUE.
    void getFloati_v(uint32_t pname, uint32_t index, float* params);
    void getDoublei_v(uint32_t pname, uint32_t index, double* params);
    void getInteger64i_v(uint32_t pname, uint32_t index, int64_t* params);
    // Returns the current graphics-reset status (SPEC §22.5). This frontend has no
    // reset-detection path, so it always reports GL_NO_ERROR.
    GLenum getGraphicsResetStatus();
    // Returns true iff `cap` is an enabled, tracked capability; an untracked cap
    // sets GL_INVALID_ENUM and returns false (mirrors desktop GL glIsEnabled).
    bool isEnabled(uint32_t cap);

    // Indexed capability variants (SPEC §10.3.1). `cap` must be an indexable
    // capability (GL_BLEND, GL_SCISSOR_TEST) else GL_INVALID_ENUM; `index` must be
    // < kMaxIndexedBuffers else GL_INVALID_VALUE.
    void enableIndexed(uint32_t cap, uint32_t index);
    void disableIndexed(uint32_t cap, uint32_t index);
    bool isEnabledIndexed(uint32_t cap, uint32_t index);

    // ---- Debug messaging (SPEC §20.4) / debug groups (SPEC §20.5) ----
    // Install a debug callback; `userParam` is forwarded to every invocation.
    // Passing null disables callback invocation (messages are still filtered and
    // logged for glGetDebugMessageLog).
    void debugMessageCallback(GLDEBUGPROC callback, const void* userParam);
    // Enable/disable message generation for the (source, type, severity) space.
    // GL_DONT_CARE on any of source/type/severity acts as a wildcard. When `count`
    // > 0, `ids` selects specific message ids within the matched (source, type)
    // and `enabled` applies per-id, overriding the space-wide filter.
    void debugMessageControl(GLenum source, GLenum type, GLenum severity,
                             GLsizei count, const GLuint* ids, GLboolean enabled);
    // Generate an application message (SPEC §20.4 glDebugMessageInsert). The
    // message passes through the control filter; if enabled it is delivered to
    // the callback (if installed) and appended to the retrievable log.
    void debugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* buf);
    // Retrieve and clear up to `count` logged messages (SPEC §20.4
    // glGetDebugMessageLog). Returns the number of messages retrieved. Each entry
    // is written to the parallel arrays; `messageLog` receives the concatenated
    // NUL-terminated messages, with `lengths[i]` giving each length (excl. NUL).
    GLuint getDebugMessageLog(GLuint count, GLsizei bufSize, GLenum* sources,
                              GLenum* types, GLuint* ids, GLenum* severities,
                              GLsizei* lengths, GLchar* messageLog);
    // Push/pop a named debug group (SPEC §20.5). Push emits a PUSH_GROUP message;
    // pop emits a POP_GROUP message and underflow sets GL_STACK_UNDERFLOW.
    void pushDebugGroup(GLenum source, GLuint id, GLsizei length,
                       const GLchar* message);
    void popDebugGroup();
    // Current debug group stack depth (0 when outside any group).
    size_t debugGroupDepth() const { return debugGroups_.size(); }

private:
    // Shared body for glGetQueryObject* (SPEC §4): reads the cached result /
    // availability from the backend query resource into the requested width/sign.
    void getQueryObjectImpl(GLObjectName id, uint32_t pname, void* params, bool is64,
                            bool isSigned);    // Shared body for glGetQueryBufferObject* (SPEC §4 / ARB_query_buffer_object):
    // writes the cached query result/availability into a buffer object's CPU
    // mirror at `offset` (alignment + bounds checked) then uploads to the backend.
    void getQueryBufferObjectImpl(GLObjectName id, GLObjectName buffer, uint32_t pname,
                                  intptr_t offset, bool is64, bool isSigned);

    // Object-label support (SPEC §22.2). objectHasType reports whether `name` is a
    // live object in the namespace given by `identifier`.
    bool objectHasType(uint32_t identifier, GLObjectName name) const;

    // Shared validation/apply for the indexed buffer-binding commands
    // (SPEC §6.1.1). checkIndexedBufferTarget classifies the target (enum vs
    // honest capability gap), checkIndexedBufferBinding validates one binding
    // point's parameters, and applyIndexedBufferBinding records the binding and
    // pushes it to the backend. bindBuffersImpl is the multi-bind body shared by
    // glBindBuffersBase / glBindBuffersRange.
    GLError checkIndexedBufferTarget(uint32_t target) const;
    GLError checkIndexedBufferBinding(uint32_t index, GLObjectName buffer,
                                      intptr_t offset, intptr_t size,
                                      bool range) const;
    void applyIndexedBufferBinding(uint32_t target, uint32_t index,
                                   GLObjectName buffer, intptr_t offset,
                                   intptr_t size, bool range);
    void bindBuffersImpl(uint32_t target, uint32_t first, GLsizei count,
                         const GLObjectName* buffers, const intptr_t* offsets,
                         const intptr_t* sizes, bool range);

    // Shared bodies for the separate attribute-format commands (SPEC §10.3.2/
    // §10.3.4). The DSA (`glVertexArray*`) and non-DSA (`glBindVertexBuffer* /
    // glVertexAttrib*`) spellings differ only in how the vertex array object is
    // selected, so both resolve a VAO and then call these.
    void bindVertexBufferImpl(VertexArrayObject& vao, uint32_t bindingindex,
                              GLObjectName buffer, intptr_t offset,
                              int32_t stride);
    void bindVertexBuffersImpl(VertexArrayObject& vao, uint32_t first,
                               GLsizei count, const GLObjectName* buffers,
                               const intptr_t* offsets, const int32_t* strides);
    void vertexAttribBindingImpl(VertexArrayObject& vao, uint32_t attribindex,
                                 uint32_t bindingindex);
    void vertexBindingDivisorImpl(VertexArrayObject& vao, uint32_t bindingindex,
                                  uint32_t divisor);
    // Resolves the VAO bound to GL_VERTEX_ARRAY_BINDING for the non-DSA
    // spellings; reports GL_INVALID_OPERATION and returns nullptr when none is
    // bound (SPEC §10.3.1: those commands need a vertex array object).
    VertexArrayObject* boundVertexArrayForEdit();

    // Backend program for the currently active program (nullptr when none / not
    // linked / no backend resource). Used by the uniform setters.
    BackendProgram* activeBackendProgram();
    // Backend program for an explicit program object; nullptr (and
    // GL_INVALID_OPERATION) when `program` is not a linked program with a
    // backend resource. Used by the glProgramUniform* setters.
    BackendProgram* backendProgramFor(GLObjectName program);

    // Recompute mutable-storage metadata (base dimensions / level count) from the
    // recorded glTexImage* levels so getTextureLevelParameter* queries return the
    // uploaded size. Immutable storage (glTextureStorage*) owns these fields itself.
    void updateMutableTextureStorage(TextureObject* tex);
    // Shared body for DSA + classic texture-level parameter queries (SPEC §8.1).
    void getTexLevelParameterivImpl(TextureObject* tex, int level, GLenum pname,
                                    int32_t* params);
    void getTexLevelParameterfvImpl(TextureObject* tex, int level, GLenum pname,
                                    float* params);
    // Shared body for DSA + classic renderbuffer-parameter queries (SPEC §9.2.4).
    void getRenderbufferParameterivImpl(RenderbufferObject* rbo, uint32_t pname,
                                        int32_t* params);
    // Shared body for DSA + classic FBO attachment-parameter queries (SPEC §9.2.3).
    void getFramebufferAttachmentParameterivImpl(FramebufferObject* fbo,
                                                 uint32_t attachment,
                                                 uint32_t pname, int32_t* params);
    GLObjectName nextName_ = 1;

    IGraphicsBackend& backend_;
    GLError error_ = GLError::NoError;
    GLStateTracker state_;

    std::unordered_map<GLObjectName, std::unique_ptr<BufferObject>> buffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<TextureObject>> textures_;
    std::unordered_map<GLObjectName, std::unique_ptr<RenderbufferObject>> renderbuffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<FramebufferObject>> framebuffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<VertexArrayObject>> vertexArrays_;
    std::unordered_map<GLObjectName, std::unique_ptr<SamplerObject>> samplers_;
    std::unordered_map<GLObjectName, std::unique_ptr<ShaderObject>> shaders_;
    std::unordered_map<GLObjectName, std::unique_ptr<ProgramObject>> programs_;
    std::unordered_map<GLObjectName, std::unique_ptr<ProgramPipelineObject>>
        pipelines_;
    std::unordered_map<GLObjectName, std::unique_ptr<TransformFeedbackObject>>
        transformFeedbacks_;
    std::unordered_map<GLObjectName, std::unique_ptr<QueryObject>> queries_;
    // Active query per target (SPEC §4: only one query per target may be active).
    std::unordered_map<uint32_t, GLObjectName> activeQueries_;
    // Frontend-owned sync objects. The raw pointer doubles as the opaque GLsync.
    std::vector<std::unique_ptr<SyncObject>> syncs_;

    bool vertexStateDirty_ = false;
    bool transformFeedbackActive_ = false;
    bool transformFeedbackPaused_ = false;
    bool conditionalRenderActive_ = false;
    GLObjectName conditionalRenderQuery_ = 0;

    // True if `target` is a query type allowed to predicate a conditional-render
    // region (SPEC §10.11).
    bool isConditionalRenderQueryType(uint32_t target) const;

    // Returns the indexed buffer-binding slot for a transform-feedback object, or
    // nullptr after setting the appropriate GL error (out-of-range index, or an
    // ungenerated TF object name). `xfb == 0` selects the default TF object.
    TransformFeedbackObject::TfBufferBinding* tfBufferBindingSlot(
        GLObjectName xfb, uint32_t index);

    // Returns the binding slot of the currently active TF object (the bound named
    // object, or the default object when none is bound). nullptr on out-of-range.
    TransformFeedbackObject::TfBufferBinding* activeTransformFeedbackBinding(
        uint32_t index);

    std::unordered_map<uint32_t, GLObjectName> boundBuffers_;
    GLObjectName boundRenderbuffer_ = 0;
    GLObjectName boundFramebuffer_ = 0;
    GLObjectName boundVertexArray_ = 0;
    GLObjectName boundTransformFeedback_ = 0;
    GLObjectName boundProgramPipeline_ = 0;

    // Buffer bindings of the default (name 0) transform-feedback object
    // (SPEC §13.2.1). Indexed by binding point 0..kMaxTransformFeedbackBuffers-1.
    std::vector<TransformFeedbackObject::TfBufferBinding>
        defaultTransformFeedbackBuffers_{kMaxTransformFeedbackBuffers};

    // Object-label stores (SPEC §22.2). objectLabels_ is keyed by (identifier << 32
    // | name); ptrLabels_ by the raw sync pointer.
    std::unordered_map<uint64_t, std::string> objectLabels_;
    std::unordered_map<const void*, std::string> ptrLabels_;

    // ---- Debug messaging state (SPEC §20.4 / §20.5) ----
    struct DebugMessage {
        GLenum source;
        GLenum type;
        GLuint id;
        GLenum severity;
        std::string text;
    };
    GLDEBUGPROC debugCallback_ = nullptr;
    const void* debugUserParam_ = nullptr;
    // Space-wide enable filter indexed by (source, type, severity) using the
    // fixed enum value set; default-constructed to "all enabled".
    static constexpr int kDebugSources = 7;  // API..OTHER (DONT_CARE handled via index 6)
    static constexpr int kDebugTypes = 10;   // ERROR..OTHER + PUSH/POP_GROUP
    static constexpr int kDebugSeverities = 4; // HIGH..LOW + NOTIFICATION
    bool debugEnabled_[7][10][4] = {};       // filled true in constructor
    // Per-(source,type) per-id overrides; present key disables/enables that id.
    std::map<std::pair<GLenum, GLenum>, std::map<GLuint, bool>> debugIdEnabled_;
    // Ring of generated messages for glGetDebugMessageLog (FIFO).
    std::deque<DebugMessage> debugLog_;
    static constexpr size_t kDebugLogMax = 1024;
    // Debug group stack: each entry records its (source, id, message).
    std::vector<DebugMessage> debugGroups_;

    // True when a message of the given (source, type, id, severity) passes the
    // current control filter. Per-id rules (when present for source,type,id)
    // override the space-wide table.
    bool debugMessageEnabled(GLenum source, GLenum type, GLuint id,
                             GLenum severity) const;
    // Core emit path: filter, invoke callback, append to log. `length < 0` means
    // `buf` is NUL-terminated (SPEC §20.4 allows -1).
    void emitDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity,
                         GLsizei length, const GLchar* buf);
};

} // namespace glcompat
