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
// Indexed string query (SPEC §22.2). Only GL_EXTENSIONS is indexable; this
// frontend exposes no extensions, so any index yields GL_INVALID_VALUE + nullptr;
// other names yield GL_INVALID_ENUM + nullptr.
const GLubyte* glGetStringi(GLenum name, GLuint index);

// Read one of the standard driver strings (GL_VENDOR, GL_RENDERER, GL_VERSION,
// GL_SHADING_LANGUAGE_VERSION) directly from the backing native/GLES context,
// bypassing the frontend's synthetic glGetString values. Returns the driver's
// null-terminated string, or nullptr when there is no current context, the name
// is not one of the four above, or the backend has no native context. C linkage
// for a stable ABI.
extern "C" const char* yagltGetBackingGlString(GLenum name);

void glGenBuffers(GLsizei n, GLuint* buffers);
void glCreateBuffers(GLsizei n, GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
GLboolean glIsBuffer(GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);

// Buffer sub-data / immutable storage / copy (SPEC §6).
void glBufferSubData(GLenum target, GLsizeiptr offset, GLsizeiptr size,
                     const GLvoid* data);
 void glBufferStorage(GLenum target, GLsizeiptr size, const GLvoid* data,
                      GLbitfield flags);
// DSA buffer allocation (SPEC §6.1/§6.2 glNamedBufferData / glNamedBufferSubData
// / glNamedBufferStorage). Operate on a named buffer by object name.
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data,
                       GLenum usage);
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size,
                          const GLvoid* data);
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const GLvoid* data,
                          GLbitfield flags);
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                         GLintptr readOffset, GLintptr writeOffset,
                         GLsizeiptr size);
// DSA buffer copy (SPEC §6). Copies a region between two named buffers.
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer,
                              GLintptr readOffset, GLintptr writeOffset,
                              GLsizeiptr size);

// Buffer parameter queries (SPEC §6 / §22). Reads frontend-owned buffer state.
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params);
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64* params);
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint* params);
// Mapped-buffer pointer queries (SPEC §6.1.1 glGetBufferPointerv /
// glGetNamedBufferPointerv). pname must be GL_BUFFER_MAP_POINTER.
void glGetBufferPointerv(GLenum target, GLenum pname, void** params);
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void** params);

// Internal format queries (SPEC §22.3). Forwards to the backend, which answers
// with implementation-specific format support.
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname,
                          GLsizei bufSize, GLint* params);
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname,
                              GLsizei bufSize, GLint64* params);

// Multisample sample-position query (SPEC §14.3.1). Returns the (x, y) location
// of the given sample; pname must be SAMPLE_POSITION.
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat* val);

// Buffer mapping (SPEC §6). Returns a pointer into the frontend data store, or
// nullptr on error. glUnmapBuffer returns GL_TRUE on success.
GLvoid* glMapBuffer(GLenum target, GLenum access);
GLvoid* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                        GLbitfield access);
GLboolean glUnmapBuffer(GLenum target);
GLvoid glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
// DSA buffer mapping (SPEC §6.1). Operate on a named buffer by object name.
GLvoid* glMapNamedBuffer(GLuint buffer, GLenum access);
GLvoid* glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length,
                             GLbitfield access);
GLboolean glUnmapNamedBuffer(GLuint buffer);
GLvoid glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset,
                                    GLsizeiptr length);

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
// Buffer data invalidation (SPEC §6.5). These take a buffer object name, not a
// binding target; GL has no target-based or "Named" spelling for them.
void glInvalidateBufferData(GLuint buffer);
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset,
                               GLsizeiptr length);

// Indexed buffer bindings (SPEC §6.1.1). Capability-guarded in the frontend:
// a target without indexed binding points yields GL_INVALID_ENUM and binding an
// unsupported target (e.g. SSBO on ES 3.0) yields GL_INVALID_OPERATION. The
// multi-bind forms (ARB_multi_bind) bind consecutive points and validate each
// entry separately; a null `buffers` array resets the range.
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer,
                       GLintptr offset, GLsizeiptr size);
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count,
                       const GLuint* buffers);
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count,
                        const GLuint* buffers, const GLintptr* offsets,
                        const GLsizeiptr* sizes);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glDeleteTextures(GLsizei n, const GLuint* textures);
