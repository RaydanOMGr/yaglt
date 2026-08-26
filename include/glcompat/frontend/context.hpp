#pragma once

#include "glcompat/core/backend.hpp"
#include "glcompat/frontend/error.hpp"
#include "glcompat/frontend/objects.hpp"
#include "glcompat/state/gl_state.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace glcompat {

// Frontend OpenGL context. Owns object identity, name allocation, binding
// state, and validation. Talks to the backend only through IGraphicsBackend,
// never to a native API. This is the foundation the OpenGL 4.6 API entry
// points will dispatch into (SPEC §2.1, §10, §11).
class Context {
public:
    explicit Context(IGraphicsBackend& backend) : backend_(backend) {}

    IGraphicsBackend& backend() { return backend_; }

    // Push tracked pipeline state to the backend (SPEC §10). Calls
    // GLStateTracker::apply() on the backend's GLStateSink; the backend then
    // issues only the native calls whose state actually changed. No-op when the
    // backend exposes no sink. The frontend calls this at draw / state-flush
    // time so redundant native calls are skipped.
    void flushState();

    // Centralized pipeline state (SPEC §10). glEnable/glDisable/glBlendFunc/
    // glUseProgram/etc. route through here so a backend can avoid redundant
    // native calls via GLStateTracker::apply().
    GLStateTracker& state() { return state_; }

    // --- Error (SPEC §19) ---
    GLError getError();          // returns and clears the pending error
    void setError(GLError e);    // records the first error since last getError

