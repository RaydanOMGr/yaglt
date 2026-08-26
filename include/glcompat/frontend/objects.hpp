#pragma once

#include "glcompat/core/backend_resources.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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
    intptr_t size = 0;   // last glBufferData size
    uint32_t usage = 0;  // last glBufferData usage
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

    // Vertex attribute slot state (SPEC §2.1). Indexed by attribute location.
    struct AttribState {
        uint32_t index = 0;
        bool enabled = false;
        int32_t size = 4;
        uint32_t type = 0;
        bool normalized = false;
        int32_t stride = 0;
        intptr_t offset = 0;
    };
    std::vector<AttribState> attribs;

    AttribState& attrib(uint32_t index) {
        for (auto& a : attribs) {
            if (a.index == index) return a;
        }
        attribs.push_back(AttribState{});
        attribs.back().index = index;
        return attribs.back();
    }
};

// Frontend shader object (SPEC §8). Source + compile status live here, decoupled
// from the backend shader resource it owns.
class ShaderObject {
public:
    ShaderObject(GLObjectName n, uint32_t stage) : name(n), stage(stage) {}
    GLObjectName name = 0;
    uint32_t stage = 0;     // GL_VERTEX_SHADER / GL_FRAGMENT_SHADER / ...
    std::string source;
    bool compiled = false;
    std::string infoLog;
    std::unique_ptr<BackendShader> backend;
};

// Frontend program object (SPEC §8). Owns the attached shader list and the
// linked backend program resource.
class ProgramObject {
public:
    explicit ProgramObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::vector<GLObjectName> attachedShaders;
    bool linked = false;
    std::string infoLog;
    std::unique_ptr<BackendProgram> backend;
};

} // namespace glcompat