GLboolean glIsTexture(GLuint texture);
// Selects the active texture image unit (SPEC §2.1). `texture` must be
// GL_TEXTURE0 + i within the supported unit range.
void glActiveTexture(GLenum texture);

// Direct State Access texture binding (SPEC §2.1, capability-gated by
// DirectStateAccess). glBindTextureUnit binds a texture to a specific unit
// without changing the active-texture selector; glBindTextures (SPEC §8.1)
// binds an array of textures to consecutive units, each to the target it was
// created with (a zero entry / null array resets the unit's targets).
void glBindTextureUnit(GLuint unit, GLuint texture);
void glBindTextures(GLuint first, GLsizei count, const GLuint* textures);

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
 // Copy texture sub-image from the framebuffer (SPEC §8.5 glCopyTexSubImage*D).
 void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x,
                         GLint y, GLsizei width);
 void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height);
 void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                         GLint zoffset, GLint x, GLint y, GLsizei width,
                         GLsizei height);

// Compressed texture image upload (SPEC §8.6 glCompressedTexImage*D).
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalFormat,
                           GLsizei width, GLint border, GLsizei imageSize,
                           const GLvoid* data);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalFormat,
                           GLsizei width, GLsizei height, GLint border,
                           GLsizei imageSize, const GLvoid* data);
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalFormat,
                           GLsizei width, GLsizei height, GLsizei depth, GLint border,
                           GLsizei imageSize, const GLvoid* data);
// Compressed texture sub-image upload (SPEC §8.6 glCompressedTexSubImage*D).
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                              GLsizei width, GLenum format, GLsizei imageSize,
                              const GLvoid* data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                              GLint yoffset, GLsizei width, GLsizei height,
                              GLenum format, GLsizei imageSize, const GLvoid* data);
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset,
                              GLint yoffset, GLint zoffset, GLsizei width,
                              GLsizei height, GLsizei depth, GLenum format,
                              GLsizei imageSize, const GLvoid* data);
// Texture parameter queries (SPEC §8.1). glGetTexParameteriv reads the bound
// texture for `target`; glGetTextureParameteriv is the DSA variant for an
// explicit texture object (capability-gated by DirectStateAccess).
void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params);
 void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
                    GLvoid* pixels);
  void glGetCompressedTexImage(GLenum target, GLint level, GLvoid* img);
  // Robustness (ARB_robustness / GL 4.5) bounds-checked read-back. `bufSize` is
  // the byte capacity of `pixels`.
  void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type,
                     GLsizei bufSize, GLvoid* pixels);
  void glGetnCompressedTexImage(GLenum target, GLint level, GLsizei bufSize,
                                GLvoid* img);
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
// Compressed DSA sub-image upload (SPEC §8.6 glCompressedTextureSubImage*D).
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset,
                                  GLsizei width, GLenum format, GLsizei imageSize,
                                  const GLvoid* data);
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset,
                                  GLint yoffset, GLsizei width, GLsizei height,
                                  GLenum format, GLsizei imageSize,
                                  const GLvoid* data);
 void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset,
                                   GLint yoffset, GLint zoffset, GLsizei width,
                                   GLsizei height, GLsizei depth, GLenum format,
                                   GLsizei imageSize, const GLvoid* data);
 // Copy DSA texture sub-image from the framebuffer (SPEC §8.5 glCopyTextureSubImage*D).
 void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x,
                             GLint y, GLsizei width);
 void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset,
                             GLint yoffset, GLint x, GLint y, GLsizei width,
                             GLsizei height);
 void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset,
                             GLint yoffset, GLint zoffset, GLint x, GLint y,
                             GLsizei width, GLsizei height);

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
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname,
                             GLint* params);
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname,
                             GLfloat* params);
 void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type,
                        GLvoid* pixels);
  void glGetCompressedTextureImage(GLuint texture, GLint level, GLvoid* img);
  // Robustness (ARB_robustness / GL 4.5) bounds-checked DSA read-back.
  void glGetnTextureImage(GLuint texture, GLint level, GLenum format, GLenum type,
                          GLsizei bufSize, GLvoid* pixels);
  void glGetnCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize,
                                    GLvoid* img);
 void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                           GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                           GLenum format, GLenum type, GLsizei bufSize, GLvoid* pixels);
 void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset,
                                     GLint yoffset, GLint zoffset, GLsizei width,
                                     GLsizei height, GLsizei depth, GLsizei bufSize,
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
GLboolean glIsRenderbuffer(GLuint renderbuffer);
void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width,
                         GLsizei height);
