#pragma once

#include "glcompat/core/backend_resources.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include <cstdint>
#include <map>
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
    intptr_t size = 0;   // last glBufferData/glBufferStorage size
    uint32_t usage = 0;  // last glBufferData/glBufferStorage usage

    // CPU-side data store (SPEC §6). YAGLT keeps an authoritative mirror of the
    // buffer contents so map/getBufferParameter/subdata/copy semantics are fully
    // defined even on a backend without native storage (the mock). Real backends
    // additionally receive the data via the BackendBuffer so the driver copy stays
    // consistent; the frontend mirror is the source of truth for queries and maps.
    std::vector<uint8_t> store;

    // Immutable storage (glBufferStorage). Once true, size/usage/flags are fixed
    // and a further glBufferData/glBufferStorage reports GL_INVALID_OPERATION.
    bool immutable = false;
    uint32_t immutableFlags = 0;

    // Mapping state (glMapBuffer / glMapBufferRange / glUnmapBuffer).
    bool mapped = false;
    intptr_t mapOffset = 0;
    intptr_t mapLength = 0;
    uint32_t mapAccess = 0;
    // Stable pointer to the mapped region of the CPU mirror (SPEC §6.1.1
    // BUFFER_MAP_POINTER). Cleared to nullptr on unmap.
    void* mapPointer = nullptr;
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
        int depth = 0;
        uint32_t format = 0;
        uint32_t type = 0;
        bool hasData = false;
    };

    GLObjectName name = 0;
    uint32_t target = GL_TEXTURE_2D;        // last bound/targeted target
    std::unordered_map<uint32_t, int> params;   // scalar int pname -> param
    std::unordered_map<uint32_t, float> paramsf; // scalar float pname -> param
    std::unordered_map<uint32_t, std::vector<float>> paramsfv; // float vector
    std::unordered_map<uint32_t, std::vector<int>> paramsiv;    // int vector
    std::unordered_map<uint32_t, std::vector<int32_t>> paramsIiv; // integer (signed) vector
    std::unordered_map<uint32_t, std::vector<uint32_t>> paramsIuiv; // integer (unsigned) vector
    std::vector<Image> images;              // allocated levels (glTexImage2D)
    bool storageSet = false;

    // Immutable storage (glTextureStorage*D, DSA / SPEC §8.1). Once set the
    // levels and base dimensions drive glGetTextureLevelParameter* queries.
    int storageLevels = 0;
    int storageBaseWidth = 0;
    int storageBaseHeight = 0;
    int storageBaseDepth = 0;
    uint32_t storageInternalFormat = 0;
    bool immutableStorage = false;

    // Texture view state (SPEC §8.19 glTextureView). A view shares immutable
    // storage with its source texture but reinterprets a level/layer subrange and
    // possibly a different (compatible) internal format. The source texture must
    // already have immutable storage; the view derives its own storage metadata
    // (levels/base dimensions) from the source for level queries.
    bool isView = false;
    GLObjectName viewSource = 0;
    uint32_t viewInternalFormat = 0;
    uint32_t viewMinLevel = 0;
    uint32_t viewNumLevels = 0;
    uint32_t viewMinLayer = 0;
    uint32_t viewNumLayers = 0;

    // Recorded sub-image uploads (glTexSubImage*D), used for completeness queries
    // and tests. The backend resource sees the native call.
    struct SubImage {
        uint32_t target = 0;
        int level = 0;
        int xoffset = 0, yoffset = 0, zoffset = 0;
        int width = 0, height = 0, depth = 0;
        uint32_t format = 0, type = 0;
        int dim = 2; // 1, 2, or 3
    };
    std::vector<SubImage> subimages;

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
    int samples = 0;
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
        int layer = 0;         // layer for glNamedFramebufferTextureLayer
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
        // ARRAY_BUFFER bound when this attrib was specified. On GLES the attribute
        // buffer binding is captured from the bound ARRAY_BUFFER at gl*VertexAttrib
        // Pointer time; the flush binds this buffer before issuing the native call
        // (SPEC §2.1 / GLES: no client-side vertex arrays).
        GLObjectName buffer = 0;
        // Per-attribute divisor (SPEC §10, glVertexAttribDivisor). 0 = advance once
        // per vertex (the GL default); >0 advances once per `divisor` instances.
        uint32_t divisor = 0;
        // Vertex buffer binding point this attribute sources from (SPEC §10.3.1,
        // the separate attribute-format model introduced with DSA). Defaults to
        // the attribute index so the legacy single-binding path
        // (glVertexAttribPointer) keeps working unchanged.
        uint32_t binding = 0;
        // Offset of this attribute's first component within its vertex buffer
        // binding (glVertexArrayAttrib*Format relativeoffset).
        intptr_t relativeoffset = 0;
        // Current generic vertex attribute value (SPEC §10.2). Used when the
        // attribute is disabled (not sourced from a vertex buffer array). GL
        // keeps these as double-precision; the I (integer) family stores integral
        // values that round-trip through glGetVertexAttribiv. Defaults match the
        // GL initial state (w = 1).
        double currentValue[4] = {0.0, 0.0, 0.0, 1.0};
        uint32_t currentType = GL_FLOAT; // GL_FLOAT / GL_INT / GL_UNSIGNED_INT
    };
    std::vector<AttribState> attribs;

    // Vertex buffer binding points (SPEC §10.3.1). Indexed by binding index; a
    // binding couples a buffer object with a base offset, stride, and divisor.
    // The separate-format model lets several attributes share one interleaved
    // buffer through distinct binding points and relative offsets.
    struct VertexBufferBinding {
        GLObjectName buffer = 0;
        intptr_t offset = 0;
        int32_t stride = 0;
        uint32_t divisor = 0;
    };
    std::map<uint32_t, VertexBufferBinding> bindings;

    // Element array buffer bound to this VAO (glVertexArrayElementBuffer,
    // SPEC §10.3.1). 0 means no element buffer is attached.
    GLObjectName elementBuffer = 0;

    AttribState& attrib(uint32_t index) {
        for (auto& a : attribs) {
            if (a.index == index) return a;
        }
        attribs.push_back(AttribState{});
        attribs.back().index = index;
        return attribs.back();
    }
    VertexBufferBinding& binding(uint32_t index) { return bindings[index]; }
};

