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

// String queries (SPEC §22.2). VENDOR="YAGLT", RENDERER="YAGLT",
// VERSION="4.6.0 Compatibility Profile YAGLT" (major.minor.release per spec,
// vendor-specific suffix is implementation-dependent). Unknown name yields
// GL_INVALID_ENUM and nullptr.
const GLubyte* glGetString(GLenum name);

void glGenBuffers(GLsizei n, GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);

// Indexed buffer bindings (SPEC §8). Capability-guarded in the frontend:
// binding an unsupported target (e.g. SSBO on ES 3.0) yields GL_INVALID_OPERATION.
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer,
                       GLintptr offset, GLsizeiptr size);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glDeleteTextures(GLsizei n, const GLuint* textures);
// Selects the active texture image unit (SPEC §2.1). `texture` must be
// GL_TEXTURE0 + i within the supported unit range.
void glActiveTexture(GLenum texture);

// Direct State Access texture binding (SPEC §2.1, capability-gated by
// DirectStateAccess). glBindTextureUnit binds a texture to a specific unit
// without changing the active-texture selector; glBindTextures binds an array
// of textures to consecutive units for a single target.
void glBindTextureUnit(GLuint unit, GLuint texture);
void glBindTextures(GLuint first, GLsizei count, GLenum target,
                    const GLuint* textures);

// Texture storage + parameters (SPEC §2.1). Operate on the bound texture.
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                 GLsizei height, GLint border, GLenum format, GLenum type,
                 const GLvoid* data);
void glTexParameteri(GLenum target, GLenum pname, GLint param);

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width,
                         GLsizei height);

void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);

// Framebuffer attachments (SPEC §2.1).
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum texTarget,
                           GLuint texture, GLint level);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum rbTarget,
                               GLuint renderbuffer);
GLenum glCheckFramebufferStatus(GLenum target);

void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glBindVertexArray(GLuint array);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);

// --- Transform feedback (SPEC §13.3) ---
GLuint glGenTransformFeedback();
void glGenTransformFeedbacks(GLsizei n, GLuint* names);
void glBindTransformFeedback(GLuint name);
void glDeleteTransformFeedback(GLuint name);
void glDeleteTransformFeedbacks(GLsizei n, const GLuint* names);
void glBeginTransformFeedback(GLenum primitiveMode);
void glEndTransformFeedback();
void glPauseTransformFeedback();
void glResumeTransformFeedback();

// --- Sampler objects (SPEC §8.2) ---
GLuint glGenSampler();
void glGenSamplers(GLsizei n, GLuint* samplers);
void glBindSampler(GLuint unit, GLuint sampler);
void glDeleteSampler(GLuint sampler);
void glDeleteSamplers(GLsizei n, const GLuint* samplers);
GLboolean glIsSampler(GLuint sampler);
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param);
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params);

// --- Shaders / programs (SPEC §8) ---
// glCreateShader / glCreateProgram return the new object name (0 on failure).
GLuint glCreateShader(GLenum stage);
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* strings,
                    const GLint* lengths);
void glShaderSource(GLuint shader, const std::string& source);
void glCompileShader(GLuint shader);
GLint glGetShaderiv(GLuint shader, GLenum pname);
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length,
                       GLchar* infoLog);
void glDeleteShader(GLuint shader);

GLuint glCreateProgram();
void glAttachShader(GLuint program, GLuint shader);
void glLinkProgram(GLuint program);
GLint glGetProgramiv(GLuint program, GLenum pname);
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length,
                        GLchar* infoLog);
void glDeleteProgram(GLuint program);
GLint glGetAttribLocation(GLuint program, const GLchar* name);

// --- Uniforms (SPEC §8) ---
// glGetUniformLocation returns -1 for an unknown/non-linked program. The setters
// operate on the currently active program (glUseProgram); a -1 location is a
// silent no-op, matching glUniform* semantics.
GLint glGetUniformLocation(GLuint program, const GLchar* name);
void glUniform1f(GLint location, GLfloat v0);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform1i(GLint location, GLint v0);
void glUniform2i(GLint location, GLint v0, GLint v1);
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform1iv(GLint location, GLsizei count, const GLint* value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                       const GLfloat* value);

// --- Vertex attributes (SPEC §2.1) ---
void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                           GLboolean normalized, GLint stride,
                           const GLvoid* offset);

// --- State management (SPEC §10) ---
// These record state into the current context's GLStateTracker. The tracked
// state is pushed to the backend via glFlushState() at draw / flush time.
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendEquation(GLenum mode);
// Separate RGB/alpha blend factors and equations (SPEC §17.3). glBlendFunc and
// glBlendEquation set both RGB and alpha; the *Separate forms set them
// independently (RGB from the first pair, alpha from the second).
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha,
                        GLenum dstAlpha);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
// Constant blend color used by the GL_CONSTANT_* blend factors (SPEC §17.3).
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glUseProgram(GLuint prog);
void glDepthFunc(GLenum func);
void glDepthMask(bool flag);
void glDepthRange(GLdouble nearVal, GLdouble farVal);
void glDepthRangef(GLfloat nearVal, GLfloat farVal);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

// Stencil test state (SPEC §17.3.3). These set the front and back stencil state
// to identical values. Pushed to the backend via glFlushState() at draw/flush
// time (SPEC §10).
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilMask(GLuint mask);

// Pixel store (SPEC §10). Affects subsequent texture/image uploads.
void glPixelStorei(GLenum pname, GLint param);

// Viewport (glViewport) and scissor box (glScissor), recorded in the current
// context's GLStateTracker and pushed to the backend on the next flush (SPEC §10).
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);

// Clear values + clear (SPEC §2.1). glClearColor/glClearDepth record the
// per-context clear values; glClear flushes tracked state then clears the bound
// framebuffer for the given mask. An invalid mask reports GL_INVALID_VALUE.
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClearDepth(GLdouble depth);
void glClearDepthf(GLfloat depth);
void glClear(GLuint mask);

// Command stream flush / finish (SPEC §2.1).
void glFlush();
void glFinish();

// Read back pixels from the bound framebuffer (SPEC §2.1). Non-positive
// width/height reports GL_INVALID_VALUE.
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                  GLenum type, GLvoid* pixels);

// Flush tracked pipeline state to the backend (SPEC §10). Pushes only the
// state that changed since the last flush, so the driver is not re-set for
// unchanged state. Call this at draw / state-flush time.
void glFlushState();

// State queries (SPEC §22). glGet* read the frontend-owned tracked state, so
// they never round-trip to the backend driver (SPEC §10). An unknown pname
// yields GL_INVALID_ENUM; a null buffer yields GL_INVALID_VALUE. glIsEnabled
// returns the enabled state of a tracked capability (GL_INVALID_ENUM for an
// untracked one).
void glGetBooleanv(GLenum pname, GLboolean* params);
void glGetIntegerv(GLenum pname, GLint* params);
void glGetFloatv(GLenum pname, GLfloat* params);
void glGetDoublev(GLenum pname, GLdouble* params);
GLboolean glIsEnabled(GLenum cap);

// --- Draw commands (SPEC §2.1) ---
// The frontend flushes tracked pipeline state to the backend immediately before
// issuing the draw, so redundant native state calls are skipped. Drawing with no
// active program yields GL_INVALID_OPERATION; instanced draws consult the
// capability table and report unsupported honestly.
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
                             const GLvoid* indices, GLsizei primcount);

} // namespace glcompat
