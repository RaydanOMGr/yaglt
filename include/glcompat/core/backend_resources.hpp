#pragma once

#include <string>
#include <vector>

namespace glcompat {

// Opaque backend resource handles. Concrete backends subclass these.
// The frontend stores std::unique_ptr<BackendX> and never inspects internals,
// so backend-native handles never leak into the generic frontend API.
class BackendBuffer {
public:
    virtual ~BackendBuffer() = default;
    // Allocate/stream buffer storage (SPEC §2.1 glBufferData). `data` may be null.
    virtual void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                            const void* data) {}
    // Update a sub-region of existing storage (SPEC §6 glBufferSubData). The
    // frontend validates bounds and keeps its own CPU mirror; backends that own
    // native storage forward this to the driver.
    virtual void bufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                               const void* data) {}
    // Allocate immutable storage (SPEC §6 glBufferStorage). Once allocated the
    // buffer's size/usage/flags cannot change. Backends with native immutable
    // storage (GLES 3.1+) forward this; others may emulate via bufferData.
    virtual void bufferStorage(uint32_t target, intptr_t size, uint32_t flags,
                               const void* data) {}
    // Copy between two buffers' storage (SPEC §6 glCopyBufferSubData).
    virtual void copySubData(uint32_t readTarget, uint32_t writeTarget,
                             intptr_t readOffset, intptr_t writeOffset,
                             intptr_t size) {}
    // Map a region for CPU access (SPEC §6 glMapBufferRange). Returns a pointer to
    // backend-owned memory, or nullptr when the backend has no native mapping (the
    // frontend then serves the CPU mirror it maintains). Default no-op.
    virtual void* mapBufferRange(uint32_t target, intptr_t offset, intptr_t length,
                                 uint32_t access) {
        (void)target; (void)offset; (void)length; (void)access;
        return nullptr;
    }
    // Unmap a previously mapped region (SPEC §6 glUnmapBuffer). Default no-op.
    virtual void unmapBuffer(uint32_t target) { (void)target; }
    // Flush a mapped sub-region so backend writes the CPU-mirror contents back to
    // native storage (SPEC §6 glFlushMappedBufferRange). Default no-op.
    virtual void flushMappedBufferRange(uint32_t target, intptr_t offset,
                                        intptr_t length) {
        (void)target; (void)offset; (void)length;
    }
    // DSA buffer mapping (SPEC §6.1 glMapNamedBuffer / glMapNamedBufferRange /
    // glUnmapNamedBuffer / glFlushMappedNamedBufferRange). Keyed by this buffer
    // resource (not a bind target), unlike the target-based variants above. The
    // default forwards onto the target-based methods with a placeholder target,
    // which is sufficient for backends that ignore the target (mock); native
    // backends override these to use the DSA entry points directly.
    virtual void* mapNamedBufferRange(intptr_t offset, intptr_t length,
                                      uint32_t access) {
        return mapBufferRange(0, offset, length, access);
    }
    virtual void unmapNamedBuffer() { unmapBuffer(0); }
    virtual void flushMappedNamedBufferRange(intptr_t offset, intptr_t length) {
        flushMappedBufferRange(0, offset, length);
    }
    // Discard the buffer's cached data store (SPEC §6 glInvalidateBufferData /
    // glInvalidateBufferSubData), a driver hint. Default no-op; GLES 3.0+ forwards
    // to the driver, which may free or repurpose the backing store.
    virtual void invalidateBufferData(uint32_t target) {
        (void)target;
    }
    virtual void invalidateBufferSubData(uint32_t target, intptr_t offset,
                                         intptr_t length) {
        (void)target; (void)offset; (void)length;
    }
    // Native driver buffer name (0 when the backend has no native handle). The
    // frontend registers this so the backend's name->native map resolves buffer
    // binds at draw/flush time (SPEC §3/§11).
    virtual uint32_t nativeId() const { return 0; }

    // DSA buffer allocation (SPEC §6.1/§6.2 glNamedBufferData / glNamedBufferSubData
    // / glNamedBufferStorage). Keyed by this buffer resource (not a bind target),
    // unlike the target-based variants above. The default forwards onto the
    // target-based methods with a placeholder target, which is sufficient for
    // backends that ignore the target (mock); native backends override these.
    virtual void namedBufferData(intptr_t size, uint32_t usage, const void* data) {
        bufferData(0, size, usage, data);
    }
    virtual void namedBufferSubData(intptr_t offset, intptr_t size,
                                    const void* data) {
        bufferSubData(0, offset, size, data);
    }
    virtual void namedBufferStorage(intptr_t size, uint32_t flags,
                                    const void* data) {
        bufferStorage(0, size, flags, data);
    }
};
class BackendTexture {
public:
    virtual ~BackendTexture() = default;
    // Allocate storage for a 2D texture level (SPEC §2.1 glTexImage2D).
    // `data` may be null.
    virtual void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                            int width, int height, uint32_t format, uint32_t type,
                            const void* data) {}
    // Allocate storage for a 1D texture level (SPEC §8 TexImage1D). `data` may
    // be null. The default implementation is a no-op; backends opt in.
    virtual void texImage1D(uint32_t target, int level, uint32_t internalFormat,
                            int width, uint32_t format, uint32_t type,
                            const void* data) {}
    // Allocate storage for a 3D texture level (SPEC §8 TexImage3D). `data` may
    // be null. The default implementation is a no-op; backends opt in.
    virtual void texImage3D(uint32_t target, int level, uint32_t internalFormat,
                            int width, int height, int depth, uint32_t format,
                            uint32_t type, const void* data) {}
    // Set a texture parameter (SPEC §2.1 glTexParameter*). The frontend keeps the
    // authoritative value and forwards the native call; backends opt in.
    virtual void texParameteri(uint32_t target, uint32_t pname, int param) {}
    virtual void texParameterf(uint32_t target, uint32_t pname, float param) {}
    virtual void texParameterfv(uint32_t target, uint32_t pname,
                                const float* params, int count) {}
    virtual void texParameteriv(uint32_t target, uint32_t pname,
                                const int* params, int count) {}
    // Set integer (signed / unsigned) texture parameters (SPEC §8.1
    // glTexParameterIiv / glTexParameterIuiv). The frontend keeps the authoritative
    // vector and forwards the native call; backends opt in.
    virtual void texParameterIiv(uint32_t target, uint32_t pname, const int32_t* params,
                                int count) {}
    virtual void texParameterIuiv(uint32_t target, uint32_t pname, const uint32_t* params,
                                  int count) {}
    // Upload a sub-region of an existing texture level (SPEC §8.6 TexSubImage*D).
    // The frontend validates bounds and that the level was allocated by a prior
    // TexImage; backends with native storage forward the call to the driver.
    virtual void texSubImage1D(uint32_t target, int level, int xoffset, int width,
                               uint32_t format, uint32_t type, const void* data) {}
    virtual void texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                               int width, int height, uint32_t format, uint32_t type,
                               const void* data) {}
    virtual void texSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                               int zoffset, int width, int height, int depth,
                               uint32_t format, uint32_t type, const void* data) {}
    // Define a texture image by copying from the framebuffer (SPEC §8.5
    // CopyTexImage*D). The backend uses the currently bound read framebuffer.
    virtual void copyTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                                int x, int y, int width, int border) {}
    virtual void copyTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                                 int x, int y, int width, int height, int border) {}
    // Allocate immutable storage for one texture level (SPEC §2.1 / §8.1,
    // glTextureStorage*D DSA). `levels` is the total mip levels; width/height/
    // depth the level-0 dimensions. GLES has no 1D textures so storage1D is a
    // no-op on that backend.
    virtual void storage1D(uint32_t target, int levels, uint32_t internalFormat,
                           int width) {}
    virtual void storage2D(uint32_t target, int levels, uint32_t internalFormat,
                           int width, int height) {}
    virtual void storage3D(uint32_t target, int levels, uint32_t internalFormat,
                           int width, int height, int depth) {}
    // Regenerate the full mipmap chain (SPEC §8.1 glGenerateTextureMipmap).
    virtual void generateMipmap(uint32_t target) {}
    // Bind a buffer object as the texture's texel store (SPEC §8.9 glTextureBuffer
    // / glTextureBufferRange). `bufferNativeId` is the backend-native buffer id.
    virtual void textureBuffer(uint32_t target, uint32_t internalFormat,
                               uint32_t bufferNativeId) {}
    virtual void textureBufferRange(uint32_t target, uint32_t internalFormat,
                                     uint32_t bufferNativeId, intptr_t offset,
                                     intptr_t size) {}
    // Immutable multisample storage (SPEC §8.19 glTexStorage2DMultisample /
    // glTexStorage3DMultisample). `fixedSampleLocations` mirrors the GL boolean.
    virtual void storage2DMultisample(uint32_t target, int samples,
                                      uint32_t internalFormat, int width, int height,
                                      bool fixedSampleLocations) {}
    virtual void storage3DMultisample(uint32_t target, int samples,
                                      uint32_t internalFormat, int width, int height,
                                      int depth, bool fixedSampleLocations) {}
    // Mutable multisample allocation (SPEC §8.19 glTexImage2DMultisample /
    // glTexImage3DMultisample).
    virtual void texImage2DMultisample(uint32_t target, int samples,
                                       uint32_t internalFormat, int width, int height,
                                       bool fixedSampleLocations) {}
    virtual void texImage3DMultisample(uint32_t target, int samples,
                                       uint32_t internalFormat, int width, int height,
                                       int depth, bool fixedSampleLocations) {}
    // Level queries (SPEC §8.1 glGetTextureLevelParameter*). The frontend owns
    // width/height/depth/internalFormat for allocated storage; backends with
    // native introspection override these for completeness.
    virtual void getLevelParameteriv(uint32_t target, int level, uint32_t pname,
                                     int32_t* params) {}
    virtual void getLevelParameterfv(uint32_t target, int level, uint32_t pname,
                                     float* params) {}
    // Read texel data back (SPEC §8.1 glGetTextureImage). The frontend forwards
    // to the backend, which serves driver memory; backends without native reads
    // are no-ops (the mock records the call).
    virtual void getTexImage(uint32_t target, int level, uint32_t format,
                             uint32_t type, void* pixels) {}
    // Read back a compressed texture image (SPEC §8.11 glGetCompressedTexImage /
    // glGetCompressedTextureImage). Returns the compressed block data directly;
    // backends without native reads are no-ops (the mock records the call).
    virtual void getCompressedTexImage(uint32_t target, int level, void* pixels) {}
    // Robustness variants (ARB_robustness / GL 4.5): `bufSize` bounds the `pixels`
    // write in bytes. Backends with a native robust entry call it; backends without
    // one fall back to the non-robust read (the mock records the call).
    virtual void getTexImage(uint32_t target, int level, uint32_t format,
                             uint32_t type, int bufSize, void* pixels) {}
    virtual void getCompressedTexImage(uint32_t target, int level, int bufSize,
                                       void* pixels) {}
    // Read back a sub-rectangle of a texture image (SPEC §8.11.4
    // glGetTextureSubImage). Backends without native reads are no-ops (the mock
    // records the call).
    virtual void getTextureSubImage(uint32_t target, int level, int xoffset,
                                    int yoffset, int zoffset, int width, int height,
                                    int depth, uint32_t format, uint32_t type,
                                    int bufSize, void* pixels) {}
    // Read back a compressed sub-rectangle of a texture image (SPEC §8.11.5
    // glGetCompressedTextureSubImage). Backends without native reads are no-ops
    // (the mock records the call).
    virtual void getCompressedTextureSubImage(uint32_t target, int level, int xoffset,
                                              int yoffset, int zoffset, int width,
                                              int height, int depth, int bufSize,
                                              void* pixels) {}
    // Invalidate all or part of a texture's contents (SPEC §8.1 glInvalidateTexImage
    // / glInvalidateTexSubImage). A driver discard hint; backends opt in.
    virtual void invalidateTexImage(uint32_t target, int level) {}
    virtual void invalidateTexSubImage(uint32_t target, int level, int xoffset,
                                      int yoffset, int zoffset, int width, int height,
                                      int depth) {}
    // Create a texture view that shares storage with another texture (SPEC
    // §8.19 glTextureView). `origTextureNativeId` is the backend-native id of the
    // source texture (which must already have immutable storage). The frontend
    // owns the view's object identity and derived level/layer ranges; backends
    // with native texture-view support (GLES 3.1+) forward the call.
    virtual void view(uint32_t target, uint32_t origTextureNativeId,
                      uint32_t internalFormat, uint32_t minLevel, uint32_t numLevels,
                      uint32_t minLayer, uint32_t numLayers) {}
    // Native backend texture id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};