// Classic (non-DSA) multisample renderbuffer storage (SPEC §9.2.4). Operates on
// the renderbuffer bound to `target` (must be GL_RENDERBUFFER).
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                      GLenum internalFormat, GLsizei width,
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
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);


void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
GLboolean glIsFramebuffer(GLuint framebuffer);

// Framebuffer attachments (SPEC §2.1 / §9.2.1).
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum texTarget,
                           GLuint texture, GLint level);
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum texTarget,
                           GLuint texture, GLint level);
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum texTarget,
                           GLuint texture, GLint level, GLint layer);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum rbTarget,
                               GLuint renderbuffer);
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture,
                          GLint level);
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture,
                               GLint level, GLint layer);
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
 void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint* params);
 void glFramebufferParameteri(GLenum target, GLenum pname, GLint param);
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer,
                                                GLenum attachment, GLenum pname,
                                                GLint* params);
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment,
                                           GLenum pname, GLint* params);

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

// DSA draw/read-buffer selection (SPEC §9.3.1).
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf);
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
                                   const GLenum* bufs);
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum buf);

void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value);
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value);
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value);
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);

void glClipControl(GLenum origin, GLenum depth);


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

// Separate attribute format on the bound VAO (SPEC §10.3.2/§10.3.4,
// ARB_vertex_attrib_binding). Non-DSA spellings of the glVertexArray* calls
// above: the vertex array object is the one bound to GL_VERTEX_ARRAY_BINDING
// (none bound -> GL_INVALID_OPERATION).
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset,
                        GLsizei stride);
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint* buffers,
                         const GLintptr* offsets, const GLsizei* strides);
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type,
                          GLboolean normalized, GLuint relativeoffset);
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type,
                           GLuint relativeoffset);
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type,
                           GLuint relativeoffset);
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex);
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor);

// --- Transform feedback (SPEC §13.3) ---
GLuint glGenTransformFeedback();
void glGenTransformFeedbacks(GLsizei n, GLuint* names);
void glCreateTransformFeedbacks(GLsizei n, GLuint* names);
void glBindTransformFeedback(GLuint name);
void glDeleteTransformFeedback(GLuint name);
void glDeleteTransformFeedbacks(GLsizei n, const GLuint* names);
GLboolean glIsTransformFeedback(GLuint name);
void glBeginTransformFeedback(GLenum primitiveMode);
void glEndTransformFeedback();
void glPauseTransformFeedback();
 void glResumeTransformFeedback();
 void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer);
 void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer,
                                     GLintptr offset, GLsizeiptr size);
// Transform-feedback object state queries (SPEC §22.4). `xfb == 0` queries the
// default object; any other name must exist (else GL_INVALID_OPERATION). Legal
// pnames: TRANSFORM_FEEDBACK_ACTIVE/PAUSED for the scalar form,
// TRANSFORM_FEEDBACK_BUFFER_BINDING for `i_v`, and TRANSFORM_FEEDBACK_BUFFER_-
// START/SIZE for `i64_v`; `index` must be < the number of TF binding points.
 void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint* param);
 void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint* param);
 void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index,
                                  GLint64* param);

// --- Query objects (SPEC §4 / §19) ---
GLuint glGenQuery();
void glGenQueries(GLsizei n, GLuint* names);
void glCreateQueries(GLenum target, GLsizei n, GLuint* ids);
void glDeleteQuery(GLuint id);
void glDeleteQueries(GLsizei n, const GLuint* names);
GLboolean glIsQuery(GLuint id);
void glBeginQuery(GLenum target, GLuint id);
void glEndQuery(GLenum target);
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id);
void glEndQueryIndexed(GLenum target, GLuint index);
void glQueryCounter(GLuint id, GLenum target);
void glGetQueryiv(GLenum target, GLenum pname, GLint* params);
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint* params);
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params);
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params);
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params);
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params);
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);

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
void glCreateSamplers(GLsizei n, GLuint* samplers);
void glBindSampler(GLuint unit, GLuint sampler);
void glBindSamplers(GLuint first, GLsizei count, const GLuint* samplers);
void glBindImageTexture(GLuint unit, GLuint texture, GLint level,
                        GLboolean layered, GLint layer, GLenum access,
                        GLenum format);
