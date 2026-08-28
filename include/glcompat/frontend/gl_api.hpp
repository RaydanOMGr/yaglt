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

// Buffer sub-data / immutable storage / copy (SPEC §6).
void glBufferSubData(GLenum target, GLsizeiptr offset, GLsizeiptr size,
                     const GLvoid* data);
void glBufferStorage(GLenum target, GLsizeiptr size, const GLvoid* data,
                     GLbitfield flags);
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                         GLintptr readOffset, GLintptr writeOffset,
                         GLsizeiptr size);

// Buffer parameter queries (SPEC §6 / §22). Reads frontend-owned buffer state.
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params);
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64* params);

// Buffer mapping (SPEC §6). Returns a pointer into the frontend data store, or
// nullptr on error. glUnmapBuffer returns GL_TRUE on success.
GLvoid* glMapBuffer(GLenum target, GLenum access);
GLvoid* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                        GLbitfield access);
GLboolean glUnmapBuffer(GLenum target);

// Buffer data read-back / clear / discard (SPEC §6). The frontend keeps an
// authoritative CPU mirror of each buffer's data store, so getBufferSubData
// reads exact bytes and clear*BufferData fills the mirror in-memory (then
// re-uploads the affected range to the backend, which has no native
// glClearBufferData). invalidate*BufferData relays a driver discard hint.
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                       GLvoid* data);
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size,
                            GLvoid* data);
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format,
                      GLenum type, const GLvoid* data);
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format,
                           GLenum type, const GLvoid* data);
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset,
                         GLsizeiptr size, GLenum format, GLenum type,
                         const GLvoid* data);
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat,
                             GLintptr offset, GLsizeiptr size, GLenum format,
                             GLenum type, const GLvoid* data);
void glInvalidateBufferData(GLenum target);
void glInvalidateBufferSubData(GLenum target, GLintptr offset, GLsizeiptr length);
void glInvalidateNamedBufferData(GLuint buffer);
void glInvalidateNamedBufferSubData(GLuint buffer, GLintptr offset,
                                   GLsizeiptr length);

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
void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                  GLenum format, GLenum type, const GLvoid* data);
void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                  GLsizei height, GLsizei depth, GLenum format, GLenum type,
                  const GLvoid* data);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
// Texture parameter setters (SPEC §8). glTexParameterf sets a float scalar;
// glTexParameterfv/iv set vector parameters (e.g. GL_TEXTURE_BORDER_COLOR).
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params,
                      GLsizei count);
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params,
                      GLsizei count);
// Integer (signed / unsigned) texture parameter setters + queries (SPEC §8.1).
void glTexParameterIiv(GLenum target, GLenum pname, const GLint* params);
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint* params);
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params);
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params);
// Regenerate the mipmap chain for the bound texture (SPEC §8.1 glGenerateMipmap).
void glGenerateMipmap(GLenum target);
// Invalidate texture contents (SPEC §8.1). glInvalidateTexImage discards the
// whole level; glInvalidateTexSubImage discards a sub-region.
void glInvalidateTexImage(GLenum target, GLint level);
void glInvalidateTexSubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                             GLint zoffset, GLsizei width, GLsizei height, GLsizei depth);
// Texture parameter queries (SPEC §8.1). glGetTexParameterfv reads a float
// scalar or the first component of a float vector parameter.
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
// Texture sub-image specification (SPEC §8.6 TexSubImage*D).
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width,
                     GLenum format, GLenum type, const GLvoid* pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type,
                     const GLvoid* pixels);
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type, const GLvoid* pixels);
// Define a texture image by copying from the framebuffer (SPEC §8.5 CopyTexImage*D).
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x,
                      GLint y, GLsizei width, GLint border);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x,
                      GLint y, GLsizei width, GLsizei height, GLint border);
// Texture parameter queries (SPEC §8.1). glGetTexParameteriv reads the bound
// texture for `target`; glGetTextureParameteriv is the DSA variant for an
// explicit texture object (capability-gated by DirectStateAccess).
void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params);
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
                   GLvoid* pixels);
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params);

// Direct State Access texture surface (SPEC §2.1 / §8.1). Operate on an explicit
// named texture object instead of the bound one; capability-gated by
// DirectStateAccess (Emulated: YAGLT emulates DSA via the object's backend).
void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalFormat,
                        GLsizei width);
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalFormat,
                        GLsizei width, GLsizei height);
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalFormat,
                        GLsizei width, GLsizei height, GLsizei depth);
