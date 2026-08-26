#pragma once

#include <string>

namespace glcompat {

// Opaque backend resource handles. Concrete backends subclass these.
// The frontend stores std::unique_ptr<BackendX> and never inspects internals,
// so backend-native handles never leak into the generic frontend API.
class BackendBuffer {
public:
    virtual ~BackendBuffer() = default;
    // Allocate/stream buffer storage (SPEC §2.1 glBufferData). `data` may be null.
    virtual void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                           const void* data) {}
};
class BackendTexture {
public:
    virtual ~BackendTexture() = default;
    // Allocate storage for a 2D texture level (SPEC §2.1 glTexImage2D).
    // `data` may be null.
    virtual void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                            int width, int height, uint32_t format, uint32_t type,
                            const void* data) {}
    // Set a texture parameter (filter / wrap), SPEC §2.1 glTexParameteri.
    virtual void texParameteri(uint32_t target, uint32_t pname, int param) {}
    // Native backend texture id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};
class BackendRenderbuffer {
public:
    virtual ~BackendRenderbuffer() = default;
    // Native backend renderbuffer id (e.g. driver GLuint). 0 when not applicable.
    virtual uint32_t nativeId() const { return 0; }
};
class BackendFramebuffer {
public:
    virtual ~BackendFramebuffer() = default;
    // Attach a texture level (SPEC §2.1 glFramebufferTexture2D). `nativeTexture`
    // is the backend-native texture id resolved by the frontend.
    virtual void framebufferTexture2D(uint32_t target, uint32_t attachment,
                                      uint32_t texTarget, uint32_t nativeTexture,
                                      int level) {}
    // Attach a renderbuffer (SPEC §2.1 glFramebufferRenderbuffer).
    virtual void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                         uint32_t rbTarget,
                                         uint32_t nativeRenderbuffer) {}
    // Returns a GL_FRAMEBUFFER_* status code. Defaults to Complete; real backends
    // query driver completeness.
    virtual uint32_t checkStatus(uint32_t /*target*/) const { return 0x8CD5; }
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

    // Uniform management (SPEC §8). Operate on this linked program. Defaults are
    // no-ops so backends opt in. `getUniformLocation` returns -1 when the uniform
    // is absent (matching desktop GL semantics). A -1 location is a silent no-op
    // in every setter, matching glUniform* behavior.
    virtual int getUniformLocation(const std::string& name) const { return -1; }
    virtual void uniform1f(int loc, float v0) {}
    virtual void uniform2f(int loc, float v0, float v1) {}
    virtual void uniform3f(int loc, float v0, float v1, float v2) {}
    virtual void uniform4f(int loc, float v0, float v1, float v2, float v3) {}
    virtual void uniform1i(int loc, int v0) {}
    virtual void uniform2i(int loc, int v0, int v1) {}
    virtual void uniform3i(int loc, int v0, int v1, int v2) {}
    virtual void uniform4i(int loc, int v0, int v1, int v2, int v3) {}
    virtual void uniform1fv(int loc, const float* v, int count) {}
    virtual void uniform1iv(int loc, const int* v, int count) {}
    virtual void uniformMatrix4fv(int loc, const float* m, int count,
                                 bool transpose) {}
};

} // namespace glcompat