void glBindImageTextures(GLuint first, GLsizei count, const GLuint* textures);
void glDeleteSampler(GLuint sampler);
void glDeleteSamplers(GLsizei n, const GLuint* samplers);
GLboolean glIsSampler(GLuint sampler);
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param);
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params);
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint* params);
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint* params);
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint* params);
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params);
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params);
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint* params);
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint* params);

// --- Shaders / programs (SPEC §8) ---
// glCreateShader / glCreateProgram return the new object name (0 on failure).
GLuint glCreateShader(GLenum stage);
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* strings,
                    const GLint* lengths);
void glShaderSource(GLuint shader, const std::string& source);
void glCompileShader(GLuint shader);
void glReleaseShaderCompiler(void);
void glSpecializeShader(GLuint shader, const GLchar* entryPoint,
                        GLuint numSpecializationConstants, const GLuint* pConstantIndex,
                        const GLuint* pConstantValue);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
void glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint* range,
                                GLint* precision);

// Convenience overload (C++ only, not exported by the C shim): returns the
// queried value directly for callers that prefer the return-value style.
GLint glGetShaderiv(GLuint shader, GLenum pname);
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length,
                       GLchar* infoLog);
void glDeleteShader(GLuint shader);
GLboolean glIsShader(GLuint shader);

GLuint glCreateProgram();
 void glAttachShader(GLuint program, GLuint shader);
 void glDetachShader(GLuint program, GLuint shader);
 void glLinkProgram(GLuint program);
 void glValidateProgram(GLuint program);
 void glProgramParameteri(GLuint program, GLenum pname, GLint value);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
// Convenience overload (C++ only, not exported by the C shim): returns the
// queried value directly for callers that prefer the return-value style.
GLint glGetProgramiv(GLuint program, GLenum pname);
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count,
                         GLuint* shaders);
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length,
                       GLchar* source);

// Object labels (SPEC §22.2). glObjectLabel / glGetObjectLabel address objects by
// namespace `identifier` (GL_BUFFER / GL_SHADER / GL_PROGRAM / GL_VERTEX_ARRAY /
// GL_QUERY / GL_PROGRAM_PIPELINE / GL_TRANSFORM_FEEDBACK / GL_SAMPLER / GL_TEXTURE /
// GL_RENDERBUFFER / GL_FRAMEBUFFER) and name; glObjectPtrLabel / glGetObjectPtrLabel
// address sync objects via their raw pointer.
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar* label);
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei* length,
                     GLchar* label);
void glObjectPtrLabel(const void* ptr, GLsizei length, const GLchar* label);
void glGetObjectPtrLabel(const void* ptr, GLsizei bufSize, GLsizei* length,
                        GLchar* label);


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
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname,
                             GLint* params);

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize,
                        GLsizei* length, GLint* size, GLenum* type, GLchar* name);
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize,
                       GLsizei* length, GLint* size, GLenum* type, GLchar* name);
GLuint glGetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName);
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex,
                              GLenum pname, GLint* params);
 void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize,
                              GLsizei* length, GLchar* uniformName);
 void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount,
                            const GLuint* uniformIndices, GLenum pname,
                            GLint* params);
  void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex,
                                    GLsizei bufSize, GLsizei* length,
                                    GLchar* uniformBlockName);
  void glGetUniformIndices(GLuint program, GLsizei uniformCount,
                          const GLchar* const* uniformNames,
                          GLuint* uniformIndices);
   void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex,
                              GLuint uniformBlockBinding);
   void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex,
                                    GLuint storageBlockBinding);


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
 void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname,
                          GLint* values);
 void glDeleteProgram(GLuint program);
