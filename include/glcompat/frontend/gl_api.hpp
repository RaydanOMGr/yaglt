#pragma once

#include "glcompat/frontend/gl_types.hpp"
#include "glcompat/frontend/context.hpp"

namespace glcompat {

// Public OpenGL-compatible API surface. These functions dispatch into the
// current frontend Context. They intentionally live in the `glcompat`
// namespace; a later shim can map them onto the global `gl*` symbols so an
// application can be linked against YAGLT as its GL provider.
//
// This is the frontend half (SPEC §2.1): semantics, naming, validation, and
// error behavior. Backend resource creation happens through the Context's
// IGraphicsBackend, never here.

// Current-context registry (one context active per thread in a full impl;
// single global for the headless foundation).
void setCurrentContext(Context* ctx);
Context* getCurrentContext();

GLenum glGetError();

void glGenBuffers(GLsizei n, GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glDeleteTextures(GLsizei n, const GLuint* textures);

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);

void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);

void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glBindVertexArray(GLuint array);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);

// --- State management (SPEC §10) ---
// These record state into the current context's GLStateTracker. The tracked
// state is pushed to the backend via glFlushState() at draw / flush time.
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendEquation(GLenum mode);
void glUseProgram(GLuint prog);
void glDepthFunc(GLenum func);
void glDepthMask(bool flag);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

// Flush tracked pipeline state to the backend (SPEC §10). Pushes only the
// state that changed since the last flush, so the driver is not re-set for
// unchanged state. Call this at draw / state-flush time.
void glFlushState();

} // namespace glcompat
