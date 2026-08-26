#pragma once

#include "glcompat/core/backend_resources.hpp"
#include <cstdint>
#include <memory>

namespace glcompat {

using GLObjectName = uint32_t;

// Frontend OpenGL object. Identity (the GL name) and OpenGL-visible state live
// here, decoupled from the backend resource it owns. The backend handle is an
// opaque unique_ptr so it can be recreated / lazily allocated / emulated
// without breaking OpenGL object identity (SPEC §11).
class BufferObject {
public:
    explicit BufferObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unique_ptr<BackendBuffer> backend;
    uint32_t target = 0; // last bound target, 0 = unbound
};

class TextureObject {
public:
    explicit TextureObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unique_ptr<BackendTexture> backend;
};

class RenderbufferObject {
public:
    explicit RenderbufferObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unique_ptr<BackendRenderbuffer> backend;
};

class FramebufferObject {
public:
    explicit FramebufferObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unique_ptr<BackendFramebuffer> backend;
};

class VertexArrayObject {
public:
    explicit VertexArrayObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unique_ptr<BackendVertexArray> backend;
};

} // namespace glcompat