class BackendRenderbuffer {
public:
    virtual ~BackendRenderbuffer() = default;
    // Allocate storage for the renderbuffer (SPEC §2.1 glRenderbufferStorage).
    virtual void renderbufferStorage(uint32_t target, uint32_t internalFormat,
                                     int width, int height) {}
    // Allocate multisample storage (SPEC §9.2.4 glRenderbufferStorageMultisample /
    // glNamedRenderbufferStorageMultisample). `samples` is the requested sample
    // count (0 means single-sample). Backends opt in; default no-op.
    virtual void renderbufferStorageMultisample(uint32_t target, int samples,
                                               uint32_t internalFormat, int width,
                                               int height) {}
    // Native backend renderbuffer id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};
class BackendFramebuffer {
public:
    virtual ~BackendFramebuffer() = default;
    // Native driver framebuffer id (resolved by the frontend via the native map).
    virtual uint32_t nativeId() const { return 0; }
    // Attach a texture level (SPEC §2.1 glFramebufferTexture2D). `nativeTexture`
    // is the backend-native texture id resolved by the frontend.
    virtual void framebufferTexture2D(uint32_t target, uint32_t attachment,
                                      uint32_t texTarget, uint32_t nativeTexture,
                                      int level) {}
    // Attach a renderbuffer (SPEC §2.1 glFramebufferRenderbuffer).
    virtual void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                         uint32_t rbTarget,
                                         uint32_t nativeRenderbuffer) {}
    // Attach a single layer of a texture (SPEC §9.2 glFramebufferTextureLayer /
    // glNamedFramebufferTextureLayer). `nativeTexture` is the backend-native id;
    // `layer` selects the layer of a 1D/2D array or 3D texture.
    virtual void framebufferTextureLayer(uint32_t target, uint32_t attachment,
                                         uint32_t nativeTexture, int level,
                                         int layer) {}
    // Set a framebuffer parameter (SPEC §9.2 glFramebufferParameteri /
    // glNamedFramebufferParameteri), e.g. GL_FRAMEBUFFER_DEFAULT_WIDTH/HEIGHT/
    // SAMPLES. Backends opt in; default no-op.
    virtual void framebufferParameteri(uint32_t target, uint32_t pname,
                                       int param) {}
    // Returns a GL_FRAMEBUFFER_* status code. Defaults to Complete; real backends
    // query driver completeness.
    virtual uint32_t checkStatus(uint32_t /*target*/) const { return 0x8CD5; }
};
class BackendVertexArray {
public:
    virtual ~BackendVertexArray() = default;
    // Native backend VAO id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};

// Sampler object (SPEC §8.2). Holds texture-parameter state that overrides the
// per-texture state when bound to a texture unit. Defaults are no-ops so backends
// opt in. `samplerParameteri` sets a scalar sampler parameter (wrap/min/mag
// filter/compare/lod bias); `samplerParameterf`/`samplerParameterfv` the float
// and float-vector parameters (LOD range/bias, BORDER_COLOR); the `Iiv`/`Iuiv`
// forms mirror the signed/unsigned integer accessor for the scalar int pnames.
class BackendSampler {
public:
    virtual ~BackendSampler() = default;
    virtual void samplerParameteri(uint32_t pname, int param) {}
    virtual void samplerParameterf(uint32_t pname, float param) {}
    virtual void samplerParameterfv(uint32_t pname, const float* params, int count) {}
    virtual void samplerParameterIiv(uint32_t pname, const int32_t* params) {}
    virtual void samplerParameterIuiv(uint32_t pname, const uint32_t* params) {}
    // Native backend sampler id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};

// Query object (SPEC §4 / §19). Occlusion/primitive/timer queries capture
// counter values during drawing. Defaults are no-ops so backends opt in.
// `queryResult` fills the most recent result value and availability; the
// frontend interprets the pname (QUERY_RESULT / QUERY_RESULT_AVAILABLE).
class BackendQuery {
public:
    virtual ~BackendQuery() = default;
    virtual void begin(uint32_t target) {}
    virtual void end() {}
    // Record a timestamp query when all prior GL commands have completed
    // (SPEC §4.2.1 glQueryCounter, target == GL_TIMESTAMP). Default no-op so
    // backends opt in; the GLES backend forwards to the driver, the mock records it.
    virtual void queryCounter(uint32_t target) { (void)target; }
    virtual void queryResult(int64_t* value, bool* available) {
        *value = 0;
        *available = false;
    }
};

// Transform feedback object (SPEC §13.3). Captures primitives during drawing into
// bound transform-feedback buffers. Defaults are no-ops so backends opt in.
class BackendTransformFeedback {
public:
    virtual ~BackendTransformFeedback() = default;
    // Begin/end a transform-feedback capture of the given primitive mode
    // (GL_POINTS / GL_LINES / GL_TRIANGLES). pause/resume suspend/resume an
    // active capture without ending it.
    virtual void begin(uint32_t mode) {}
    virtual void end() {}
    virtual void pause() {}
    virtual void resume() {}
    // Number of vertices written to `stream` by the most recent capture
    // (SPEC §13.3.3 glDrawTransformFeedback). The frontend forwards it to draw
    // calls so backends that do not track driver-side captured counts can record
    // the intended draw size. Defaults to 0.
    virtual int64_t getCapturedVertexCount(uint32_t stream = 0) const {
        (void)stream;
        return 0;
    }
    // Native driver handle for this transform-feedback object (0 when the backend
    // has no native resource). The frontend passes it to draw calls so the GLES
    // backend can issue glDrawTransformFeedback(id) against the right object.
    virtual uint32_t nativeHandle() const { return 0; }
};
class BackendShader {
public:
    virtual ~BackendShader() = default;