GLboolean glIsProgram(GLuint program);
GLint glGetAttribLocation(GLuint program, const GLchar* name);
GLint glGetFragDataLocation(GLuint program, const GLchar* name);
GLint glGetFragDataIndex(GLuint program, const GLchar* name);
// Transform feedback varying reflection (SPEC §13.3.1). Returns through
// `length`/`size`/`type`/`name` the properties of the `index`-th captured varying
// of `program`. Out-of-range `index` or a backend without introspection yields
// GL_INVALID_VALUE; a non-program object yields GL_INVALID_OPERATION.
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize,
                                   GLsizei* length, GLsizei* size, GLenum* type,
                                   GLchar* name);
 void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name);

 // Bind a user-defined fragment shader output to a fragment color number (SPEC
 // §7.3.7 / §15.1.2 glBindFragDataLocation /
 // glBindFragDataLocationIndexed). The indexed form also sets the dual-source
 // index (0 or 1).
 void glBindFragDataLocation(GLuint program, GLuint colorNumber, const GLchar* name);
 void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber,
                                    GLuint index, const GLchar* name);

 // Specify the transform-feedback varyings captured for `program` (SPEC §13.3.1).
 void glTransformFeedbackVaryings(GLuint program, GLsizei count,
                                  const GLchar* const* varyings, GLenum bufferMode);


// --- Uniforms (SPEC §8) ---
// glGetUniformLocation returns -1 for an unknown/non-linked program. The setters
// operate on the currently active program (glUseProgram); a -1 location is a
// silent no-op, matching glUniform* semantics.
GLint glGetUniformLocation(GLuint program, const GLchar* name);

// --- Program uniform value queries (SPEC §7.9 glGetUniform{f,i,ui,d}v) ---
// Read back a uniform value from a successfully linked program. The program must
// be linked (GL_INVALID_OPERATION otherwise); a -1 location generates
// GL_INVALID_OPERATION; a null `params` generates GL_INVALID_VALUE.
void glGetUniformfv(GLuint program, GLint location, GLfloat* params);
void glGetUniformiv(GLuint program, GLint location, GLint* params);
void glGetUniformuiv(GLuint program, GLint location, GLuint* params);
void glGetUniformdv(GLuint program, GLint location, GLdouble* params);

// Robust (GL4.5 ARB_robustness) bounds-checked variants. `bufSize` is the maximum
// size in basic machine units of the `params` buffer; a negative bufSize
// generates GL_INVALID_VALUE.
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat* params);
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint* params);
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint* params);
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble* params);
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
 void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
 void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
 void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
 void glUniform2iv(GLint location, GLsizei count, const GLint* value);
 void glUniform3iv(GLint location, GLsizei count, const GLint* value);
 void glUniform4iv(GLint location, GLsizei count, const GLint* value);
 void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniform1d(GLint location, GLdouble v0);
 void glUniform2d(GLint location, GLdouble v0, GLdouble v1);
 void glUniform3d(GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
 void glUniform4d(GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
 void glUniform1dv(GLint location, GLsizei count, const GLdouble* value);
 void glUniform2dv(GLint location, GLsizei count, const GLdouble* value);
 void glUniform3dv(GLint location, GLsizei count, const GLdouble* value);
 void glUniform4dv(GLint location, GLsizei count, const GLdouble* value);
 void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
// Non-square matrix uniforms (SPEC §7.6 glUniformMatrix{2x3,3x2,2x4,4x2,3x4,4x3}{fd}v).
// The first number is the column count, the second the row count; `value` holds
// count * columns * rows components.
 void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat* value);
 void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose,
                         const GLdouble* value);
 void glUniform1ui(GLint location, GLuint v0);
 void glUniform2ui(GLint location, GLuint v0, GLuint v1);
 void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
 void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
 void glUniform1uiv(GLint location, GLsizei count, const GLuint* value);
 void glUniform2uiv(GLint location, GLsizei count, const GLuint* value);
 void glUniform3uiv(GLint location, GLsizei count, const GLuint* value);
 void glUniform4uiv(GLint location, GLsizei count, const GLuint* value);

// --- Program uniforms (SPEC §7.9, glProgramUniform*) ---
// Like glUniform* but target an explicit program; the program must be a
// successfully linked program object (GL_INVALID_OPERATION otherwise), and a
// -1 location is a silent no-op.
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0);
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1);
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glProgramUniform1i(GLuint program, GLint location, GLint v0);
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1);
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
 void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat* value);
 void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint* value);
 void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat* value);
 void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat* value);
 void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat* value);
 void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint* value);
 void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint* value);
 void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint* value);
 void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniform1d(GLuint program, GLint location, GLdouble v0);
 void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1);
 void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
 void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
 void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
 void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
 void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
 void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
 void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
