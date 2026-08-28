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

const GLubyte* glGetString(GLenum name) {
    if (g_current == nullptr) return nullptr;
    return g_current->getString(name);
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

void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    if (g_current == nullptr) return;
    g_current->bufferData(target, size, usage, data);
}

void glBufferSubData(GLenum target, GLsizeiptr offset, GLsizeiptr size,
                     const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->bufferSubData(target, offset, size, data);
}

void glBufferStorage(GLenum target, GLsizeiptr size, const GLvoid* data,
                     GLbitfield flags) {
    if (g_current == nullptr) return;
    g_current->bufferStorage(target, size, data, flags);
}

void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                         GLintptr readOffset, GLintptr writeOffset,
                         GLsizeiptr size) {
    if (g_current == nullptr) return;
    g_current->copyBufferSubData(readTarget, writeTarget, readOffset,
                                 writeOffset, size);
}

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getBufferParameteriv(target, pname, params);
}

GLvoid* glMapBuffer(GLenum target, GLenum access) {
    if (g_current == nullptr) return nullptr;
    return g_current->mapBuffer(target, access);
}

GLvoid* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                         GLbitfield access) {
    if (g_current == nullptr) return nullptr;
    return g_current->mapBufferRange(target, offset, length, access);
}

GLboolean glUnmapBuffer(GLenum target) {
    if (g_current == nullptr) return GL_FALSE;
    return g_current->unmapBuffer(target) ? GL_TRUE : GL_FALSE;
}

void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                       GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->getBufferSubData(target, offset, size, data);
}

void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size,
                            GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->getNamedBufferSubData(buffer, offset, size, data);
}

void glClearBufferData(GLenum target, GLenum internalformat, GLenum format,
                      GLenum type, const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->clearBufferData(target, internalformat, format, type, data);
}

void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format,
                           GLenum type, const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->clearNamedBufferData(buffer, internalformat, format, type, data);
}

void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset,
                         GLsizeiptr size, GLenum format, GLenum type,
                         const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->clearBufferSubData(target, internalformat, offset, size, format,
                                 type, data);
}

void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat,
                             GLintptr offset, GLsizeiptr size, GLenum format,
                             GLenum type, const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->clearNamedBufferSubData(buffer, internalformat, offset, size,
                                     format, type, data);
}

void glInvalidateBufferData(GLenum target) {
    if (g_current == nullptr) return;
    g_current->invalidateBufferData(target);
}

void glInvalidateBufferSubData(GLenum target, GLintptr offset, GLsizeiptr length) {
    if (g_current == nullptr) return;
    g_current->invalidateBufferSubData(target, offset, length);
}

void glInvalidateNamedBufferData(GLuint buffer) {
    if (g_current == nullptr) return;
    g_current->invalidateNamedBufferData(buffer);
}

void glInvalidateNamedBufferSubData(GLuint buffer, GLintptr offset,
                                   GLsizeiptr length) {
    if (g_current == nullptr) return;
    g_current->invalidateNamedBufferSubData(buffer, offset, length);
}

void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    if (g_current == nullptr) return;
    g_current->bindBufferBase(target, index, buffer);
}

void glBindBufferRange(GLenum target, GLuint index, GLuint buffer,
                       GLintptr offset, GLsizeiptr size) {
    if (g_current == nullptr) return;
    g_current->bindBufferRange(target, index, buffer, offset, size);
}

void glGenTextures(GLsizei n, GLuint* textures) {
    if (g_current == nullptr) return;
    g_current->genTextures(static_cast<uint32_t>(n), textures);
}

void glBindTexture(GLenum target, GLuint texture) {
    if (g_current == nullptr) return;
    g_current->bindTexture(target, texture);
}

void glActiveTexture(GLenum texture) {
    if (g_current == nullptr) return;
    g_current->activeTexture(texture);
}

void glBindTextureUnit(GLuint unit, GLuint texture) {
    if (g_current == nullptr) return;
    g_current->bindTextureUnit(static_cast<uint32_t>(unit), texture);
}

void glBindTextures(GLuint first, GLsizei count, GLenum target,
                    const GLuint* textures) {
    if (g_current == nullptr) return;
    g_current->bindTextures(static_cast<uint32_t>(first),
                            static_cast<uint32_t>(count), target, textures);
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    if (g_current == nullptr) return;
    g_current->deleteTextures(static_cast<uint32_t>(n), textures);
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                 GLsizei height, GLint, GLenum format, GLenum type,
                 const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->texImage2D(target, level, static_cast<uint32_t>(internalFormat),
                          static_cast<int>(width), static_cast<int>(height),
                          static_cast<uint32_t>(format),
                          static_cast<uint32_t>(type), data);
}

void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                  GLenum format, GLenum type, const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->texImage1D(target, level, static_cast<uint32_t>(internalFormat),
                          static_cast<int>(width), static_cast<uint32_t>(format),
                          static_cast<uint32_t>(type), data);
}

void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                  GLsizei height, GLsizei depth, GLenum format, GLenum type,
                  const GLvoid* data) {
    if (g_current == nullptr) return;
    g_current->texImage3D(target, level, static_cast<uint32_t>(internalFormat),
                          static_cast<int>(width), static_cast<int>(height),
                          static_cast<int>(depth), static_cast<uint32_t>(format),
                          static_cast<uint32_t>(type), data);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    if (g_current == nullptr) return;
    g_current->texParameteri(target, pname, static_cast<int>(param));
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    if (g_current == nullptr) return;
    g_current->texParameterf(target, pname, param);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params,
                      GLsizei count) {
    if (g_current == nullptr) return;
    g_current->texParameterfv(target, pname, params, static_cast<int>(count));
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint* params,
                      GLsizei count) {
    if (g_current == nullptr) return;
    g_current->texParameteriv(target, pname, params, static_cast<int>(count));
}

void glTexParameterIiv(GLenum target, GLenum pname, const GLint* params) {
    if (g_current == nullptr) return;
    g_current->texParameterIiv(target, pname, params);
}

