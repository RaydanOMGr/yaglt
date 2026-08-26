#pragma once

#include <string>

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
    // Native backend VAO id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};
class BackendShader {
public:
    virtual ~BackendShader() = default;

    // Compile already-translated source for this shader's stage. Returns true on
    // success; on failure fills `log` with a diagnostic. The frontend runs any
    // desktop->backend translation (via IShaderCompiler) before calling this, so
    // the backend receives backend-compatible source.
    virtual bool compile(const std::string& source, std::string& log) = 0;
};
class BackendProgram {
public:
    virtual ~BackendProgram() = default;

    // Attach a previously compiled backend shader to this program.
    virtual void attach(BackendShader& shader) = 0;
    // Link the attached shaders. Returns true on success; fills `log` on failure.
    virtual bool link(std::string& log) = 0;
    // Attribute location for `name` after linking (-1 if absent).
    virtual int getAttribLocation(const std::string& name) const = 0;
    // Native backend program id (e.g. driver GLuint). 0 when not linked.
    virtual uint32_t nativeId() const = 0;
};

} // namespace glcompat