// Non-square matrix program uniforms (SPEC §7.6
// glProgramUniformMatrix{2x3,3x2,2x4,4x2,3x4,4x3}{fd}v).
 void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
 void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
 void glProgramUniform1ui(GLuint program, GLint location, GLuint v0);
 void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1);
 void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
 void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
 void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint* value);
 void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint* value);
 void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint* value);
 void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint* value);

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
// Indexed (per-draw-buffer) blending (SPEC §15.3 / §17.3.4,
// ARB_draw_buffers_blend). `buf` selects the draw-buffer slot; buffer 0 is
// equivalent to the non-indexed glBlendFunc / glBlendEquation setters. `buf`
// >= MAX_DRAW_BUFFERS (8) reports GL_INVALID_VALUE; invalid blend factors /
// equations report GL_INVALID_ENUM.
void glBlendFunci(GLuint buf, GLenum src, GLenum dst);
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB,
                          GLenum srcAlpha, GLenum dstAlpha);
void glBlendEquationi(GLuint buf, GLenum mode);
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
void glUseProgram(GLuint prog);

// --- Program pipelines (SPEC §7.4) ---
// Build a single-stage separable program (glCreateShaderProgramv). Returns the
// new program name (query LINK_STATUS / INFO_LOG for success).
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar* const* strings);
void glGenProgramPipelines(GLsizei n, GLuint* pipelines);
void glCreateProgramPipelines(GLsizei n, GLuint* pipelines);
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
void glDepthRangeIndexed(GLuint index, GLdouble nearVal, GLdouble farVal);
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble* v);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

// Rasterization scalar state (SPEC §11). Recorded in the current context's
// GLStateTracker and pushed to the backend only when the value changes (SPEC
// §10). glPointSize / glLineWidth / glPolygonOffset map directly to GLES3.
void glPointSize(GLfloat size);
void glLineWidth(GLfloat width);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp);

// Point parameters (SPEC §10.2, glPointParameter*). pname validation and the
// non-negative / sprite-origin checks live in Context; the frontend owns the
// values and pushes them to the backend on the next state flush (SPEC §10).
void glPointParameteri(GLenum pname, GLint param);
void glPointParameterf(GLenum pname, GLfloat param);
void glPointParameteriv(GLenum pname, const GLint* params);
void glPointParameterfv(GLenum pname, const GLfloat* params);

// Patch parameters (SPEC §10.6, glPatchParameter{i,fv}). pname validation and
// the non-positive vertex-count check live in Context; the frontend owns the
// values and pushes them to the backend on the next state flush (SPEC §10).
void glPatchParameteri(GLenum pname, GLint value);
void glPatchParameterfv(GLenum pname, const GLfloat* values);

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
void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue,
                  GLboolean alpha);
void glSampleCoverage(GLfloat value, GLboolean invert);

// Primitive restart index (SPEC §10.4, glPrimitiveRestartIndex). Activation is via
// glEnable(GL_PRIMITIVE_RESTART); this records the restart index, pushed to the
// backend only when it changes (SPEC §10).
void glPrimitiveRestartIndex(GLuint index);

// Pixel store (SPEC §8.4). Affects subsequent texture/image uploads.
void glPixelStorei(GLenum pname, GLint param);
void glPixelStoref(GLenum pname, GLfloat param);

// Viewport (glViewport) and scissor box (glScissor), recorded in the current
// context's GLStateTracker and pushed to the backend on the next flush (SPEC §10).
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
void glViewportIndexedfv(GLuint index, const GLfloat* v);
void glScissorIndexed(GLuint index, GLint x, GLint y, GLsizei width, GLsizei height);
void glScissorIndexedv(GLuint index, const GLint* v);
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat* v);
void glScissorArrayv(GLuint first, GLsizei count, const GLint* v);

// Clear values + clear (SPEC §2.1). glClearColor/glClearDepth record the
// per-context clear values; glClear flushes tracked state then clears the bound
// framebuffer for the given mask. An invalid mask reports GL_INVALID_VALUE.
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClearDepth(GLdouble depth);
void glClearDepthf(GLfloat depth);
void glClearStencil(GLint s);
void glClear(GLuint mask);

// --- Texture clearing (SPEC §8.10) ---
// DSA clears of a named texture / sub-region. `data` is normally null.
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type,
                    const void* data);
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                       GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const void* data);

// Copy a texel sub-region between two image objects (textures/renderbuffers).
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel,
                       GLint srcX, GLint srcY, GLint srcZ, GLuint dstName,
                       GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY,
                       GLint dstZ, GLsizei srcWidth, GLsizei srcHeight,
                       GLsizei srcDepth);

