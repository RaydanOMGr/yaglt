#pragma once

#include "glcompat/core/backend_resources.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
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

    // Texture state tracked on the frontend (decoupled from backend storage).
    // The backend resource only sees the resolved native calls.
    struct Image {
        int level = 0;
        uint32_t internalFormat = 0;
        int width = 0;
        int height = 0;
        uint32_t format = 0;
        uint32_t type = 0;
        bool hasData = false;
    };

    GLObjectName name = 0;
    uint32_t target = GL_TEXTURE_2D;        // last bound/targeted target
    std::unordered_map<uint32_t, int> params; // pname -> param
    std::vector<Image> images;              // allocated levels (glTexImage2D)
    bool storageSet = false;
    std::unique_ptr<BackendTexture> backend;
};

class RenderbufferObject {
public:
    explicit RenderbufferObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    // Storage recorded on the frontend (decoupled from backend allocation).
    uint32_t internalFormat = 0;
    int width = 0;
    int height = 0;
    bool storageSet = false;
    std::unique_ptr<BackendRenderbuffer> backend;
};

class FramebufferObject {
public:
    explicit FramebufferObject(GLObjectName n) : name(n) {}

    // An attachment point on this framebuffer (SPEC §2.1).
    struct Attachment {
        uint32_t attachment = 0; // GL_COLOR_ATTACHMENT0 / GL_DEPTH_ATTACHMENT / ...
        uint32_t type = 0;      // 0=texture, 1=renderbuffer
        GLObjectName name = 0;  // frontend object name
        uint32_t texTarget = 0; // relevant for texture attachments
        int level = 0;
    };

    GLObjectName name = 0;
    std::vector<Attachment> attachments;
    std::unique_ptr<BackendFramebuffer> backend;

    // Frontend-side completeness check (SPEC §2.1): an FBO is complete only when
    // it has at least one attachment and every attachment references an existing
    // object. Backend completeness (format support) is queried via the backend
    // resource's checkStatus(); this is the structural precondition.
    bool isStructurallyComplete() const {
        if (attachments.empty()) return false;
        for (const auto& a : attachments) {
            if (a.name == 0) return false;
        }
        return true;
    }
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
