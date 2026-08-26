#include "glcompat/frontend/gl_api.hpp"

namespace glcompat {

namespace {
Context* g_current = nullptr;

GLenum mapError(GLError e) {
    switch (e) {
    case GLError::NoError: return GL_NO_ERROR;
    case GLError::InvalidEnum: return GL_INVALID_ENUM;
    case GLError::InvalidValue: return GL_INVALID_VALUE;
    case GLError::InvalidOperation: return GL_INVALID_OPERATION;
    case GLError::InvalidName: return GL_INVALID_OPERATION;
    case GLError::OutOfMemory: return GL_INVALID_OPERATION;
    case GLError::NoCurrentContext: return GL_INVALID_OPERATION;
    }
    return GL_NO_ERROR;
}
} // namespace

void setCurrentContext(Context* ctx) { g_current = ctx; }
Context* getCurrentContext() { return g_current; }

GLenum glGetError() {
    if (g_current == nullptr) return GL_INVALID_OPERATION;
    return mapError(g_current->getError());
}

void glGenBuffers(GLsizei n, GLuint* buffers) {
    if (g_current == nullptr) return;
    g_current->genBuffers(static_cast<uint32_t>(n), buffers);
}

void glBindBuffer(GLenum target, GLuint buffer) {
    if (g_current == nullptr) return;
    g_current->bindBuffer(target, buffer);
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    if (g_current == nullptr) return;
    g_current->deleteBuffers(static_cast<uint32_t>(n), buffers);
}

void glBufferData(GLenum target, GLsizeiptr size, const GLvoid*, GLenum usage) {
    if (g_current == nullptr) return;
    g_current->bufferData(target, size, usage);
}

void glGenTextures(GLsizei n, GLuint* textures) {
    if (g_current == nullptr) return;
    g_current->genTextures(static_cast<uint32_t>(n), textures);
}

void glBindTexture(GLenum, GLuint texture) {
    if (g_current == nullptr) return;
    g_current->bindTexture(texture);
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    if (g_current == nullptr) return;
    g_current->deleteTextures(static_cast<uint32_t>(n), textures);
}

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    if (g_current == nullptr) return;
    g_current->genRenderbuffers(static_cast<uint32_t>(n), renderbuffers);
}

void glBindRenderbuffer(GLenum, GLuint renderbuffer) {
    if (g_current == nullptr) return;
    g_current->bindRenderbuffer(renderbuffer);
}

void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    if (g_current == nullptr) return;
    g_current->deleteRenderbuffers(static_cast<uint32_t>(n), renderbuffers);
}

void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    if (g_current == nullptr) return;
    g_current->genFramebuffers(static_cast<uint32_t>(n), framebuffers);
}

void glBindFramebuffer(GLenum, GLuint framebuffer) {
    if (g_current == nullptr) return;
    g_current->bindFramebuffer(framebuffer);
}

void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {
    if (g_current == nullptr) return;
    g_current->deleteFramebuffers(static_cast<uint32_t>(n), framebuffers);
}

void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    if (g_current == nullptr) return;
    g_current->genVertexArrays(static_cast<uint32_t>(n), arrays);
}

void glBindVertexArray(GLuint array) {
    if (g_current == nullptr) return;
    g_current->bindVertexArray(array);
}

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    if (g_current == nullptr) return;
    g_current->deleteVertexArrays(static_cast<uint32_t>(n), arrays);
}

// --- State management (SPEC §10) ---

void glEnable(GLenum cap) {
    if (g_current == nullptr) return;
    g_current->state().setCapability(cap, true);
}

void glDisable(GLenum cap) {
    if (g_current == nullptr) return;
    g_current->state().setCapability(cap, false);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    if (g_current == nullptr) return;
    g_current->state().setBlendFunc(sfactor, dfactor);
}

void glBlendEquation(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->state().setBlendEquation(mode);
}

void glUseProgram(GLuint prog) {
    if (g_current == nullptr) return;
    g_current->state().useProgram(prog);
}

void glDepthFunc(GLenum func) {
    if (g_current == nullptr) return;
    g_current->state().setDepthFunc(func);
}

void glDepthMask(bool flag) {
    if (g_current == nullptr) return;
    g_current->state().setDepthMask(flag);
}

void glCullFace(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->state().setCullFace(mode);
}

void glFrontFace(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->state().setFrontFace(mode);
}

void glFlushState() {
    if (g_current == nullptr) return;
    g_current->flushState();
}

} // namespace glcompat