// Whole-framebuffer buffer selection (SPEC §15 / §16). glDrawBuffers selects the
// draw buffers for the bound framebuffer; glReadBuffer selects its read buffer.
void glDrawBuffers(GLsizei n, const GLenum* bufs);
void glReadBuffer(GLenum buf);
// Single draw-buffer selection for the bound framebuffer (SPEC §9.3.1).
void glDrawBuffer(GLenum buf);

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

// Memory barriers (SPEC §7.13.2). glMemoryBarrier orders prior memory transactions
// (shader writes to images, SSBOs, atomic counters, buffer updates, …) so subsequent
// operations observe them; glMemoryBarrierByRegion is the framebuffer-region-scoped
// variant. `barriers` is a bitwise OR of the ALL_BARRIERS_BIT-derived flags. No
// validation is performed beyond a null-context guard.
void glMemoryBarrier(GLbitfield barriers);
void glMemoryBarrierByRegion(GLbitfield barriers);

// Read back pixels from the bound framebuffer (SPEC §2.1). Non-positive
// width/height reports GL_INVALID_VALUE.
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                  GLenum type, GLvoid* pixels);

// Robust pixel readback (SPEC §18 / ARB_robustness): like glReadPixels but with
// a byte-capacity guard `bufSize` on `pixels`. Non-positive width/height or a
// negative `bufSize` reports GL_INVALID_VALUE; an undersized buffer is truncated
// silently (no error), matching the robustness spec.
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                   GLenum type, GLsizei bufSize, GLvoid* pixels);

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
// 64-bit scalar query (SPEC §22.1): same frontend-owned state as glGetIntegerv
// widened to GLint64. Unknown pname -> GL_INVALID_ENUM; null params ->
// GL_INVALID_VALUE.
void glGetInteger64v(GLenum pname, GLint64* params);
// Indexed scalar queries (SPEC §22.1). Only GL_BLEND / GL_SCISSOR_TEST are
// supported (index < 16); other pnames -> GL_INVALID_ENUM, out-of-range index ->
// GL_INVALID_VALUE, null params -> GL_INVALID_VALUE.
void glGetIntegeri_v(GLenum pname, GLuint index, GLint* params);
void glGetBooleani_v(GLenum pname, GLuint index, GLboolean* params);
void glGetFloati_v(GLenum pname, GLuint index, GLfloat* params);
void glGetDoublei_v(GLenum pname, GLuint index, GLdouble* params);
void glGetInteger64i_v(GLenum pname, GLuint index, GLint64* params);
// Current graphics-reset status (SPEC §22.5). This frontend always reports
// GL_NO_ERROR.
GLenum glGetGraphicsResetStatus(void);
// Texture barrier (SPEC §10.9.2). Error-free; orders later texture reads after
// earlier draws that wrote the same texture within this context.
void glTextureBarrier(void);
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

// Base-instance draws (SPEC §10, ARB_base_instance / GL 4.2). Extend the
// instanced draws with `baseinstance` (per-instance attribute offset).
// Capability-gated by Feature::BaseInstance; an active program is required.
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count,
                                       GLsizei primcount, GLuint baseinstance);
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type,
                                         const GLvoid* indices, GLsizei primcount,
                                         GLuint baseinstance);
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count,
                                                   GLenum type, const GLvoid* indices,
                                                   GLsizei primcount, GLint basevertex,
                                                   GLuint baseinstance);

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
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble* params);
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble* params);
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params);
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params);
void glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** params);
void glGetVertexArrayiv(GLuint vao, GLenum pname, GLint* params);
void glGetVertexArrayIndexediv(GLuint vao, GLuint index, GLenum pname, GLint* params);
void glGetVertexArrayIndexed64iv(GLuint vao, GLuint index, GLenum pname, GLint64* params);

// Quality hint (SPEC §21.1.1, glHint). Non-binding; the frontend records the
// requested target/mode and forwards it to the backend at flush. Invalid target
// or mode -> GL_INVALID_ENUM.
void glHint(GLenum target, GLenum mode);