void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint* params) {
    if (g_current == nullptr) return;
    g_current->texParameterIuiv(target, pname, params);
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    if (g_current == nullptr) return;
    g_current->getTexParameterfv(target, pname, params);
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width,
                     GLenum format, GLenum type, const GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->texSubImage1D(target, level, xoffset, static_cast<int>(width),
                             static_cast<uint32_t>(format),
                             static_cast<uint32_t>(type), pixels);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type,
                     const GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->texSubImage2D(target, level, xoffset, yoffset,
                             static_cast<int>(width), static_cast<int>(height),
                             static_cast<uint32_t>(format),
                             static_cast<uint32_t>(type), pixels);
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type, const GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->texSubImage3D(target, level, xoffset, yoffset, zoffset,
                             static_cast<int>(width), static_cast<int>(height),
                             static_cast<int>(depth),
                             static_cast<uint32_t>(format),
                             static_cast<uint32_t>(type), pixels);
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x,
                      GLint y, GLsizei width, GLint border) {
    if (g_current == nullptr) return;
    g_current->copyTexImage1D(target, level,
                              static_cast<uint32_t>(internalFormat), x, y,
                              static_cast<int>(width), border);
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x,
                      GLint y, GLsizei width, GLsizei height, GLint border) {
    if (g_current == nullptr) return;
    g_current->copyTexImage2D(target, level,
                              static_cast<uint32_t>(internalFormat), x, y,
                              static_cast<int>(width), static_cast<int>(height),
                              border);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getTexParameteriv(target, pname, params);
}

void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getTexParameterIiv(target, pname, params);
}

void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params) {
    if (g_current == nullptr) return;
    g_current->getTexParameterIuiv(target, pname, params);
}

void glGenerateMipmap(GLenum target) {
    if (g_current == nullptr) return;
    g_current->generateMipmap(target);
}

void glInvalidateTexImage(GLenum target, GLint level) {
    if (g_current == nullptr) return;
    g_current->invalidateTexImage(target, level);
}

void glInvalidateTexSubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                            GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) {
    if (g_current == nullptr) return;
    g_current->invalidateTexSubImage(target, level, xoffset, yoffset, zoffset,
                                   static_cast<int>(width), static_cast<int>(height),
                                   static_cast<int>(depth));
}

void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getTextureParameteriv(texture, pname, params);
}

void glCreateTextures(GLenum target, GLsizei n, GLuint* textures) {
    if (g_current == nullptr) return;
    g_current->createTextures(target, static_cast<uint32_t>(n), textures);
}

void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalFormat,
                        GLsizei width) {
    if (g_current == nullptr) return;
    g_current->textureStorage1D(texture, static_cast<int>(levels),
                               static_cast<uint32_t>(internalFormat),
                               static_cast<int>(width));
}

void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalFormat,
                        GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->textureStorage2D(texture, static_cast<int>(levels),
                               static_cast<uint32_t>(internalFormat),
                               static_cast<int>(width), static_cast<int>(height));
}

void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalFormat,
                        GLsizei width, GLsizei height, GLsizei depth) {
    if (g_current == nullptr) return;
    g_current->textureStorage3D(texture, static_cast<int>(levels),
                               static_cast<uint32_t>(internalFormat),
                               static_cast<int>(width), static_cast<int>(height),
                               static_cast<int>(depth));
}

void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width,
                         GLenum format, GLenum type, const GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->textureSubImage1D(texture, level, xoffset, static_cast<int>(width),
                                 static_cast<uint32_t>(format),
                                 static_cast<uint32_t>(type), pixels);
}

void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                         GLsizei width, GLsizei height, GLenum format, GLenum type,
                         const GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->textureSubImage2D(texture, level, xoffset, yoffset,
                                 static_cast<int>(width), static_cast<int>(height),
                                 static_cast<uint32_t>(format),
                                 static_cast<uint32_t>(type), pixels);
}

void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                         GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, const GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->textureSubImage3D(texture, level, xoffset, yoffset, zoffset,
                                 static_cast<int>(width), static_cast<int>(height),
                                 static_cast<int>(depth),
                                 static_cast<uint32_t>(format),
                                 static_cast<uint32_t>(type), pixels);
}

void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {
    if (g_current == nullptr) return;
    g_current->textureParameteri(texture, pname, static_cast<int>(param));
}

void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {
    if (g_current == nullptr) return;
    g_current->textureParameterf(texture, pname, param);
}

void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params,
                          GLsizei count) {
    if (g_current == nullptr) return;
    g_current->textureParameterfv(texture, pname, params, static_cast<int>(count));
}

void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params,
                          GLsizei count) {
    if (g_current == nullptr) return;
    g_current->textureParameteriv(texture, pname, params, static_cast<int>(count));
}

void glGenerateTextureMipmap(GLuint texture) {
    if (g_current == nullptr) return;
    g_current->generateTextureMipmap(texture);
}

void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint* params) {
    if (g_current == nullptr) return;
    g_current->textureParameterIiv(texture, pname, params);
}

void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint* params) {
    if (g_current == nullptr) return;
    g_current->textureParameterIuiv(texture, pname, params);
}

void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getTextureParameterIiv(texture, pname, params);
}

void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params) {
    if (g_current == nullptr) return;
    g_current->getTextureParameterIuiv(texture, pname, params);
}

void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params) {
    if (g_current == nullptr) return;
    g_current->getTextureParameterfv(texture, pname, params);
}

void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname,
                                  GLint* params) {
    if (g_current == nullptr) return;
    g_current->getTextureLevelParameteriv(texture, level, pname, params);
}

void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname,
                                  GLfloat* params) {
    if (g_current == nullptr) return;
    g_current->getTextureLevelParameterfv(texture, level, pname, params);
}

void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type,
                       GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->getTextureImage(texture, level, static_cast<uint32_t>(format),
                               static_cast<uint32_t>(type), pixels);
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
                   GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->getTexImage(target, level, static_cast<uint32_t>(format),
                           static_cast<uint32_t>(type), pixels);
}

void glTextureBuffer(GLuint texture, GLenum internalFormat, GLuint buffer) {
    if (g_current == nullptr) return;
    g_current->textureBuffer(texture, static_cast<uint32_t>(internalFormat),
                             buffer);
}

void glTextureBufferRange(GLuint texture, GLenum internalFormat, GLuint buffer,
                           GLintptr offset, GLsizeiptr size) {
    if (g_current == nullptr) return;
    g_current->textureBufferRange(texture, static_cast<uint32_t>(internalFormat),
                                  buffer, offset, size);
}

void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat,
                    GLsizei width) {
    if (g_current == nullptr) return;
    g_current->texStorage1D(target, static_cast<int>(levels),
                            static_cast<uint32_t>(internalFormat),
                            static_cast<int>(width));
}