// Texture views (SPEC §8.19): create `texture` as an alias that shares the
// immutable storage of `origtexture`, exposing a level/layer subrange under a
// (compatible) `internalformat`. `origtexture` must already have immutable
// storage; capability-gated by TextureViews.
void glTextureView(GLuint texture, GLenum target, GLuint origtexture,
                  GLenum internalformat, GLuint minlevel, GLuint numlevels,
                  GLuint minlayer, GLuint numlayers);
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width,
                         GLenum format, GLenum type, const GLvoid* pixels);
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                         GLsizei width, GLsizei height, GLenum format, GLenum type,
                         const GLvoid* pixels);
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                         GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, const GLvoid* pixels);
void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param);
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params,
                          GLsizei count);
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params,
                           GLsizei count);
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint* params);
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint* params);
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params);
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params);
void glGenerateTextureMipmap(GLuint texture);
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params);
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname,
                                  GLint* params);
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname,
                                  GLfloat* params);
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type,
                       GLvoid* pixels);
void glTextureBuffer(GLuint texture, GLenum internalFormat, GLuint buffer);
void glTextureBufferRange(GLuint texture, GLenum internalFormat, GLuint buffer,
                           GLintptr offset, GLsizeiptr size);

// Non-DSA texture storage (SPEC §8.5). Operate on the texture bound to `target`.
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat,
                     GLsizei width);
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat,
                     GLsizei width, GLsizei height);
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat,
                     GLsizei width, GLsizei height, GLsizei depth);
void glTexBuffer(GLenum target, GLenum internalFormat, GLuint buffer);
void glTexBufferRange(GLenum target, GLenum internalFormat, GLuint buffer,
                       GLintptr offset, GLsizeiptr size);
// Multisample texture storage (SPEC §8.19).
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                               GLsizei width, GLsizei height,
                               GLboolean fixedsamplelocations);
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLboolean fixedsamplelocations);
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                             GLsizei width, GLsizei height,
                             GLboolean fixedsamplelocations);
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                             GLsizei width, GLsizei height, GLsizei depth,
                             GLboolean fixedsamplelocations);
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples,
                                   GLenum internalFormat, GLsizei width,
                                   GLsizei height, GLboolean fixedsamplelocations);
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples,
                                   GLenum internalFormat, GLsizei width,
                                   GLsizei height, GLsizei depth,
                                   GLboolean fixedsamplelocations);

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width,
                         GLsizei height);

// Direct State Access renderbuffer surface (SPEC §8.2 / §9.2).
void glCreateRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalFormat,
                                GLsizei width, GLsizei height);
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples,
                                          GLenum internalFormat, GLsizei width,
                                          GLsizei height);
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname,
                                      GLint* params);

void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);

// Framebuffer attachments (SPEC §2.1).
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum texTarget,
                           GLuint texture, GLint level);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum rbTarget,
                               GLuint renderbuffer);
GLenum glCheckFramebufferStatus(GLenum target);

// Direct State Access framebuffer surface (SPEC §9.2). glCreateFramebuffers
// generates names; the named attach / parameter / query / status entry points
// operate on an explicit framebuffer without touching the bound one. Named
// blit/invalidate/clear bind the named framebuffer(s) to the driver and reuse
// the whole-framebuffer backend ops.
void glCreateFramebuffers(GLsizei n, GLuint* framebuffers);
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment,
                                   GLenum renderbufferTarget, GLuint renderbuffer);
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment,
                              GLuint texture, GLint level);
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment,
                                   GLuint texture, GLint level, GLint layer);
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target);
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param);
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname,
                                     GLint* params);
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer,
                                               GLenum attachment, GLenum pname,
                                               GLint* params);
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments,
                                     const GLenum* attachments);
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments,
                                        const GLenum* attachments, GLint x,
                                        GLint y, GLsizei width, GLsizei height);
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                              const GLint* value);
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                               const GLuint* value);
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                              const GLfloat* value);
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                              GLfloat depth, GLint stencil);

void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glBindVertexArray(GLuint array);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
GLboolean glIsVertexArray(GLuint array);

// Direct State Access vertex-array surface (SPEC §10.3.1).
void glCreateVertexArrays(GLsizei n, GLuint* arrays);
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer);
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index);
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer,
                               GLintptr offset, GLsizei stride);
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count,
                                const GLuint* buffers, const GLintptr* offsets,
                                const GLsizei* strides);
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size,
                               GLenum type, GLboolean normalized,
                               GLuint relativeoffset);
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size,
                                GLenum type, GLuint relativeoffset);
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size,
                                GLenum type, GLuint relativeoffset);
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex,
                                GLuint bindingindex);
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex,
                                 GLuint divisor);

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