    // --- Buffers ---
    GLObjectName genBuffer();
    void genBuffers(uint32_t n, GLObjectName* names);
    void bindBuffer(uint32_t target, GLObjectName name);
    GLObjectName boundBuffer(uint32_t target) const;
    void deleteBuffer(GLObjectName name);
    void deleteBuffers(uint32_t n, const GLObjectName* names);
    void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                    const void* data);
    BufferObject* getBuffer(GLObjectName name);

    // --- Indexed buffer bindings (SPEC §8) ---
    // Capability-guarded: binding a target the backend does not support
    // (e.g. SSBO on ES 3.0, UBO on ES 2.0) reports GL_INVALID_OPERATION
    // honestly instead of issuing an unsupported native call.
    void bindBufferBase(uint32_t target, uint32_t index, GLObjectName buffer);
    void bindBufferRange(uint32_t target, uint32_t index, GLObjectName buffer,
                         intptr_t offset, intptr_t size);

    // --- Textures ---
    GLObjectName genTexture();
    void genTextures(uint32_t n, GLObjectName* names);
    void bindTexture(GLObjectName name);
    GLObjectName boundTexture() const;
    void deleteTexture(GLObjectName name);
    void deleteTextures(uint32_t n, const GLObjectName* names);
    TextureObject* getTexture(GLObjectName name);

    // --- Textures (SPEC §2.1) ---
    // Operate on the currently bound texture. glTexImage2D allocates storage on
    // the backend resource; glTexParameteri records the parameter and pushes it to
    // the backend resource. Wrong/unknown targets are ignored like desktop GL.
    void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, uint32_t format, uint32_t type,
                    const void* data);
    void texParameteri(uint32_t target, uint32_t pname, int param);

    // --- Renderbuffers ---
    GLObjectName genRenderbuffer();
    void genRenderbuffers(uint32_t n, GLObjectName* names);
    void bindRenderbuffer(GLObjectName name);
    GLObjectName boundRenderbuffer() const;
    void deleteRenderbuffer(GLObjectName name);
    void deleteRenderbuffers(uint32_t n, const GLObjectName* names);
    RenderbufferObject* getRenderbuffer(GLObjectName name);

    // --- Framebuffers ---
    GLObjectName genFramebuffer();
    void genFramebuffers(uint32_t n, GLObjectName* names);
    void bindFramebuffer(GLObjectName name);
    GLObjectName boundFramebuffer() const;
    void deleteFramebuffer(GLObjectName name);
    void deleteFramebuffers(uint32_t n, const GLObjectName* names);
    FramebufferObject* getFramebuffer(GLObjectName name);

    // --- Framebuffers (SPEC §2.1) ---
    // Operate on the currently bound framebuffer. Attachment points are recorded
    // on the Frontend framebuffer object and forwarded to the backend resource.
    // Attaching a non-existent object reports GL_INVALID_OPERATION honestly.
    void framebufferTexture2D(uint32_t target, uint32_t attachment,
                              uint32_t texTarget, GLObjectName texture, int level);
    void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                 uint32_t rbTarget, GLObjectName renderbuffer);
    // Returns a GL_FRAMEBUFFER_* status code. Combines the structural check with
    // the backend resource's driver-level checkStatus().
    uint32_t checkFramebufferStatus(uint32_t target);

    // --- Pixel store (SPEC §10) ---
    // Records global pixel-store state in the tracker and pushes it to the
    // backend immediately (it affects subsequent texture/image uploads).
    void pixelStorei(uint32_t pname, int param);

    // --- Vertex arrays ---
    GLObjectName genVertexArray();
    void genVertexArrays(uint32_t n, GLObjectName* names);
    void bindVertexArray(GLObjectName name);
    GLObjectName boundVertexArray() const;
    void deleteVertexArray(GLObjectName name);
    void deleteVertexArrays(uint32_t n, const GLObjectName* names);
    VertexArrayObject* getVertexArray(GLObjectName name);

    // --- Shaders / programs (SPEC §8) ---
    // Capability-guarded: ShaderObjects / ProgramObjects must be supported by the
    // backend or these report GL_INVALID_OPERATION honestly. The desktop->backend
    // source translation (IShaderCompiler) runs inside compileShader so the
    // backend receives backend-compatible source.
    GLObjectName createShader(uint32_t stage);
    void shaderSource(GLObjectName shader, const std::string& src);
    void compileShader(GLObjectName shader);
    bool isShaderCompiled(GLObjectName shader) const;
    std::string shaderInfoLog(GLObjectName shader) const;
    void deleteShader(GLObjectName shader);
    ShaderObject* getShader(GLObjectName name);
    const ShaderObject* getShader(GLObjectName name) const;

    GLObjectName createProgram();
    void attachShader(GLObjectName program, GLObjectName shader);
    void linkProgram(GLObjectName program);
    bool isProgramLinked(GLObjectName program) const;
    std::string programInfoLog(GLObjectName program) const;
    int getAttribLocation(GLObjectName program, const std::string& name) const;
    void deleteProgram(GLObjectName program);
    ProgramObject* getProgram(GLObjectName name);
    const ProgramObject* getProgram(GLObjectName name) const;

    // --- Vertex attributes (SPEC §2.1) ---
    // Operate on the currently bound VAO (glBindVertexArray); with no VAO bound
    // they report GL_INVALID_OPERATION. State is recorded on the VAO and pushed
    // to the backend (via GLStateSink) at draw / flush time.
    void enableVertexAttribArray(uint32_t index);
    void disableVertexAttribArray(uint32_t index);
    void vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                             bool normalized, int32_t stride, intptr_t offset);

    // --- Draw (SPEC §2.1) ---
    // Flush tracked pipeline state to the backend, then issue the draw. Drawing
    // with no active program is GL_INVALID_OPERATION (core profile). Instanced
    // draws consult the capability table and report unsupported honestly.
    void drawArrays(uint32_t mode, int32_t first, int32_t count);
    void drawElements(uint32_t mode, int32_t count, uint32_t type,
                      intptr_t indices);
    void drawArraysInstanced(uint32_t mode, int32_t first, int32_t count,
                             int32_t primcount);
    void drawElementsInstanced(uint32_t mode, int32_t count, uint32_t type,
                               intptr_t indices, int32_t primcount);

    // --- Uniforms (SPEC §8) ---
    // Query a uniform location for an explicit program. Setting uniforms operates
    // on the currently active program (glUseProgram). No active program or a
    // non-linked program yields GL_INVALID_OPERATION; a -1 location is a silent
    // no-op (standard glUniform* semantics).
    int getUniformLocation(GLObjectName program, const std::string& name);
    void uniform1f(int loc, float v0);
    void uniform2f(int loc, float v0, float v1);
    void uniform3f(int loc, float v0, float v1, float v2);
    void uniform4f(int loc, float v0, float v1, float v2, float v3);
    void uniform1i(int loc, int v0);
    void uniform2i(int loc, int v0, int v1);
    void uniform3i(int loc, int v0, int v1, int v2);
    void uniform4i(int loc, int v0, int v1, int v2, int v3);
    void uniform1fv(int loc, const float* v, int count);
    void uniform1iv(int loc, const int* v, int count);
    void uniformMatrix4fv(int loc, const float* m, int count, bool transpose);

private:
    // Backend program for the currently active program (nullptr when none / not
    // linked / no backend resource). Used by the uniform setters.
    BackendProgram* activeBackendProgram();
    GLObjectName nextName_ = 1;

    IGraphicsBackend& backend_;
    GLError error_ = GLError::NoError;
    GLStateTracker state_;

    std::unordered_map<GLObjectName, std::unique_ptr<BufferObject>> buffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<TextureObject>> textures_;
    std::unordered_map<GLObjectName, std::unique_ptr<RenderbufferObject>> renderbuffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<FramebufferObject>> framebuffers_;
    std::unordered_map<GLObjectName, std::unique_ptr<VertexArrayObject>> vertexArrays_;
    std::unordered_map<GLObjectName, std::unique_ptr<ShaderObject>> shaders_;
    std::unordered_map<GLObjectName, std::unique_ptr<ProgramObject>> programs_;

    bool vertexStateDirty_ = false;

    std::unordered_map<uint32_t, GLObjectName> boundBuffers_;
    GLObjectName boundTexture_ = 0;
    GLObjectName boundRenderbuffer_ = 0;
    GLObjectName boundFramebuffer_ = 0;
    GLObjectName boundVertexArray_ = 0;
};

} // namespace glcompat
