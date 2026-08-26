#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/backend_resources.hpp"

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
    GLESLibPtr lib;
    GLuint handle = 0;
};

} // namespace glcompat