// --- Query objects (SPEC §4 / §19) ---
GLuint glGenQuery();
void glGenQueries(GLsizei n, GLuint* names);
void glDeleteQuery(GLuint id);
void glDeleteQueries(GLsizei n, const GLuint* names);
GLboolean glIsQuery(GLuint id);
void glBeginQuery(GLenum target, GLuint id);
void glEndQuery(GLenum target);
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id);
void glEndQueryIndexed(GLenum target, GLuint index);
void glGetQueryiv(GLenum target, GLenum pname, GLint* params);
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params);
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params);
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params);
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params);

// --- Sync objects (SPEC §4 / §20, ARB_sync) ---
GLsync glFenceSync(GLenum condition, GLbitfield flags);
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
void glDeleteSync(GLsync sync);
GLboolean glIsSync(GLsync sync);
void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length,
                GLint* values);

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
void glProgramParameteri(GLuint program, GLenum pname, GLint value);
GLint glGetProgramiv(GLuint program, GLenum pname);
void glProgramBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length);
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length,
                        GLenum* binaryFormat, void* binary);
void glShaderBinary(GLsizei count, const GLuint* shaders, GLenum binaryFormat,
                    const void* binary, GLsizei length);
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length,
                        GLchar* infoLog);
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface,
                                const GLchar* name);
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index,
                              GLsizei bufSize, GLsizei* length, GLchar* name);
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index,
                            GLsizei propCount, const GLenum* props, GLsizei bufSize,
                            GLsizei* length, GLint* params);
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface,
                                   const GLchar* name);
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface,
                                         const GLchar* name);

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize,
                        GLsizei* length, GLint* size, GLenum* type, GLchar* name);
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize,
                       GLsizei* length, GLint* size, GLenum* type, GLchar* name);
GLuint glGetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName);
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex,
                              GLenum pname, GLint* params);
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex,
                                 GLsizei bufSize, GLsizei* length,
                                 GLchar* uniformBlockName);

GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar* name);
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype,
                                     const GLchar* name);
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index,
                                    GLenum pname, GLint* values);
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype,
                                      GLuint index, GLsizei bufSize, GLsizei* length,
                                      GLchar* name);
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index,
                               GLsizei bufSize, GLsizei* length, GLchar* name);
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count,
                            const GLuint* indices);
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint* params);
void glDeleteProgram(GLuint program);
GLint glGetAttribLocation(GLuint program, const GLchar* name);
void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name);

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
void glEnablei(GLenum cap, GLuint index);
void glDisablei(GLenum cap, GLuint index);
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

// --- Program pipelines (SPEC §7.4) ---
// Build a single-stage separable program (glCreateShaderProgramv). Returns the
// new program name (query LINK_STATUS / INFO_LOG for success).
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar* const* strings);
void glGenProgramPipelines(GLsizei n, GLuint* pipelines);
void glDeleteProgramPipelines(GLsizei n, const GLuint* pipelines);
GLboolean glIsProgramPipeline(GLuint pipeline);
void glBindProgramPipeline(GLuint pipeline);
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program);
void glActiveShaderProgram(GLuint pipeline, GLuint program);
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params);
void glValidateProgramPipeline(GLuint pipeline);
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei* length,
                                 GLchar* infoLog);
void glDepthFunc(GLenum func);
void glDepthMask(bool flag);
void glDepthRange(GLdouble nearVal, GLdouble farVal);
void glDepthRangef(GLfloat nearVal, GLfloat farVal);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

// Rasterization scalar state (SPEC §11). Recorded in the current context's
// GLStateTracker and pushed to the backend only when the value changes (SPEC
// §10). glPointSize / glLineWidth / glPolygonOffset map directly to GLES3.
void glPointSize(GLfloat size);
void glLineWidth(GLfloat width);
void glPolygonOffset(GLfloat factor, GLfloat units);

// Rasterization polygon mode (SPEC §11.1, glPolygonMode). Per-side render mode;
// unsupported `face`/`mode` report GL_INVALID_ENUM honestly (the tracker rejects
// them).
void glPolygonMode(GLenum face, GLenum mode);

// Multisample raster state (SPEC §11.5). glSampleMaski sets one mask word
// (maskNumber must be < MAX_SAMPLE_MASK_WORDS, else GL_INVALID_VALUE).
// glMinSampleShading sets the minimum sample-shading fraction in [0,1] (else
// GL_INVALID_VALUE).
void glSampleMaski(GLuint maskNumber, GLuint mask);
void glMinSampleShading(GLfloat value);
// Provoking vertex convention (SPEC §11, glProvokingVertex). `mode` must be
// GL_FIRST_VERTEX_CONVENTION or GL_LAST_VERTEX_CONVENTION (else GL_INVALID_ENUM).
void glProvokingVertex(GLenum mode);