    // Compile already-translated source for this shader's stage. Returns true on
    // success; on failure fills `log` with a diagnostic. The frontend runs any
    // desktop->backend translation (via IShaderCompiler) before calling this, so
    // the backend receives backend-compatible source.
    virtual bool compile(const std::string& source, std::string& log) = 0;
    // Load a precompiled shader binary (glShaderBinary, SPEC §7.2). Default no-op
    // so backends opt in; a real driver consumes the blob directly.
    virtual void loadBinary(uint32_t binaryFormat, const void* binary, int32_t length) {
        (void)binaryFormat; (void)binary; (void)length;
    }
    // Specialize a previously loaded SPIR-V shader and compile it (glSpecialize-
    // Shader, SPEC §7.4). `entryPoint` selects the SPIR-V entry point; `num-
    // Constants` plus `constantIndex`/`constantValue` supply SPIR-V specialization
    // constants. Returns true on success; on failure fills `log`. Default is a
    // no-op success so backends that consume pre-specialized binaries opt in.
    virtual bool specialize(const std::string& entryPoint, uint32_t numConstants,
                            const uint32_t* constantIndex, const uint32_t* constantValue,
                            std::string& log) {
        (void)entryPoint; (void)numConstants; (void)constantIndex; (void)constantValue;
        log.clear();
        return true;
    }
};
class BackendProgram {
public:
    virtual ~BackendProgram() = default;