void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat,
                    GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->texStorage2D(target, static_cast<int>(levels),
                            static_cast<uint32_t>(internalFormat),
                            static_cast<int>(width), static_cast<int>(height));
}

void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat,
                    GLsizei width, GLsizei height, GLsizei depth) {
    if (g_current == nullptr) return;
    g_current->texStorage3D(target, static_cast<int>(levels),
                            static_cast<uint32_t>(internalFormat),
                            static_cast<int>(width), static_cast<int>(height),
                            static_cast<int>(depth));
}

void glTexBuffer(GLenum target, GLenum internalFormat, GLuint buffer) {
    if (g_current == nullptr) return;
    g_current->texBuffer(target, static_cast<uint32_t>(internalFormat), buffer);
}

void glTexBufferRange(GLenum target, GLenum internalFormat, GLuint buffer,
                      GLintptr offset, GLsizeiptr size) {
    if (g_current == nullptr) return;
    g_current->texBufferRange(target, static_cast<uint32_t>(internalFormat), buffer,
                              offset, size);
}

void glTexStorage2DMultisample(GLenum target, GLsizei samples,
                               GLenum internalFormat, GLsizei width, GLsizei height,
                               GLboolean fixedsamplelocations) {
    if (g_current == nullptr) return;
    g_current->texStorage2DMultisample(target, static_cast<int>(samples),
                                       static_cast<uint32_t>(internalFormat),
                                       static_cast<int>(width),
                                       static_cast<int>(height),
                                       fixedsamplelocations != 0);
}

void glTexStorage3DMultisample(GLenum target, GLsizei samples,
                               GLenum internalFormat, GLsizei width, GLsizei height,
                               GLsizei depth, GLboolean fixedsamplelocations) {
    if (g_current == nullptr) return;
    g_current->texStorage3DMultisample(target, static_cast<int>(samples),
                                       static_cast<uint32_t>(internalFormat),
                                       static_cast<int>(width),
                                       static_cast<int>(height),
                                       static_cast<int>(depth),
                                       fixedsamplelocations != 0);
}

void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                             GLsizei width, GLsizei height,
                             GLboolean fixedsamplelocations) {
    if (g_current == nullptr) return;
    g_current->texImage2DMultisample(target, static_cast<int>(samples),
                                     static_cast<uint32_t>(internalFormat),
                                     static_cast<int>(width),
                                     static_cast<int>(height),
                                     fixedsamplelocations != 0);
}

void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                             GLsizei width, GLsizei height, GLsizei depth,
                             GLboolean fixedsamplelocations) {
    if (g_current == nullptr) return;
    g_current->texImage3DMultisample(target, static_cast<int>(samples),
                                     static_cast<uint32_t>(internalFormat),
                                     static_cast<int>(width),
                                     static_cast<int>(height),
                                     static_cast<int>(depth),
                                     fixedsamplelocations != 0);
}

void glTextureStorage2DMultisample(GLuint texture, GLsizei samples,
                                   GLenum internalFormat, GLsizei width,
                                   GLsizei height, GLboolean fixedsamplelocations) {
    if (g_current == nullptr) return;
    g_current->textureStorage2DMultisample(texture, static_cast<int>(samples),
                                           static_cast<uint32_t>(internalFormat),
                                           static_cast<int>(width),
                                           static_cast<int>(height),
                                           fixedsamplelocations != 0);
}

void glTextureStorage3DMultisample(GLuint texture, GLsizei samples,
                                   GLenum internalFormat, GLsizei width,
                                   GLsizei height, GLsizei depth,
                                   GLboolean fixedsamplelocations) {
    if (g_current == nullptr) return;
    g_current->textureStorage3DMultisample(texture, static_cast<int>(samples),
                                           static_cast<uint32_t>(internalFormat),
                                           static_cast<int>(width),
                                           static_cast<int>(height),
                                           static_cast<int>(depth),
                                           fixedsamplelocations != 0);
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

void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width,
                         GLsizei height) {
    if (g_current == nullptr) return;
    g_current->renderbufferStorage(target, static_cast<uint32_t>(internalFormat),
                                  static_cast<int>(width),
                                  static_cast<int>(height));
}

// Direct State Access renderbuffer surface (SPEC §8.2 / §9.2).
void glCreateRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    if (g_current == nullptr) return;
    g_current->createRenderbuffers(static_cast<uint32_t>(n), renderbuffers);
}
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalFormat,
                               GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->namedRenderbufferStorage(renderbuffer,
                                       static_cast<uint32_t>(internalFormat),
                                       static_cast<int>(width),
                                       static_cast<int>(height));
}
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples,
                                          GLenum internalFormat, GLsizei width,
                                          GLsizei height) {
    if (g_current == nullptr) return;
    g_current->namedRenderbufferStorageMultisample(
        renderbuffer, static_cast<int>(samples),
        static_cast<uint32_t>(internalFormat), static_cast<int>(width),
        static_cast<int>(height));
}
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname,
                                      GLint* params) {
    if (g_current == nullptr) return;
    g_current->getNamedRenderbufferParameteriv(renderbuffer, pname, params);
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

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum texTarget,
                           GLuint texture, GLint level) {
    if (g_current == nullptr) return;
    g_current->framebufferTexture2D(target, attachment, texTarget, texture, level);
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum rbTarget,
                               GLuint renderbuffer) {
    if (g_current == nullptr) return;
    g_current->framebufferRenderbuffer(target, attachment, rbTarget, renderbuffer);
}

GLenum glCheckFramebufferStatus(GLenum target) {
    if (g_current == nullptr) return GL_FRAMEBUFFER_COMPLETE;
    return g_current->checkFramebufferStatus(target);
}

