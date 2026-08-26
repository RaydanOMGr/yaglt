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

void glBindTexture(GLenum, GLuint texture) {
    if (g_current == nullptr) return;
    g_current->bindTexture(texture);
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

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    if (g_current == nullptr) return;
    g_current->texParameteri(target, pname, static_cast<int>(param));
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

void glDeleteProgram(GLuint program) {
    if (g_current == nullptr) return;
    g_current->deleteProgram(program);
}

GLint glGetAttribLocation(GLuint program, const GLchar* name) {
    if (g_current == nullptr) return -1;
    return g_current->getAttribLocation(program, name ? name : "");
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

} // namespace glcompat
