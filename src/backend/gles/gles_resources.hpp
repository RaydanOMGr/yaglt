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
        if (lib && lib->loaded && lib->glBufferData)
            lib->glBufferData(target, size, data, usage);
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
        if (lib && lib->loaded && lib->glTexImage2D)
            lib->glTexImage2D(target, level, static_cast<GLint>(internalFormat),
                             static_cast<GLsizei>(width),
                             static_cast<GLsizei>(height), 0,
                             format, type, data);
    }
    void texParameteri(uint32_t target, uint32_t pname, int param) override {
        if (lib && lib->loaded && lib->glTexParameteri)
            lib->glTexParameteri(target, pname, param);
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
