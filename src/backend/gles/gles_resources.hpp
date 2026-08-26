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
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendTexture : BackendTexture {
    GLESBackendTexture(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendTexture() override {
        if (lib && lib->loaded && lib->glDeleteTextures) lib->glDeleteTextures(1, &handle);
    }
    GLESLibPtr lib;
    GLuint handle = 0;
};

struct GLESBackendRenderbuffer : BackendRenderbuffer {
    GLESBackendRenderbuffer(GLESLibPtr lib, GLuint h) : lib(lib), handle(h) {}
    ~GLESBackendRenderbuffer() override {
        if (lib && lib->loaded && lib->glDeleteRenderbuffers)
            lib->glDeleteRenderbuffers(1, &handle);
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
    GLESLibPtr lib;
    GLuint handle = 0;
};

} // namespace glcompat