// Frontend sampler object (SPEC §8.2). Owns an opaque backend sampler resource
// and records the scalar sampler parameters set via glSamplerParameteri so they
// can be queried by glGetSamplerParameteriv.
class SamplerObject {
public:
    explicit SamplerObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unordered_map<uint32_t, int> params; // pname -> param
    std::unique_ptr<BackendSampler> backend;
};

// Frontend transform-feedback object (SPEC §13.3). Owns an opaque backend TF
// resource; capture begin/end/pause/resume are forwarded to it at draw time.
class TransformFeedbackObject {
public:
    explicit TransformFeedbackObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::unique_ptr<BackendTransformFeedback> backend;
};

// Frontend query object (SPEC §4 / §19). Owns an opaque backend query resource
// and records whether it is currently active (between begin/end) and the target
// it was begun against. The last query result is cached on the frontend so
// glGetQueryObject* reads frontend-owned state (SPEC §10).
class QueryObject {
public:
    explicit QueryObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    uint32_t target = 0;   // target used at beginQuery
    bool active = false;   // currently between beginQuery / endQuery
    std::unique_ptr<BackendQuery> backend;
};

// Frontend sync object (SPEC §4 / §20, ARB_sync). Fence syncs order GPU command
// completion. The frontend owns the object and hands back an opaque GLsync
// pointer; the condition/flags are recorded here.
class SyncObject {
public:
    explicit SyncObject(uint32_t id) : name(id) {}
    GLObjectName name = 0;
    uint32_t condition = 0; // GL_SYNC_GPU_COMMANDS_COMPLETE
    uint32_t flags = 0;
    bool signaled = false;   // becomes true once commands complete
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
    // Loaded shader binary (glShaderBinary). A binary fully defines the shader
    // source, so loading one marks the shader compiled (SPEC §7.2).
    std::vector<uint8_t> binary;
    uint32_t binaryFormat = 0;
};

// Frontend program object (SPEC §8). Owns the attached shader list and the
// linked backend program resource. `separable` marks a program linked for use
// with a program pipeline (glCreateShaderProgramv / PROGRAM_SEPARABLE).
class ProgramObject {
public:
    explicit ProgramObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    std::vector<GLObjectName> attachedShaders;
    bool linked = false;
    bool separable = false;
    bool binaryRetrievableHint = false;
    std::string infoLog;
    std::unique_ptr<BackendProgram> backend;
    // Loaded program binary (glProgramBinary) / retrieved blob (glGetProgramBinary).
    // The frontend is the authoritative mirror of the binary, matching the buffer
    // mirror pattern (SPEC §6). A binary fully defines the program, so loading one
    // marks the program linked.
    std::vector<uint8_t> binary;
    uint32_t binaryFormat = 0;
    // Generic attribute bindings requested via glBindAttribLocation before link.
    // name -> index; applied to the backend program at the next link (SPEC §7.3.7).
    std::map<std::string, int> attribBindings;
};

// Frontend program-pipeline object (SPEC §7.4). Maps each shader stage to the
// program that supplies it, plus the "active program" used by
// glUseProgramStages(pipeline, stages, 0). State is frontend-owned; the
// pipeline is bound to the backend via GLStateSink::bindProgramPipeline.
class ProgramPipelineObject {
public:
    explicit ProgramPipelineObject(GLObjectName n) : name(n) {}
    GLObjectName name = 0;
    // Stage bit (GL_VERTEX_SHADER_BIT, ...) -> program name. A zero value means
    // the stage is not supplied by this pipeline.
    std::unordered_map<uint32_t, GLObjectName> stagePrograms;
    GLObjectName activeProgram = 0; // set by glActiveShaderProgram
    bool validated = false;
    std::string infoLog;
};

} // namespace glcompat