    // Attach a previously compiled backend shader to this program.
    virtual void attach(BackendShader& shader) = 0;
    // Detach a previously attached backend shader (SPEC §7.4 glDetachShader).
    // Does not undo a successful link; the native program keeps its executable.
    virtual void detach(BackendShader& shader) {}
    // Link the attached shaders. Returns true on success; fills `log` on failure.
    virtual bool link(std::string& log) = 0;
    // Validate the linked program against the current GL state (SPEC §7.3
    // glValidateProgram). Sets `log` with the driver validation message. Default
    // no-op so backends opt in; the mock records the call and reports success.
    virtual void validate(std::string& log) {
        (void)log;
        log = "validated";
    }
    // Attribute location for `name` after linking (-1 if absent).
    virtual int getAttribLocation(const std::string& name) const = 0;
    // Fragment-output location / dual-source index for `name` (SPEC §7.3.6
    // glGetFragDataLocation / glGetFragDataIndex). Returns -1 when the output is
    // absent (matching desktop GL semantics). Backends with fragment-output
    // introspection override; the default -1 is honest for those without it (e.g.
    // GLES, which has no direct equivalent).
    virtual int getFragDataLocation(const std::string& name) const { return -1; }
    virtual int getFragDataIndex(const std::string& name) const { return -1; }
    // Fragment-output location / dual-source index binding (SPEC §7.3.7 /
    // §15.1.2 glBindFragDataLocation / glBindFragDataLocationIndexed). Called
    // before link(); takes effect on the subsequent link. Backends without a
    // direct equivalent (e.g. GLES) record honestly or no-op.
    virtual void bindFragDataLocation(const std::string& name, int colorNumber,
                                      int index) {
        (void)name;
        (void)colorNumber;
        (void)index;
    }
    // Transform-feedback varying reflection (SPEC §13.3.1 glGetTransformFeedback-
    // Varying). Writes the varying's `name` (trimmed to bufSize-1), `size`, and
    // `type` for `index`, and the name length (excl. nul) into `*length`. Returns
    // true on success. Returns false when `index` is out of range or the backend
    // has no introspection; the default (false) is honest for backends such as
    // GLES that expose no direct equivalent (EXT_transform_feedback query
    // semantics differ), and the frontend maps "missing" to GL_INVALID_VALUE.
    virtual bool getTransformFeedbackVarying(uint32_t index, int bufSize, int* length,
                                             int* size, uint32_t* type,
                                             char* name) const {
        (void)index; (void)bufSize; (void)length; (void)size; (void)type; (void)name;
        return false;
    }
    // Bind generic vertex attribute `index` to the attribute variable `name`
    // (SPEC §7.3.7 glBindAttribLocation). Called before link(); takes effect on
    // the next link. Default no-op so backends opt in.
    virtual void bindAttribLocation(const std::string& name, int index) {}
    // Associate a program's uniform block `blockIndex` with uniform-buffer
    // binding point `blockBinding` (SPEC §7.6.2 glUniformBlockBinding). Default
    // no-op so backends opt in; the GLES backend forwards to the driver on the
    // native program, and the mock records it.
    virtual void uniformBlockBinding(uint32_t blockIndex, uint32_t blockBinding) {
        (void)blockIndex; (void)blockBinding;
    }
    // Associate a program's shader-storage block `blockIndex` with shader-storage-
    // buffer binding point `blockBinding` (SPEC §7.6.2 glShaderStorageBlockBinding).
    // Default no-op so backends opt in; the GLES backend forwards to the driver on
    // the native program (ES 3.1+), and the mock records it.
    virtual void shaderStorageBlockBinding(uint32_t blockIndex, uint32_t blockBinding) {
        (void)blockIndex; (void)blockBinding;
    }
    // Specify the transform-feedback varying names captured when the program is
    // the active program of a transform-feedback begin (SPEC §13.3.1
    // glTransformFeedbackVaryings). Called before link(); takes effect on the
    // next link. `bufferMode` is GL_INTERLEAVED_ATTRIBS or GL_SEPARATE_ATTRIBS.
    // Default no-op so backends opt in; the GLES backend forwards to the driver
    // on the native program, and the mock records the request.
    virtual void transformFeedbackVaryings(const std::vector<std::string>& varyings,
                                           uint32_t bufferMode) {
        (void)varyings; (void)bufferMode;
    }
    // Native backend program id (e.g. driver GLuint). 0 when not linked.
    virtual uint32_t nativeId() const = 0;