// Stencil test state (SPEC §17.3.3). These set the front and back stencil state
// to identical values. Pushed to the backend via glFlushState() at draw/flush
// time (SPEC §10).
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilMask(GLuint mask);
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilMaskSeparate(GLenum face, GLuint mask);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glSampleCoverage(GLfloat value, GLboolean invert);

// Primitive restart index (SPEC §10.4, glPrimitiveRestartIndex). Activation is via
// glEnable(GL_PRIMITIVE_RESTART); this records the restart index, pushed to the
// backend only when it changes (SPEC §10).
void glPrimitiveRestartIndex(GLuint index);

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

// Whole-framebuffer buffer selection (SPEC §15 / §16). glDrawBuffers selects the
// draw buffers for the bound framebuffer; glReadBuffer selects its read buffer.
void glDrawBuffers(GLsizei n, const GLenum* bufs);
void glReadBuffer(GLenum buf);

// Color logic op (SPEC §17.3.4, glLogicOp). Capability-gated (LogicOp).
void glLogicOp(GLenum mode);

// Color clamping (SPEC §15.2.3, glClampColor). `target` must be
// GL_CLAMP_READ_COLOR; `mode` is GL_TRUE / GL_FALSE / GL_FIXED_ONLY.
void glClampColor(GLenum target, GLenum mode);

// Whole-framebuffer copy / invalidate (SPEC §15 / §16). glBlitFramebuffer copies a
// rectangle of the bound read framebuffer into the bound draw framebuffer; an
// invalid mask reports GL_INVALID_VALUE. glInvalidateFramebuffer /
// glInvalidateSubFramebuffer discard the listed attachments.
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter);
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments,
                            const GLenum* attachments);
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments,
                               const GLenum* attachments, GLint x, GLint y,
                               GLsizei width, GLsizei height);

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
GLboolean glIsEnabledi(GLenum cap, GLuint index);

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

// Vertex attribute divisor (SPEC §10, glVertexAttribDivisor). Sets the per-
// attribute instance step rate on the bound VAO; 0 = per-vertex, >0 = per-instance.
void glVertexAttribDivisor(GLuint index, GLuint divisor);

// Current generic vertex attribute values (SPEC §10.2). Recorded on the bound
// VAO; used when an attribute is disabled (not array-sourced). Out-of-range
// index -> GL_INVALID_VALUE; no VAO bound -> GL_INVALID_OPERATION.
void glVertexAttrib1f(GLuint index, GLfloat x);
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y);
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z);
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertexAttrib1fv(GLuint index, const GLfloat* v);
void glVertexAttrib2fv(GLuint index, const GLfloat* v);
void glVertexAttrib3fv(GLuint index, const GLfloat* v);
void glVertexAttrib4fv(GLuint index, const GLfloat* v);
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w);
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
void glVertexAttribI4iv(GLuint index, const GLint* v);
void glVertexAttribI4uiv(GLuint index, const GLuint* v);
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params);

// Quality hint (SPEC §21.1.1, glHint). Non-binding; the frontend records the
// requested target/mode and forwards it to the backend at flush. Invalid target
// or mode -> GL_INVALID_ENUM.
void glHint(GLenum target, GLenum mode);

// Draw expansion (SPEC §10). Multi-draw, range-bounded indexed draw, and
// base-vertex indexed draw. Each flushes tracked state first; non-instanced
// variants require an active program; capability-gated per the feature table.
void glMultiDrawArrays(GLenum mode, const GLint* firsts, const GLint* counts,
                       GLsizei drawcount);
void glMultiDrawElements(GLenum mode, const GLint* counts, GLenum type,
                         const GLvoid* const* indices, GLsizei drawcount);
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                         GLenum type, const GLvoid* indices);
 void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
                              const GLvoid* indices, GLint basevertex);
 // Indirect draw (SPEC §10). Requires an indirect buffer bound to
 // GL_DRAW_INDIRECT_BUFFER and an active program; capability-gated by
 // IndirectDrawing. `indirect` is the byte offset into that bound buffer.
  void glDrawArraysIndirect(GLenum mode, const GLvoid* indirect);
  void glDrawElementsIndirect(GLenum mode, GLenum type, const GLvoid* indirect);

  void glDispatchCompute(GLuint x, GLuint y, GLuint z);
  void glDispatchComputeIndirect(const GLvoid* indirect);



} // namespace glcompat