// Direct State Access framebuffer surface (SPEC §9.2).
void glCreateFramebuffers(GLsizei n, GLuint* framebuffers) {
    if (g_current == nullptr) return;
    g_current->createFramebuffers(static_cast<uint32_t>(n), framebuffers);
}
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment,
                                   GLenum renderbufferTarget, GLuint renderbuffer) {
    if (g_current == nullptr) return;
    g_current->namedFramebufferRenderbuffer(framebuffer, attachment,
                                           renderbufferTarget, renderbuffer);
}
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment,
                              GLuint texture, GLint level) {
    if (g_current == nullptr) return;
    g_current->namedFramebufferTexture(framebuffer, attachment, texture, level);
}
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment,
                                   GLuint texture, GLint level, GLint layer) {
    if (g_current == nullptr) return;
    g_current->namedFramebufferTextureLayer(framebuffer, attachment, texture, level,
                                           layer);
}
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
    if (g_current == nullptr) return GL_FRAMEBUFFER_COMPLETE;
    return g_current->checkNamedFramebufferStatus(framebuffer, target);
}
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {
    if (g_current == nullptr) return;
    g_current->namedFramebufferParameteri(framebuffer, pname, static_cast<int>(param));
}
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname,
                                     GLint* params) {
    if (g_current == nullptr) return;
    g_current->getNamedFramebufferParameteriv(framebuffer, pname, params);
}
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer,
                                               GLenum attachment, GLenum pname,
                                               GLint* params) {
    if (g_current == nullptr) return;
    g_current->getNamedFramebufferAttachmentParameteriv(framebuffer, attachment,
                                                       pname, params);
}
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter) {
    if (g_current == nullptr) return;
    g_current->blitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0,
                                   srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                                   static_cast<uint32_t>(mask),
                                   static_cast<uint32_t>(filter));
}
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments,
                                     const GLenum* attachments) {
    if (g_current == nullptr) return;
    g_current->invalidateNamedFramebufferData(framebuffer,
                                             static_cast<int32_t>(numAttachments),
                                             reinterpret_cast<const uint32_t*>(attachments));
}
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments,
                                        const GLenum* attachments, GLint x, GLint y,
                                        GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->invalidateNamedFramebufferSubData(
        framebuffer, static_cast<int32_t>(numAttachments),
        reinterpret_cast<const uint32_t*>(attachments), x, y,
        static_cast<int32_t>(width), static_cast<int32_t>(height));
}
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                              const GLint* value) {
    if (g_current == nullptr) return;
    g_current->clearNamedFramebufferiv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                               const GLuint* value) {
    if (g_current == nullptr) return;
    g_current->clearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                              const GLfloat* value) {
    if (g_current == nullptr) return;
    g_current->clearNamedFramebufferfv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer,
                              GLfloat depth, GLint stencil) {
    if (g_current == nullptr) return;
    g_current->clearNamedFramebufferfi(framebuffer, buffer, drawbuffer, depth,
                                      stencil);
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

GLboolean glIsVertexArray(GLuint array) {
    if (g_current == nullptr) return GL_FALSE;
    return g_current->isVertexArray(array) ? GL_TRUE : GL_FALSE;
}

// Direct State Access vertex-array surface (SPEC §10.3.1).
void glCreateVertexArrays(GLsizei n, GLuint* arrays) {
    if (g_current == nullptr) return;
    g_current->createVertexArrays(static_cast<uint32_t>(n), arrays);
}
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {
    if (g_current == nullptr) return;
    g_current->vertexArrayElementBuffer(vaobj, buffer);
}
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    if (g_current == nullptr) return;
    g_current->enableVertexArrayAttrib(vaobj, index);
}
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    if (g_current == nullptr) return;
    g_current->disableVertexArrayAttrib(vaobj, index);
}
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer,
                               GLintptr offset, GLsizei stride) {
    if (g_current == nullptr) return;
    g_current->vertexArrayVertexBuffer(vaobj, bindingindex, buffer,
                                       static_cast<intptr_t>(offset),
                                       static_cast<int32_t>(stride));
}
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count,
                                const GLuint* buffers, const GLintptr* offsets,
                                const GLsizei* strides) {
    if (g_current == nullptr) return;
    g_current->vertexArrayVertexBuffers(
        vaobj, first, static_cast<uint32_t>(count),
        reinterpret_cast<const GLObjectName*>(buffers),
        reinterpret_cast<const intptr_t*>(offsets),
        reinterpret_cast<const int32_t*>(strides));
}
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size,
                               GLenum type, GLboolean normalized,
                               GLuint relativeoffset) {
    if (g_current == nullptr) return;
    g_current->vertexArrayAttribFormat(vaobj, attribindex,
                                       static_cast<int32_t>(size), type,
                                       normalized != 0, relativeoffset);
}
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size,
                                GLenum type, GLuint relativeoffset) {
    if (g_current == nullptr) return;
    g_current->vertexArrayAttribIFormat(vaobj, attribindex,
                                        static_cast<int32_t>(size), type,
                                        relativeoffset);
}
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size,
                                GLenum type, GLuint relativeoffset) {
    if (g_current == nullptr) return;
    g_current->vertexArrayAttribLFormat(vaobj, attribindex,
                                        static_cast<int32_t>(size), type,
                                        relativeoffset);
}
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex,
                                GLuint bindingindex) {
    if (g_current == nullptr) return;
    g_current->vertexArrayAttribBinding(vaobj, attribindex, bindingindex);
}
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex,
                                 GLuint divisor) {
    if (g_current == nullptr) return;
    g_current->vertexArrayBindingDivisor(vaobj, bindingindex, divisor);
}

// --- Transform feedback (SPEC §13.3) ---

GLuint glGenTransformFeedback() {
    if (g_current == nullptr) return 0;
    return g_current->genTransformFeedback();
}

void glGenTransformFeedbacks(GLsizei n, GLuint* names) {
    if (g_current == nullptr) return;
    g_current->genTransformFeedbacks(static_cast<uint32_t>(n), names);
}

void glBindTransformFeedback(GLuint name) {
    if (g_current == nullptr) return;
    g_current->bindTransformFeedback(name);
}

void glDeleteTransformFeedback(GLuint name) {
    if (g_current == nullptr) return;
    g_current->deleteTransformFeedback(name);
}

void glDeleteTransformFeedbacks(GLsizei n, const GLuint* names) {
    if (g_current == nullptr) return;
    g_current->deleteTransformFeedbacks(static_cast<uint32_t>(n), names);
}

void glBeginTransformFeedback(GLenum primitiveMode) {
    if (g_current == nullptr) return;
    g_current->beginTransformFeedback(primitiveMode);
}

void glEndTransformFeedback() {
    if (g_current == nullptr) return;
    g_current->endTransformFeedback();
}

void glPauseTransformFeedback() {
    if (g_current == nullptr) return;
    g_current->pauseTransformFeedback();
}

void glResumeTransformFeedback() {
    if (g_current == nullptr) return;
    g_current->resumeTransformFeedback();
}

// --- Query objects (SPEC §4 / §19) ---

GLuint glGenQuery() {
    if (g_current == nullptr) return 0;
    return g_current->genQuery();
}