    // Load a precompiled program binary (glProgramBinary, SPEC §7.3 / §19.1).
    // Default no-op so backends opt in; a real driver consumes the blob directly
    // and the program becomes linked. The frontend keeps the authoritative binary
    // mirror, matching the buffer-mirror pattern.
    virtual void loadBinary(uint32_t binaryFormat, const void* binary, int32_t length) {
        (void)binaryFormat; (void)binary; (void)length;
    }

    // Uniform management (SPEC §8). Operate on this linked program. Defaults are
    // no-ops so backends opt in. `getUniformLocation` returns -1 when the uniform
    // is absent (matching desktop GL semantics). A -1 location is a silent no-op
    // in every setter, matching glUniform* behavior.
    virtual int getUniformLocation(const std::string& name) const { return -1; }
    // Uniform value readback (SPEC §7.9 glGetUniform{f,i,ui,d}v). Operate on this
    // linked program; `location` must be a valid (>= 0) location. Defaults are
    // no-ops so backends opt in; the frontend validates the program/link state,
    // the location, and a null `params` before calling these.
    virtual void getUniformfv(int32_t location, float* params) const {}
    virtual void getUniformiv(int32_t location, int32_t* params) const {}
    virtual void getUniformuiv(int32_t location, uint32_t* params) const {}
    virtual void getUniformdv(int32_t location, double* params) const {}
    virtual void uniform1f(int loc, float v0) {}
    virtual void uniform2f(int loc, float v0, float v1) {}
    virtual void uniform3f(int loc, float v0, float v1, float v2) {}
    virtual void uniform4f(int loc, float v0, float v1, float v2, float v3) {}
    virtual void uniform1i(int loc, int v0) {}
    virtual void uniform2i(int loc, int v0, int v1) {}
    virtual void uniform3i(int loc, int v0, int v1, int v2) {}
    virtual void uniform4i(int loc, int v0, int v1, int v2, int v3) {}
    virtual void uniform1fv(int loc, const float* v, int count) {}
    virtual void uniform1iv(int loc, const int* v, int count) {}
    // Double-precision uniform setters (SPEC §8). ES has no double uniforms, so
    // backends without desktop double support implement these as no-ops; the
    // frontend still validates and the mock records them.
    virtual void uniform1d(int loc, double v0) {}
    virtual void uniform2d(int loc, double v0, double v1) {}
    virtual void uniform3d(int loc, double v0, double v1, double v2) {}
    virtual void uniform4d(int loc, double v0, double v1, double v2, double v3) {}
    virtual void uniform1dv(int loc, const double* v, int count) {}
    virtual void uniform2dv(int loc, const double* v, int count) {}
    virtual void uniform3dv(int loc, const double* v, int count) {}
    virtual void uniform4dv(int loc, const double* v, int count) {}
    // Unsigned-integer uniform setters (SPEC §8). Native in GLES 3.0+.
    virtual void uniform1ui(int loc, uint32_t v0) {}
    virtual void uniform2ui(int loc, uint32_t v0, uint32_t v1) {}
    virtual void uniform3ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2) {}
    virtual void uniform4ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) {}
    virtual void uniform1uiv(int loc, const uint32_t* v, int count) {}
    virtual void uniform2uiv(int loc, const uint32_t* v, int count) {}
    virtual void uniform3uiv(int loc, const uint32_t* v, int count) {}
    virtual void uniform4uiv(int loc, const uint32_t* v, int count) {}
    // Remaining vector setters (SPEC §8) for the active program.
    virtual void uniform2fv(int loc, const float* v, int count) {}
    virtual void uniform3fv(int loc, const float* v, int count) {}
    virtual void uniform4fv(int loc, const float* v, int count) {}
    virtual void uniform2iv(int loc, const int* v, int count) {}
    virtual void uniform3iv(int loc, const int* v, int count) {}
    virtual void uniform4iv(int loc, const int* v, int count) {}
    virtual void uniformMatrix2fv(int loc, const float* m, int count, bool transpose) {}
    virtual void uniformMatrix3fv(int loc, const float* m, int count, bool transpose) {}
    virtual void uniformMatrix2dv(int loc, const double* m, int count, bool transpose) {}
    virtual void uniformMatrix3dv(int loc, const double* m, int count, bool transpose) {}
    virtual void uniformMatrix4dv(int loc, const double* m, int count, bool transpose) {}
    // Active object counts after linking (SPEC §7.3 / §7.14). Backends with
    // introspection override these; the default (0) is honest for backends that
    // do not yet expose program reflection.
    virtual int activeUniformCount() const { return 0; }
    virtual int activeAttributeCount() const { return 0; }
    virtual int activeUniformBlockCount() const { return 0; }
    // Number of active shader-storage blocks in the linked program (SPEC §7.6.2).
    // Honest 0 for backends without introspection; the mock returns a configurable
    // count so frontend block-index validation can be exercised.
    virtual int activeShaderStorageBlockCount() const { return 0; }

