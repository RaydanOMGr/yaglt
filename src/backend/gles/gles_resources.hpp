#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/backend_resources.hpp"

#include <vector>

namespace glcompat {

// Backend resource handles wrapping a real GLES object name. Deletion goes
// through the loader so the GLES object is freed when the frontend releases
// the (opaque) frontend object. The loader is held by shared_ptr so handles
// stay valid even if the backend is torn down after the resources.
struct GLESBackendBuffer : BackendBuffer {
    GLESBackendBuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendBuffer() override {
        if (lib && lib->loaded && lib->glDeleteBuffers) lib->glDeleteBuffers(1, &handle);
    }
    void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                     const void* data) override {
        if (lib && lib->loaded && lib->glBufferData) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glBufferData(target, size, data, usage);
        }
    }
    void bufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                       const void* data) override {
        if (lib && lib->loaded && lib->glBufferSubData) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glBufferSubData(target, offset, size, data);
        }
    }
    void bufferStorage(uint32_t target, intptr_t size, uint32_t flags,
                       const void* data) override {
        if (lib && lib->loaded && lib->glBufferStorage) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glBufferStorage(target, size, data,
                                 static_cast<GLenum>(flags));
        }
    }
    void copySubData(uint32_t readTarget, uint32_t writeTarget,
                     intptr_t readOffset, intptr_t writeOffset,
                     intptr_t size) override {
        if (lib && lib->loaded && lib->glCopyBufferSubData) {
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
        if (lib && lib->loaded && lib->glMapBufferRange) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            return lib->glMapBufferRange(target, offset, length,
                                         static_cast<GLbitfield>(access));
        }
        return nullptr;
    }
    void unmapBuffer(uint32_t target) override {
        if (lib && lib->loaded && lib->glUnmapBuffer) {
            if (lib->glBindBuffer) lib->glBindBuffer(target, handle);
            lib->glUnmapBuffer(target);
        }
    }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendTexture : BackendTexture {
    GLESBackendTexture(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendTexture() override {
        if (lib && lib->loaded && lib->glDeleteTextures) lib->glDeleteTextures(1, &handle);
    }
    void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                     int width, int height, uint32_t format, uint32_t type,
                     const void* data) override {
        if (!lib || !lib->loaded || !lib->glTexImage2D) return;
        // glTexImage2D operates on the texture bound to `target` on the active
        // unit, so bind our handle first (SPEC §2.1 correctness: the driver's
        // currently bound texture must be ours, not whatever was bound before).
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexImage2D(target, level, static_cast<GLint>(internalFormat),
                         static_cast<GLsizei>(width),
                         static_cast<GLsizei>(height), 0,
                         format, type, data);
    }
    void texParameteri(uint32_t target, uint32_t pname, int param) override {
        if (!lib || !lib->loaded || !lib->glTexParameteri) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexParameteri(target, pname, param);
    }
    void texParameterf(uint32_t target, uint32_t pname, float param) override {
        if (!lib || !lib->loaded || !lib->glTexParameterf) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexParameterf(target, pname, param);
    }
    void texParameterfv(uint32_t target, uint32_t pname, const float* params,
                        int count) override {
        if (!lib || !lib->loaded || !lib->glTexParameterfv || !params) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexParameterfv(target, pname, params, count);
    }
    void texParameteriv(uint32_t target, uint32_t pname, const int* params,
                        int count) override {
        if (!lib || !lib->loaded || !lib->glTexParameteriv || !params) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexParameteriv(target, pname, params, count);
    }
    void texSubImage1D(uint32_t target, int level, int xoffset, int width,
                       uint32_t format, uint32_t type, const void* data) override {
        // OpenGL ES has no 1D textures; the call is a no-op on this backend.
        if (!lib || !lib->loaded || !lib->glTexSubImage1D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexSubImage1D(target, level, xoffset, width, format, type, data);
    }
    void texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                       int width, int height, uint32_t format, uint32_t type,
                       const void* data) override {
        if (!lib || !lib->loaded || !lib->glTexSubImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexSubImage2D(target, level, xoffset, yoffset, width, height,
                             format, type, data);
    }
    void texSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                       int zoffset, int width, int height, int depth,
                       uint32_t format, uint32_t type, const void* data) override {
        if (!lib || !lib->loaded || !lib->glTexSubImage3D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width,
                             height, depth, format, type, data);
    }
    void copyTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                        int x, int y, int width, int border) override {
        if (!lib || !lib->loaded || !lib->glCopyTexImage1D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCopyTexImage1D(target, level, internalFormat, x, y, width, border);
    }
    void copyTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                        int x, int y, int width, int height, int border) override {
        if (!lib || !lib->loaded || !lib->glCopyTexImage2D) return;
        if (lib->glBindTexture) lib->glBindTexture(target, handle);
        lib->glCopyTexImage2D(target, level, internalFormat, x, y, width, height,
                              border);
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendRenderbuffer : BackendRenderbuffer {
    GLESBackendRenderbuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendRenderbuffer() override {
        if (lib && lib->loaded && lib->glDeleteRenderbuffers)
            lib->glDeleteRenderbuffers(1, &handle);
    }
    void renderbufferStorage(uint32_t target, uint32_t internalFormat, int width,
                            int height) override {
        if (!lib || !lib->loaded || !lib->glRenderbufferStorage) return;
        // The renderbuffer must be bound to the target before storage is set.
        if (lib->glBindRenderbuffer) lib->glBindRenderbuffer(target, handle);
        lib->glRenderbufferStorage(target, static_cast<GLenum>(internalFormat),
                                  static_cast<GLsizei>(width),
                                  static_cast<GLsizei>(height));
    }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendFramebuffer : BackendFramebuffer {
    GLESBackendFramebuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendFramebuffer() override {
        if (lib && lib->loaded && lib->glDeleteFramebuffers)
            lib->glDeleteFramebuffers(1, &handle);
    }
    void framebufferTexture2D(uint32_t target, uint32_t attachment,
                              uint32_t texTarget, uint32_t nativeTexture,
                              int level) override {
        if (lib && lib->loaded && lib->glFramebufferTexture2D)
            lib->glFramebufferTexture2D(target, attachment, texTarget, nativeTexture,
                                      level);
    }
    void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                 uint32_t rbTarget,
                                 uint32_t nativeRenderbuffer) override {
        if (lib && lib->loaded && lib->glFramebufferRenderbuffer)
            lib->glFramebufferRenderbuffer(target, attachment, rbTarget,
                                          nativeRenderbuffer);
    }
    uint32_t checkStatus(uint32_t target) const override {
        if (lib && lib->loaded && lib->glCheckFramebufferStatus)
            return lib->glCheckFramebufferStatus(target);
        return 0x8CD5; // GL_FRAMEBUFFER_COMPLETE
    }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendVertexArray : BackendVertexArray {
    GLESBackendVertexArray(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendVertexArray() override {
        if (lib && lib->loaded && lib->glDeleteVertexArrays)
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
        if (lib && lib->loaded && lib->glGenSamplers)
            lib->glGenSamplers(1, &handle);
    }
    ~GLESBackendSampler() override {
        if (lib && lib->loaded && lib->glDeleteSamplers && handle)
            lib->glDeleteSamplers(1, &handle);
    }
    void samplerParameteri(uint32_t pname, int param) override {
        if (lib && lib->loaded && lib->glSamplerParameteri && handle)
            lib->glSamplerParameteri(handle, pname, param);
    }
    uint32_t nativeId() const override { return handle; }
    GLESLibPtr lib;
    GLuint handle = 0;
};

// Real GLES transform-feedback object (SPEC §13.3). The native TF object is
// created lazily at construction; capture state is driven through the loader.
struct GLESBackendTransformFeedback : BackendTransformFeedback {
    GLESBackendTransformFeedback(GLESLibPtr lib) : lib(lib) {
        if (lib && lib->loaded && lib->glGenTransformFeedbacks)
            lib->glGenTransformFeedbacks(1, &handle);
    }
    ~GLESBackendTransformFeedback() override {
        if (lib && lib->loaded && lib->glDeleteTransformFeedbacks && handle)
            lib->glDeleteTransformFeedbacks(1, &handle);
    }
    void begin(uint32_t mode) override {
        if (lib && lib->loaded && lib->glBeginTransformFeedback)
            lib->glBeginTransformFeedback(mode);
    }
    void end() override {
        if (lib && lib->loaded && lib->glEndTransformFeedback)
            lib->glEndTransformFeedback();
    }
    void pause() override {
        if (lib && lib->loaded && lib->glPauseTransformFeedback)
            lib->glPauseTransformFeedback();
    }
    void resume() override {
        if (lib && lib->loaded && lib->glResumeTransformFeedback)
            lib->glResumeTransformFeedback();
    }
    GLESLibPtr lib;
    GLuint handle = 0;
};

// Real GLES query object (SPEC §4 / §19). The native query is generated lazily
// at construction; begin/end drive the driver and queryResult reads the counter
// (via ui64v when available, falling back to uiv).
struct GLESBackendQuery : BackendQuery {
    GLESBackendQuery(GLESLibPtr lib) : lib(lib) {
        if (lib && lib->loaded && lib->glGenQueries)
            lib->glGenQueries(1, &handle);
    }
    ~GLESBackendQuery() override {
        if (lib && lib->loaded && lib->glDeleteQueries && handle)
            lib->glDeleteQueries(1, &handle);
    }
    void begin(uint32_t target) override {
        activeTarget = target;
        if (lib && lib->loaded && lib->glBeginQuery && handle)
            lib->glBeginQuery(target, handle);
    }
    void end() override {
        if (lib && lib->loaded && lib->glEndQuery)
            lib->glEndQuery(activeTarget); // target must match begin
    }
    void queryResult(int64_t* value, bool* available) override {
        *value = 0;
        *available = false;
        if (!lib || !lib->loaded || handle == 0) return;
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
        if (lib && lib->loaded && lib->glCreateShader)
            handle = lib->glCreateShader(stage);
    }
    ~GLESBackendShader() override {
        if (lib && lib->loaded && lib->glDeleteShader && handle)
            lib->glDeleteShader(handle);
    }
    bool compile(const std::string& source, std::string& log) override {
        if (!lib || !lib->loaded || handle == 0) {
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
        if (lib && lib->loaded && lib->glCreateProgram)
            handle = lib->glCreateProgram();
    }
    ~GLESBackendProgram() override {
        if (lib && lib->loaded && lib->glDeleteProgram && handle)
            lib->glDeleteProgram(handle);
    }
    void attach(BackendShader& shader) override {
        if (auto* gs = dynamic_cast<GLESBackendShader*>(&shader))
            lib->glAttachShader(handle, gs->handle);
    }
    bool link(std::string& log) override {
        if (!lib || !lib->loaded || handle == 0) {
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
        if (!lib || !lib->loaded || handle == 0) return -1;
        return static_cast<int>(lib->glGetAttribLocation(handle, name.c_str()));
    }
    uint32_t nativeId() const override { return handle; }

    int getUniformLocation(const std::string& name) const override {
        if (!lib || !lib->loaded || handle == 0) return -1;
        return static_cast<int>(lib->glGetUniformLocation(handle, name.c_str()));
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