void glGenQueries(GLsizei n, GLuint* names) {
    if (g_current == nullptr || n < 0) return;
    g_current->genQueries(static_cast<uint32_t>(n), names);
}

void glDeleteQuery(GLuint id) {
    if (g_current == nullptr) return;
    g_current->deleteQuery(id);
}

void glDeleteQueries(GLsizei n, const GLuint* names) {
    if (g_current == nullptr || n < 0) return;
    g_current->deleteQueries(static_cast<uint32_t>(n), names);
}

GLboolean glIsQuery(GLuint id) {
    if (g_current == nullptr) return GL_FALSE;
    return g_current->isQuery(id) ? GL_TRUE : GL_FALSE;
}

void glBeginQuery(GLenum target, GLuint id) {
    if (g_current == nullptr) return;
    g_current->beginQuery(target, id);
}

void glEndQuery(GLenum target) {
    if (g_current == nullptr) return;
    g_current->endQuery(target);
}

void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) {
    if (g_current == nullptr) return;
    g_current->beginQueryIndexed(target, index, id);
}

void glEndQueryIndexed(GLenum target, GLuint index) {
    if (g_current == nullptr) return;
    g_current->endQueryIndexed(target, index);
}

void glGetQueryiv(GLenum target, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getQueryiv(target, pname, params);
}

void glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getQueryObjectiv(id, pname, params);
}

void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {
    if (g_current == nullptr) return;
    g_current->getQueryObjectuiv(id, pname, params);
}

void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    if (g_current == nullptr) return;
    g_current->getQueryObjecti64v(id, pname, params);
}

void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    if (g_current == nullptr) return;
    g_current->getQueryObjectui64v(id, pname, params);
}

// --- Sync objects (SPEC §4 / §20, ARB_sync) ---

GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    if (g_current == nullptr) return nullptr;
    return g_current->fenceSync(condition, flags);
}

GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    if (g_current == nullptr) return GL_WAIT_FAILED;
    return g_current->clientWaitSync(sync, flags, timeout);
}

void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    if (g_current == nullptr) return;
    g_current->waitSync(sync, flags, timeout);
}

void glDeleteSync(GLsync sync) {
    if (g_current == nullptr) return;
    g_current->deleteSync(sync);
}

GLboolean glIsSync(GLsync sync) {
    if (g_current == nullptr) return GL_FALSE;
    return g_current->isSync(sync) ? GL_TRUE : GL_FALSE;
}

void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length,
                 GLint* values) {
    if (g_current == nullptr) return;
    g_current->getSynciv(sync, pname, static_cast<uint32_t>(bufSize),
                         length, values);
}

// --- Sampler objects (SPEC §8.2) ---

GLuint glGenSampler() {
    if (g_current == nullptr) return 0;
    return g_current->genSampler();
}

void glGenSamplers(GLsizei n, GLuint* samplers) {
    if (g_current == nullptr) return;
    g_current->genSamplers(static_cast<uint32_t>(n), samplers);
}

void glBindSampler(GLuint unit, GLuint sampler) {
    if (g_current == nullptr) return;
    g_current->bindSampler(unit, sampler);
}

void glDeleteSampler(GLuint sampler) {
    if (g_current == nullptr) return;
    g_current->deleteSampler(sampler);
}

void glDeleteSamplers(GLsizei n, const GLuint* samplers) {
    if (g_current == nullptr) return;
    g_current->deleteSamplers(static_cast<uint32_t>(n), samplers);
}

GLboolean glIsSampler(GLuint sampler) {
    if (g_current == nullptr) return 0;
    return g_current->isSampler(sampler) ? 1 : 0;
}

void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    if (g_current == nullptr) return;
    g_current->samplerParameteri(sampler, pname, static_cast<int>(param));
}

void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getSamplerParameteriv(sampler, pname, params);
}

// --- Shaders / programs (SPEC §8) ---

GLuint glCreateShader(GLenum stage) {
    if (g_current == nullptr) return 0;
    return g_current->createShader(stage);
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* strings,
                    const GLint* lengths) {
    if (g_current == nullptr) return;
    if (count <= 0 || strings == nullptr) {
        g_current->shaderSource(shader, std::string());
        return;
    }
    // Join the provided string slices into one source (honouring lengths when
    // given, treating -1 as null-terminated).
    std::string src;
    for (GLsizei i = 0; i < count; ++i) {
        if (strings[i] == nullptr) continue;
        if (lengths != nullptr && lengths[i] >= 0) {
            src.append(strings[i], static_cast<size_t>(lengths[i]));
        } else {
            src.append(strings[i]);
        }
    }
    g_current->shaderSource(shader, src);
}

void glShaderSource(GLuint shader, const std::string& source) {
    if (g_current == nullptr) return;
    g_current->shaderSource(shader, source);
}

void glCompileShader(GLuint shader) {
    if (g_current == nullptr) return;
    g_current->compileShader(shader);
}

GLint glGetShaderiv(GLuint shader, GLenum pname) {
    if (g_current == nullptr) return 0;
    return g_current->getShaderiv(shader, pname);
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length,
                       GLchar* infoLog) {
    if (g_current == nullptr) return;
    g_current->getShaderInfoLog(shader, static_cast<uint32_t>(bufSize), length,
                               infoLog);
}

void glDeleteShader(GLuint shader) {
    if (g_current == nullptr) return;
    g_current->deleteShader(shader);
}

GLuint glCreateProgram() {
    if (g_current == nullptr) return 0;
    return g_current->createProgram();
}

void glAttachShader(GLuint program, GLuint shader) {
    if (g_current == nullptr) return;
    g_current->attachShader(program, shader);
}

void glLinkProgram(GLuint program) {
    if (g_current == nullptr) return;
    g_current->linkProgram(program);
}

void glProgramParameteri(GLuint program, GLenum pname, GLint value) {
    if (g_current == nullptr) return;
    g_current->programParameteri(program, pname, value);
}

GLint glGetProgramiv(GLuint program, GLenum pname) {
    if (g_current == nullptr) return 0;
    return g_current->getProgramiv(program, pname);
}

void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length,
                        GLchar* infoLog) {
    if (g_current == nullptr) return;
    g_current->getProgramInfoLog(program, static_cast<uint32_t>(bufSize), length,
                                infoLog);
}

GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface,
                                 const GLchar* name) {
    if (g_current == nullptr) return GL_INVALID_INDEX;
    return g_current->getProgramResourceIndex(program, programInterface,
                                             name ? name : "");
}