    // Program-interface reflection (SPEC §7.3.11). Defaults are honest for
    // backends without introspection: no resources are visible, names are not
    // found, locations are -1, and property reads report 0.
    virtual uint32_t programResourceCount(uint32_t programInterface) const {
        (void)programInterface;
        return 0;
    }
    virtual uint32_t getProgramResourceIndex(uint32_t programInterface,
                                             const std::string& name) const {
        (void)programInterface;
        (void)name;
        return 0xFFFFFFFFu; // GL_INVALID_INDEX: name not found
    }
    virtual void getProgramResourceName(uint32_t programInterface, uint32_t index,
                                        int32_t bufSize, int32_t* length,
                                        char* name) const {
        (void)programInterface;
        (void)index;
        (void)bufSize;
        (void)length;
        (void)name;
    }
    virtual void getProgramResourceiv(uint32_t programInterface, uint32_t index,
                                      int32_t propCount, const uint32_t* props,
                                      int32_t bufSize, int32_t* length,
                                      int32_t* params) const {
        (void)programInterface;
        (void)index;
        (void)propCount;
        (void)props;
        (void)bufSize;
        (void)length;
        (void)params;
    }
    virtual int32_t getProgramResourceLocation(uint32_t programInterface,
                                              const std::string& name) const {
        (void)programInterface;
        (void)name;
        return -1;
    }
    virtual int32_t getProgramResourceLocationIndex(uint32_t programInterface,
                                                    const std::string& name) const {
        (void)programInterface;
        (void)name;
        return -1;
    }