// Conditional rendering (SPEC §10.11, glBeginConditionalRender /
// glEndConditionalRender). Begins/ends a render region predicated on an existing
// query object. Capability-gated by ConditionalRendering; the frontend validates
// the query existence/type/activity and the predicate `mode` and forwards the
// region to the backend. Already-active region, non-query `id`, active query,
// disallowed query type, or invalid `mode` -> GL_INVALID_OPERATION/GL_INVALID_ENUM.
void glBeginConditionalRender(GLuint id, GLenum mode);
void glEndConditionalRender();

// Draw expansion (SPEC §10). Multi-draw, range-bounded indexed draw, and
// base-vertex indexed draw. Each flushes tracked state first; non-instanced
// variants require an active program; capability-gated per the feature table.
void glMultiDrawArrays(GLenum mode, const GLint* firsts, const GLint* counts,
                       GLsizei drawcount);
void glMultiDrawElements(GLenum mode, const GLint* counts, GLenum type,
                         const GLvoid* const* indices, GLsizei drawcount);
void glMultiDrawArraysBaseInstance(GLenum mode, const GLint* firsts,
                                  const GLsizei* counts,
                                  const GLsizei* instanceCounts,
                                  const GLuint* baseInstances, GLsizei drawcount);
void glMultiDrawElementsBaseInstance(GLenum mode, const GLsizei* counts,
                                    GLenum type, const GLvoid* const* indices,
                                    GLsizei drawcount,
                                    const GLuint* baseInstances);
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                         GLenum type, const GLvoid* indices);
 void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
                              const GLvoid* indices, GLint basevertex);
// Base-vertex draw variants (SPEC §10, GL 3.2 core / ARB_draw_elements_base_vertex).
// Capability-gated by Feature::DrawElementsBaseVertex; require an active program.
// `glDrawRangeElementsBaseVertex` rejects end < start with GL_INVALID_VALUE.
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type,
                                       const GLvoid* indices, GLsizei primcount,
                                       GLint basevertex);
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
                                   GLsizei count, GLenum type,
                                   const GLvoid* indices, GLint basevertex);
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei* counts, GLenum type,
                                   const GLvoid* const* indices, GLsizei drawcount,
                                   GLint basevertex);
 // Indirect draw (SPEC §10). Requires an indirect buffer bound to
 // GL_DRAW_INDIRECT_BUFFER and an active program; capability-gated by
 // IndirectDrawing. `indirect` is the byte offset into that bound buffer.
    void glDrawArraysIndirect(GLenum mode, const GLvoid* indirect);
    void glDrawElementsIndirect(GLenum mode, GLenum type, const GLvoid* indirect);

    // Multi-draw indirect (SPEC §10, ARB_multi_draw_indirect). Same preconditions as
    // the single indirect draws; issues `drawcount` indirect commands from the bound
    // GL_DRAW_INDIRECT_BUFFER at `indirect`, each `stride` bytes apart (stride 0 = tightly
    // packed). Capability-gated by IndirectDrawing.
    void glMultiDrawArraysIndirect(GLenum mode, const GLvoid* indirect,
                                  GLsizei drawcount, GLsizei stride);
    void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const GLvoid* indirect,
                                    GLsizei drawcount, GLsizei stride);


   // Transform-feedback draws (SPEC §13.3.3). Draw the captured vertex count of
   // the transform-feedback object `id` (stream variants draw a specific stream).
   void glDrawTransformFeedback(GLenum mode, GLuint id);
   void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei primcount);
   void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream);
   void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream,
                                              GLsizei primcount);

   void glDispatchCompute(GLuint x, GLuint y, GLuint z);
   void glDispatchComputeIndirect(const GLvoid* indirect);

// Debug messaging (SPEC §20.4) and debug groups (SPEC §20.5, KHR_debug).
// glDebugMessageCallback installs the delivery callback; glDebugMessageControl
// filters the (source, type, severity) space; glDebugMessageInsert generates an
// application message; glGetDebugMessageLog drains the retrievable log; the
// push/pop group pair delimits nested debug scopes.
void glDebugMessageCallback(GLDEBUGPROC callback, const void* userParam);
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity,
                           GLsizei count, const GLuint* ids, GLboolean enabled);
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity,
                          GLsizei length, const GLchar* buf);
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum* sources,
                            GLenum* types, GLuint* ids, GLenum* severities,
                            GLsizei* lengths, GLchar* messageLog);
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar* message);
void glPopDebugGroup();



} // namespace glcompat