void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index,
                              GLsizei bufSize, GLsizei* length, GLchar* name) {
    if (g_current == nullptr) return;
    g_current->getProgramResourceName(program, programInterface, index, bufSize,
                                     length, name);
}

void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index,
                            GLsizei propCount, const GLenum* props, GLsizei bufSize,
                            GLsizei* length, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getProgramResourceiv(program, programInterface, index, propCount,
                                    reinterpret_cast<const uint32_t*>(props),
                                    bufSize, length, params);
}

GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface,
                                   const GLchar* name) {
    if (g_current == nullptr) return -1;
    return g_current->getProgramResourceLocation(program, programInterface,
                                                name ? name : "");
}

GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface,
                                         const GLchar* name) {
    if (g_current == nullptr) return -1;
    return g_current->getProgramResourceLocationIndex(program, programInterface,
                                                     name ? name : "");
}

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize,
                        GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    if (g_current == nullptr) return;
    g_current->getActiveUniform(program, index, bufSize, length, size,
                                reinterpret_cast<uint32_t*>(type), name);
}

void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize,
                       GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    if (g_current == nullptr) return;
    g_current->getActiveAttrib(program, index, bufSize, length, size,
                               reinterpret_cast<uint32_t*>(type), name);
}

GLuint glGetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName) {
    if (g_current == nullptr) return GL_INVALID_INDEX;
    return g_current->getUniformBlockIndex(program,
                                           uniformBlockName ? uniformBlockName : "");
}

void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex,
                               GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}

void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex,
                                 GLsizei bufSize, GLsizei* length,
                                 GLchar* uniformBlockName) {
    if (g_current == nullptr) return;
    g_current->getActiveUniformBlockName(program, uniformBlockIndex, bufSize, length,
                                         uniformBlockName);
}

GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype,
                            const GLchar* name) {
    if (g_current == nullptr) return GL_INVALID_INDEX;
    return g_current->getSubroutineIndex(program, shadertype, name ? name : "");
}

GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype,
                                     const GLchar* name) {
    if (g_current == nullptr) return -1;
    return g_current->getSubroutineUniformLocation(program, shadertype,
                                                  name ? name : "");
}

void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index,
                                    GLenum pname, GLint* values) {
    if (g_current == nullptr) return;
    g_current->getActiveSubroutineUniformiv(program, shadertype, index, pname,
                                           values);
}

void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype,
                                      GLuint index, GLsizei bufSize,
                                      GLsizei* length, GLchar* name) {
    if (g_current == nullptr) return;
    g_current->getActiveSubroutineUniformName(program, shadertype, index, bufSize,
                                             length, name);
}

void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index,
                               GLsizei bufSize, GLsizei* length, GLchar* name) {
    if (g_current == nullptr) return;
    g_current->getActiveSubroutineName(program, shadertype, index, bufSize, length,
                                      name);
}

void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count,
                            const GLuint* indices) {
    if (g_current == nullptr) return;
    g_current->uniformSubroutinesuiv(shadertype, count, indices);
}

void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint* params) {
    if (g_current == nullptr) return;
    g_current->getUniformSubroutineuiv(shadertype, location, params);
}

void glDeleteProgram(GLuint program) {
    if (g_current == nullptr) return;
    g_current->deleteProgram(program);
}

GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    if (g_current == nullptr) return -1;
    return g_current->getAttribLocation(program, name ? name : "");
}

void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
    if (g_current == nullptr) return;
    g_current->bindAttribLocation(program, index, name ? name : "");
}

// --- Uniforms (SPEC §8) ---

GLint glGetUniformLocation(GLuint program, const GLchar* name) {
    if (g_current == nullptr) return -1;
    return g_current->getUniformLocation(program, name ? name : "");
}

void glUniform1f(GLint location, GLfloat v0) {
    if (g_current == nullptr) return;
    g_current->uniform1f(location, v0);
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    if (g_current == nullptr) return;
    g_current->uniform2f(location, v0, v1);
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    if (g_current == nullptr) return;
    g_current->uniform3f(location, v0, v1, v2);
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    if (g_current == nullptr) return;
    g_current->uniform4f(location, v0, v1, v2, v3);
}
void glUniform1i(GLint location, GLint v0) {
    if (g_current == nullptr) return;
    g_current->uniform1i(location, v0);
}
void glUniform2i(GLint location, GLint v0, GLint v1) {
    if (g_current == nullptr) return;
    g_current->uniform2i(location, v0, v1);
}
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    if (g_current == nullptr) return;
    g_current->uniform3i(location, v0, v1, v2);
}
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    if (g_current == nullptr) return;
    g_current->uniform4i(location, v0, v1, v2, v3);
}
void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) {
    if (g_current == nullptr) return;
    g_current->uniform1fv(location, value, static_cast<int>(count));
}
void glUniform1iv(GLint location, GLsizei count, const GLint* value) {
    if (g_current == nullptr) return;
    g_current->uniform1iv(location, value, static_cast<int>(count));
}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                       const GLfloat* value) {
    if (g_current == nullptr) return;
    g_current->uniformMatrix4fv(location, value, static_cast<int>(count),
                               transpose != 0);
}

// --- Vertex attributes (SPEC §2.1) ---

void glEnableVertexAttribArray(GLuint index) {
    if (g_current == nullptr) return;
    g_current->enableVertexAttribArray(index);
}

void glDisableVertexAttribArray(GLuint index) {
    if (g_current == nullptr) return;
    g_current->disableVertexAttribArray(index);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                           GLboolean normalized, GLint stride,
                           const GLvoid* offset) {
    if (g_current == nullptr) return;
    g_current->vertexAttribPointer(index, size, type, normalized != 0, stride,
                                   reinterpret_cast<intptr_t>(offset));
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

void glEnablei(GLenum cap, GLuint index) {
    if (g_current == nullptr) return;
    g_current->enableIndexed(cap, index);
}

void glDisablei(GLenum cap, GLuint index) {
    if (g_current == nullptr) return;
    g_current->disableIndexed(cap, index);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    if (g_current == nullptr) return;
    g_current->state().setBlendFunc(sfactor, dfactor);
}

void glBlendEquation(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->state().setBlendEquation(mode);
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha,
                        GLenum dstAlpha) {
    if (g_current == nullptr) return;
    g_current->state().setBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    if (g_current == nullptr) return;
    g_current->state().setBlendEquationSeparate(modeRGB, modeAlpha);
}

void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    if (g_current == nullptr) return;
    g_current->state().setBlendColor(red, green, blue, alpha);
}