    // Program-interface summary query (SPEC §7.3.1 glGetProgramInterfaceiv).
    // Defaults are honest for backends without introspection: ACTIVE_RESOURCES
    // mirrors programResourceCount (0), and the MAX_* sizing pnames report 0.
    virtual void getProgramInterfaceiv(uint32_t programInterface, uint32_t pname,
                                       int32_t* params) const {
        (void)programInterface;
        (void)pname;
        if (params) {
            *params = (pname == 0x92F5u)  // GL_ACTIVE_RESOURCES
                          ? static_cast<int32_t>(programResourceCount(programInterface))
                          : 0;
        }
    }


    // Subroutine reflection + selection (SPEC §7.9). Defaults are honest for
    // backends without introspection: names are not found, locations are -1,
    // property reads report 0, and selection is a no-op.
    virtual uint32_t getSubroutineIndex(uint32_t shadertype,
                                        const std::string& name) const {
        (void)shadertype;
        (void)name;
        return 0xFFFFFFFFu;  // GL_INVALID_INDEX
    }
    virtual int32_t getSubroutineUniformLocation(uint32_t shadertype,
                                                 const std::string& name) const {
        (void)shadertype;
        (void)name;
        return -1;
    }
    virtual void getActiveSubroutineUniformiv(uint32_t shadertype, uint32_t index,
                                             uint32_t pname, int32_t* values) const {
        (void)shadertype;
        (void)index;
        (void)pname;
        (void)values;
    }
    virtual void getActiveSubroutineUniformName(uint32_t shadertype, uint32_t index,
                                                int32_t bufSize, int32_t* length,
                                                char* name) const {
        (void)shadertype;
        (void)index;
        (void)bufSize;
        (void)length;
        (void)name;
    }
    virtual void getActiveSubroutineName(uint32_t shadertype, uint32_t index,
                                        int32_t bufSize, int32_t* length,
                                        char* name) const {
        (void)shadertype;
        (void)index;
        (void)bufSize;
        (void)length;
        (void)name;
    }
    virtual void uniformSubroutinesuiv(uint32_t shadertype, int32_t count,
                                      const uint32_t* indices) {
        (void)shadertype;
        (void)count;
        (void)indices;
    }
    virtual void getUniformSubroutineuiv(uint32_t shadertype, int32_t location,
                                        uint32_t* params) const {
        (void)shadertype;
        (void)location;
        (void)params;
    }

    // Per-stage subroutine summary query (SPEC §7.9 glGetProgramStageiv). The
    // default is honest for backends without introspection: every recognized
    // pname reports 0 (no active subroutines / unknown limits).
    virtual void getProgramStageiv(uint32_t shadertype, uint32_t pname,
                                   int32_t* values) const {
        (void)shadertype;
        (void)pname;
        if (values) *values = 0;
    }

    virtual void uniformMatrix4fv(int loc, const float* m, int count,
                                 bool transpose) {}
};

} // namespace glcompat
