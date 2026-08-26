#pragma once

namespace glcompat {

// Opaque backend resource handles. Concrete backends subclass these.
// The frontend stores std::unique_ptr<BackendX> and never inspects internals,
// so backend-native handles never leak into the generic frontend API.
class BackendBuffer {
public:
    virtual ~BackendBuffer() = default;
};
class BackendTexture {
public:
    virtual ~BackendTexture() = default;
};
class BackendRenderbuffer {
public:
    virtual ~BackendRenderbuffer() = default;
};
class BackendFramebuffer {
public:
    virtual ~BackendFramebuffer() = default;
};
class BackendVertexArray {
public:
    virtual ~BackendVertexArray() = default;
};
class BackendShader {
public:
    virtual ~BackendShader() = default;
};
class BackendProgram {
public:
    virtual ~BackendProgram() = default;
};

} // namespace glcompat