void glUseProgram(GLuint prog) {
    if (g_current == nullptr) return;
    g_current->state().useProgram(prog);
}

// --- Program pipelines (SPEC §7.4) ---

GLuint glCreateShaderProgramv(GLenum type, GLsizei count,
                              const GLchar* const* strings) {
    if (g_current == nullptr) return 0;
    return g_current->createShaderProgramv(static_cast<uint32_t>(type), count,
                                           strings);
}

void glGenProgramPipelines(GLsizei n, GLuint* pipelines) {
    if (g_current == nullptr) return;
    g_current->genProgramPipelines(static_cast<uint32_t>(n), pipelines);
}

void glDeleteProgramPipelines(GLsizei n, const GLuint* pipelines) {
    if (g_current == nullptr) return;
    g_current->deleteProgramPipelines(static_cast<uint32_t>(n), pipelines);
}

GLboolean glIsProgramPipeline(GLuint pipeline) {
    if (g_current == nullptr) return GL_FALSE;
    return g_current->isProgramPipeline(pipeline) ? GL_TRUE : GL_FALSE;
}

void glBindProgramPipeline(GLuint pipeline) {
    if (g_current == nullptr) return;
    g_current->bindProgramPipeline(pipeline);
}

void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {
    if (g_current == nullptr) return;
    g_current->useProgramStages(pipeline, static_cast<uint32_t>(stages), program);
}

void glActiveShaderProgram(GLuint pipeline, GLuint program) {
    if (g_current == nullptr) return;
    g_current->activeShaderProgram(pipeline, program);
}

void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getProgramPipelineiv(pipeline, static_cast<uint32_t>(pname),
                                    params);
}

void glValidateProgramPipeline(GLuint pipeline) {
    if (g_current == nullptr) return;
    g_current->validateProgramPipeline(pipeline);
}

void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize,
                                 GLsizei* length, GLchar* infoLog) {
    if (g_current == nullptr) return;
    g_current->getProgramPipelineInfoLog(pipeline,
                                         static_cast<uint32_t>(bufSize), length,
                                         infoLog);
}

void glDepthFunc(GLenum func) {
    if (g_current == nullptr) return;
    g_current->state().setDepthFunc(func);
}

void glDepthMask(bool flag) {
    if (g_current == nullptr) return;
    g_current->state().setDepthMask(flag);
}

void glDepthRange(GLdouble nearVal, GLdouble farVal) {
    if (g_current == nullptr) return;
    g_current->state().setDepthRange(static_cast<double>(nearVal),
                                     static_cast<double>(farVal));
}

void glDepthRangef(GLfloat nearVal, GLfloat farVal) {
    if (g_current == nullptr) return;
    g_current->state().setDepthRange(static_cast<double>(nearVal),
                                     static_cast<double>(farVal));
}

void glCullFace(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->state().setCullFace(mode);
}

void glFrontFace(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->state().setFrontFace(mode);
}

void glPointSize(GLfloat size) {
    if (g_current == nullptr) return;
    g_current->state().setPointSize(static_cast<float>(size));
}

void glLineWidth(GLfloat width) {
    if (g_current == nullptr) return;
    g_current->state().setLineWidth(static_cast<float>(width));
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    if (g_current == nullptr) return;
    g_current->state().setPolygonOffset(static_cast<float>(factor),
                                        static_cast<float>(units));
}

void glPolygonMode(GLenum face, GLenum mode) {
    if (g_current == nullptr) return;
    g_current->polygonMode(face, mode);
}

void glSampleMaski(GLuint maskNumber, GLuint mask) {
    if (g_current == nullptr) return;
    g_current->sampleMaski(maskNumber, mask);
}

void glMinSampleShading(GLfloat value) {
    if (g_current == nullptr) return;
    g_current->minSampleShading(static_cast<float>(value));
}

void glProvokingVertex(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->provokingVertex(mode);
}

void glClampColor(GLenum target, GLenum mode) {
    if (g_current == nullptr) return;
    g_current->clampColor(target, mode);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (g_current == nullptr) return;
    g_current->state().setStencilFunc(func, ref, mask);
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (g_current == nullptr) return;
    g_current->state().setStencilOp(sfail, dpfail, dppass);
}

void glStencilMask(GLuint mask) {
    if (g_current == nullptr) return;
    g_current->state().setStencilMask(mask);
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    if (g_current == nullptr) return;
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        g_current->setError(GLError::InvalidEnum);
        return;
    }
    g_current->state().setStencilFuncSeparate(face, func, ref, mask);
}

void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (g_current == nullptr) return;
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        g_current->setError(GLError::InvalidEnum);
        return;
    }
    g_current->state().setStencilOpSeparate(face, sfail, dpfail, dppass);
}

void glStencilMaskSeparate(GLenum face, GLuint mask) {
    if (g_current == nullptr) return;
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        g_current->setError(GLError::InvalidEnum);
        return;
    }
    g_current->state().setStencilMaskSeparate(face, mask);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    if (g_current == nullptr) return;
    g_current->state().setColorMask(red != 0, green != 0, blue != 0, alpha != 0);
}

void glSampleCoverage(GLfloat value, GLboolean invert) {
    if (g_current == nullptr) return;
    g_current->state().setSampleCoverage(value, invert != 0);
}

void glPrimitiveRestartIndex(GLuint index) {
    if (g_current == nullptr) return;
    g_current->primitiveRestartIndex(index);
}

void glPixelStorei(GLenum pname, GLint param) {
    if (g_current == nullptr) return;
    g_current->pixelStorei(pname, static_cast<int>(param));
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->setViewport(x, y, width, height);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->setScissor(x, y, width, height);
}

void glFlushState() {
    if (g_current == nullptr) return;
    g_current->flushState();
}

void glGetBooleanv(GLenum pname, GLboolean* params) {
    if (g_current == nullptr) return;
    g_current->getBooleanv(pname, params);
}

void glGetIntegerv(GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getIntegerv(pname, params);
}

void glGetFloatv(GLenum pname, GLfloat* params) {
    if (g_current == nullptr) return;
    g_current->getFloatv(pname, params);
}

void glGetDoublev(GLenum pname, GLdouble* params) {
    if (g_current == nullptr) return;
    g_current->getDoublev(pname, params);
}

GLboolean glIsEnabled(GLenum cap) {
    if (g_current == nullptr) return 0;
    return g_current->isEnabled(cap) ? 1 : 0;
}

GLboolean glIsEnabledi(GLenum cap, GLuint index) {
    if (g_current == nullptr) return 0;
    return g_current->isEnabledIndexed(cap, index) ? 1 : 0;
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    if (g_current == nullptr) return;
    g_current->setClearColor(red, green, blue, alpha);
}

void glClearDepth(GLdouble depth) {
    if (g_current == nullptr) return;
    g_current->setClearDepth(depth);
}

void glClearDepthf(GLfloat depth) {
    if (g_current == nullptr) return;
    g_current->setClearDepth(static_cast<double>(depth));
}

void glClear(GLuint mask) {
    if (g_current == nullptr) return;
    g_current->clear(mask);
}

void glDrawBuffers(GLsizei n, const GLenum* bufs) {
    if (g_current == nullptr) return;
    g_current->drawBuffers(static_cast<int32_t>(n), bufs);
}

void glReadBuffer(GLenum buf) {
    if (g_current == nullptr) return;
    g_current->readBuffer(buf);
}

void glLogicOp(GLenum mode) {
    if (g_current == nullptr) return;
    g_current->logicOp(mode);
}

void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter) {
    if (g_current == nullptr) return;
    g_current->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                              dstY1, mask, filter);
}

void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments,
                            const GLenum* attachments) {
    if (g_current == nullptr) return;
    g_current->invalidateFramebuffer(target, numAttachments, attachments);
}

void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments,
                               const GLenum* attachments, GLint x, GLint y,
                               GLsizei width, GLsizei height) {
    if (g_current == nullptr) return;
    g_current->invalidateSubFramebuffer(target, numAttachments, attachments, x, y,
                                      width, height);
}

void glFlush() {
    if (g_current == nullptr) return;
    g_current->flushCommands();
}

void glFinish() {
    if (g_current == nullptr) return;
    g_current->finishCommands();
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                  GLenum type, GLvoid* pixels) {
    if (g_current == nullptr) return;
    g_current->readPixels(x, y, width, height, format, type, pixels);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (g_current == nullptr) return;
    g_current->drawArrays(mode, first, count);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type,
                    const GLvoid* indices) {
    if (g_current == nullptr) return;
    g_current->drawElements(mode, count, type,
                            reinterpret_cast<intptr_t>(indices));
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count,
                           GLsizei primcount) {
    if (g_current == nullptr) return;
    g_current->drawArraysInstanced(mode, first, count, primcount);
}

void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
                             const GLvoid* indices, GLsizei primcount) {
    if (g_current == nullptr) return;
    g_current->drawElementsInstanced(mode, count, type,
                                     reinterpret_cast<intptr_t>(indices),
                                     primcount);
}

void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    if (g_current == nullptr) return;
    g_current->vertexAttribDivisor(index, divisor);
}

void glVertexAttrib1f(GLuint index, GLfloat x) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib1f(index, x);
}

void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib2f(index, x, y);
}

void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib3f(index, x, y, z);
}

void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib4f(index, x, y, z, w);
}

void glVertexAttrib1fv(GLuint index, const GLfloat* v) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib1fv(index, v);
}

void glVertexAttrib2fv(GLuint index, const GLfloat* v) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib2fv(index, v);
}

void glVertexAttrib3fv(GLuint index, const GLfloat* v) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib3fv(index, v);
}

void glVertexAttrib4fv(GLuint index, const GLfloat* v) {
    if (g_current == nullptr) return;
    g_current->vertexAttrib4fv(index, v);
}

void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    if (g_current == nullptr) return;
    g_current->vertexAttribI4i(index, x, y, z, w);
}

void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    if (g_current == nullptr) return;
    g_current->vertexAttribI4ui(index, x, y, z, w);
}

void glVertexAttribI4iv(GLuint index, const GLint* v) {
    if (g_current == nullptr) return;
    g_current->vertexAttribI4iv(index, v);
}

void glVertexAttribI4uiv(GLuint index, const GLuint* v) {
    if (g_current == nullptr) return;
    g_current->vertexAttribI4uiv(index, v);
}

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params) {
    if (g_current == nullptr) return;
    g_current->getVertexAttribfv(index, pname, params);
}

void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params) {
    if (g_current == nullptr) return;
    g_current->getVertexAttribiv(index, pname, params);
}

void glHint(GLenum target, GLenum mode) {
    if (g_current == nullptr) return;
    g_current->hint(target, mode);
}

void glMultiDrawArrays(GLenum mode, const GLint* firsts, const GLint* counts,
                       GLsizei drawcount) {
    if (g_current == nullptr) return;
    g_current->multiDrawArrays(mode, firsts, counts, drawcount);
}

void glMultiDrawElements(GLenum mode, const GLint* counts, GLenum type,
                        const GLvoid* const* indices, GLsizei drawcount) {
    if (g_current == nullptr) return;
    g_current->multiDrawElements(mode, counts, type,
                                reinterpret_cast<const intptr_t*>(indices),
                                drawcount);
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                        GLenum type, const GLvoid* indices) {
    if (g_current == nullptr) return;
    g_current->drawRangeElements(mode, start, end, count, type,
                                reinterpret_cast<intptr_t>(indices));
}

 void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
                              const GLvoid* indices, GLint basevertex) {
     if (g_current == nullptr) return;
     g_current->drawElementsBaseVertex(mode, count, type,
                                      reinterpret_cast<intptr_t>(indices),
                                      basevertex);
 }

 void glDrawArraysIndirect(GLenum mode, const GLvoid* indirect) {
     if (g_current == nullptr) return;
     g_current->drawArraysIndirect(mode, indirect);
 }

  void glDrawElementsIndirect(GLenum mode, GLenum type, const GLvoid* indirect) {
      if (g_current == nullptr) return;
      g_current->drawElementsIndirect(mode, type, indirect);
  }

  void glDispatchCompute(GLuint x, GLuint y, GLuint z) {
      if (g_current == nullptr) return;
      g_current->dispatchCompute(x, y, z);
  }

  void glDispatchComputeIndirect(const GLvoid* indirect) {
      if (g_current == nullptr) return;
      g_current->dispatchComputeIndirect(reinterpret_cast<uintptr_t>(indirect));
  }


 } // namespace glcompat
