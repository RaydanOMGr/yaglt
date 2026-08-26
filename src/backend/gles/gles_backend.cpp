#include "glcompat/backend/gles/gles_backend.hpp"
#include "glcompat/core/log.hpp"

#ifdef YAGLT_SHADER_TRANSLATE
#include "src/backend/gles/gles_translating_compiler.hpp"
#endif

#include <EGL/eglext.h>

#include <cstdlib>
#include <cstring>
#include <string>

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif
#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif
#ifndef EGL_CONTEXT_MAJOR_VERSION
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#endif
#ifndef EGL_CONTEXT_MINOR_VERSION
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#endif

namespace glcompat {

GLESBackend::GLESBackend()
    : lib_(std::make_shared<GLESLib>()),
      factory_(lib_),
      compiler_(std::make_unique<GLESShaderCompiler>(lib_)) {
#ifdef YAGLT_SHADER_TRANSLATE
    compiler_ = std::make_unique<TranslatingGLESShaderCompiler>(lib_);
#endif
}

GLESBackend::~GLESBackend() {
    if (initialized_) shutdown();
}

bool GLESBackend::createContext() {
    if (!lib_->eglGetPlatformDisplay && !lib_->eglGetDisplay) return false;

    // Prefer a surfaceless display (no window system needed).
    if (lib_->eglGetPlatformDisplay) {
        EGLint attrs[] = {EGL_NONE};
        display_ = lib_->eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                               EGL_DEFAULT_DISPLAY, attrs);
    }
    if (display_ == EGL_NO_DISPLAY && lib_->eglGetDisplay) {
        display_ = lib_->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (display_ == EGL_NO_DISPLAY) return false;

    EGLint major = 0, minor = 0;
    if (!lib_->eglInitialize(display_, &major, &minor)) return false;

    // Choose a config suitable for offscreen rendering.
    static const EGLint cfgAttrs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16,
        EGL_NONE};
    EGLConfig config = nullptr;
    EGLint num = 0;
    if (!lib_->eglChooseConfig(display_, cfgAttrs, &config, 1, &num) || num == 0) {
        return false;
    }

    static const EGLint ctxAttrs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE};
    context_ = lib_->eglCreateContext(display_, config, EGL_NO_CONTEXT, ctxAttrs);
    if (context_ == EGL_NO_CONTEXT) return false;

    // Surfaceless: bind with no draw/read surfaces.
    if (!lib_->eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
        return false;
    }
    return true;
}

void GLESBackend::queryVersion() {
    const GLubyte* v = lib_->glGetString(GL_VERSION);
    lib_->versionString = v ? reinterpret_cast<const char*>(v) : "";
    const GLubyte* r = lib_->glGetString(GL_RENDERER);
    lib_->rendererString = r ? reinterpret_cast<const char*>(r) : "";

    lib_->glesMajor = 0;
    lib_->glesMinor = 0;
    // Typical: "OpenGL ES 3.1 v1.2.3" or "OpenGL ES 2.0".
    std::string s = lib_->versionString;
    auto pos = s.find("OpenGL ES ");
    if (pos != std::string::npos) {
        s = s.substr(pos + 10);
        int mj = std::atoi(s.c_str());
        size_t dot = s.find('.');
        int mn = 0;
        if (dot != std::string::npos) mn = std::atoi(s.c_str() + dot + 1);
        lib_->glesMajor = mj;
        lib_->glesMinor = mn;
    }

    // Extensions: GLES3 exposes glGetStringi; fall back to glGetString.
    lib_->extensionsString.clear();
    if (lib_->glGetStringi && lib_->glGetIntegerv) {
        GLint n = 0;
        lib_->glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (GLint i = 0; i < n; ++i) {
            const GLubyte* e = nullptr;
            lib_->glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i), &e);
            if (e) {
                if (!lib_->extensionsString.empty()) lib_->extensionsString += " ";
                lib_->extensionsString += reinterpret_cast<const char*>(e);
            }
        }
    } else if (lib_->glGetString) {
        const GLubyte* e = lib_->glGetString(GL_EXTENSIONS);
        if (e) lib_->extensionsString = reinterpret_cast<const char*>(e);
    }
}

bool GLESBackend::initialize() {
    if (initialized_) return true;
    if (!lib_->load()) return false;
    if (!createContext()) {
        lib_->unload();
        return false;
    }
    queryVersion();
    populateGLESCapabilities(caps_, *lib_);
    log(LogCategory::Backend, LogLevel::Info) << "selected backend: GLES (ES "
        << lib_->glesMajor << "." << lib_->glesMinor
        << ", renderer=" << lib_->rendererString << ")";
    log(LogCategory::GLES, LogLevel::Debug) << "detected extensions: "
        << (lib_->extensionsString.empty() ? "(none)" : lib_->extensionsString);
    caps_.report();
    initialized_ = true;
    return true;
}

void GLESBackend::shutdown() {
    if (display_ != EGL_NO_DISPLAY) {
        if (context_ != EGL_NO_CONTEXT && lib_->eglDestroyContext)
            lib_->eglDestroyContext(display_, context_);
        if (lib_->eglTerminate) lib_->eglTerminate(display_);
    }
    context_ = EGL_NO_CONTEXT;
    display_ = EGL_NO_DISPLAY;
    initialized_ = false;
}

std::string GLESBackend::describe() const {
    return "GLESBackend(ES " + std::to_string(lib_->glesMajor) + "." +
           std::to_string(lib_->glesMinor) + ", renderer=" + lib_->rendererString + ")";
}

// --- GLStateSink (SPEC §10) ---

void GLESBackend::enable(GLenum cap) {
    if (lib_->glEnable) lib_->glEnable(cap);
}

void GLESBackend::disable(GLenum cap) {
    if (lib_->glDisable) lib_->glDisable(cap);
}

void GLESBackend::useProgram(uint32_t prog) {
    // Translate the frontend program name to the native driver id when known.
    auto it = nativeMap_.find(prog);
    GLuint native = it != nativeMap_.end() ? it->second : prog;
    if (lib_->glUseProgram) lib_->glUseProgram(native);
    // Keep the loader's active-program cache in sync so the program's own uniform
    // calls (which bind via the same loader) don't issue a redundant glUseProgram.
    lib_->currentProgram = native;
}

void GLESBackend::bindVertexArray(uint32_t vao) {
    auto it = nativeMap_.find(vao);
    GLuint native = it != nativeMap_.end() ? it->second : vao;
    if (lib_->glBindVertexArray) lib_->glBindVertexArray(native);
}

void GLESBackend::activeTexture(uint32_t unit) {
    if (lib_->glActiveTexture) lib_->glActiveTexture(unit);
}

void GLESBackend::bindTexture(uint32_t target, uint32_t texture) {
    // Resolve the frontend texture name to the native driver id when known.
    auto it = nativeMap_.find(texture);
    GLuint native = it != nativeMap_.end() ? it->second : texture;
    if (lib_->glBindTexture) lib_->glBindTexture(target, native);
}

void GLESBackend::bindSampler(uint32_t unit, uint32_t sampler) {
    // Resolve the frontend sampler name to the native driver id when known.
    auto it = nativeMap_.find(sampler);
    GLuint native = it != nativeMap_.end() ? it->second : sampler;
    if (lib_->glBindSampler) lib_->glBindSampler(unit, native);
}

void GLESBackend::enableVertexAttribArray(uint32_t index) {
    if (lib_->glEnableVertexAttribArray)
        lib_->glEnableVertexAttribArray(static_cast<GLuint>(index));
}

void GLESBackend::disableVertexAttribArray(uint32_t index) {
    if (lib_->glDisableVertexAttribArray)
        lib_->glDisableVertexAttribArray(static_cast<GLuint>(index));
}

void GLESBackend::vertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                      bool normalized, int32_t stride,
                                      intptr_t offset) {
    if (lib_->glVertexAttribPointer)
        lib_->glVertexAttribPointer(static_cast<GLuint>(index),
                                    static_cast<GLint>(size), type,
                                    normalized ? GL_TRUE : GL_FALSE,
                                    static_cast<GLsizei>(stride),
                                    reinterpret_cast<const void*>(offset));
}

void GLESBackend::bindNativeObject(uint32_t name, uint32_t nativeId) {
    nativeMap_[name] = nativeId;
}

void GLESBackend::blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB,
                                    uint32_t srcAlpha, uint32_t dstAlpha) {
    if (lib_->glBlendFuncSeparate)
        lib_->glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    else if (lib_->glBlendFunc)
        lib_->glBlendFunc(srcRGB, dstRGB); // fall back: RGB factors (no alpha split)
}

void GLESBackend::blendEquationSeparate(uint32_t modeRGB, uint32_t modeAlpha) {
    if (lib_->glBlendEquationSeparate)
        lib_->glBlendEquationSeparate(modeRGB, modeAlpha);
    else if (lib_->glBlendEquation)
        lib_->glBlendEquation(modeRGB); // fall back: single equation
}

void GLESBackend::blendColor(float r, float g, float b, float a) {
    if (lib_->glBlendColor) lib_->glBlendColor(r, g, b, a);
}

void GLESBackend::depthFunc(GLenum func) {
    if (lib_->glDepthFunc) lib_->glDepthFunc(func);
}

void GLESBackend::depthMask(bool enabled) {
    if (lib_->glDepthMask) lib_->glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void GLESBackend::depthRange(double nearVal, double farVal) {
    if (lib_->glDepthRangef) {
        // GLES uses float depth range; promote double to float.
        lib_->glDepthRangef(static_cast<GLfloat>(nearVal),
                           static_cast<GLfloat>(farVal));
    }
}

void GLESBackend::stencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (lib_->glStencilFunc) lib_->glStencilFunc(func, ref, mask);
}

void GLESBackend::stencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (lib_->glStencilOp) lib_->glStencilOp(sfail, dpfail, dppass);
}

void GLESBackend::stencilMask(GLuint mask) {
    if (lib_->glStencilMask) lib_->glStencilMask(mask);
}

void GLESBackend::colorMask(bool r, bool g, bool b, bool a) {
    if (lib_->glColorMask)
        lib_->glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE,
                         b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
}

void GLESBackend::cullFace(GLenum mode) {
    if (lib_->glCullFace) lib_->glCullFace(mode);
}

void GLESBackend::frontFace(GLenum mode) {
    if (lib_->glFrontFace) lib_->glFrontFace(mode);
}

void GLESBackend::pointSize(float size) {
    if (lib_->glPointSize) lib_->glPointSize(size);
}

void GLESBackend::lineWidth(float width) {
    if (lib_->glLineWidth) lib_->glLineWidth(width);
}

void GLESBackend::polygonOffset(float factor, float units) {
    if (lib_->glPolygonOffset) lib_->glPolygonOffset(factor, units);
}

void GLESBackend::pixelStorei(GLenum pname, GLint param) {
    if (lib_->glPixelStorei) lib_->glPixelStorei(pname, param);
}

void GLESBackend::setViewport(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (lib_->glViewport) lib_->glViewport(x, y, w, h);
}

void GLESBackend::setScissor(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (lib_->glScissor) lib_->glScissor(x, y, w, h);
}

void GLESBackend::clearColor(float r, float g, float b, float a) {
    if (lib_->glClearColor) lib_->glClearColor(r, g, b, a);
}

void GLESBackend::clearDepth(double d) {
    if (lib_->glClearDepthf) {
        // GLES uses a float depth clear value; promote double to float.
        lib_->glClearDepthf(static_cast<GLfloat>(d));
    }
}

void GLESBackend::drawBuffers(int32_t n, const uint32_t* bufs) {
    if (lib_->glDrawBuffers && n > 0 && bufs)
        lib_->glDrawBuffers(n, reinterpret_cast<const GLenum*>(bufs));
}

void GLESBackend::readBuffer(uint32_t buf) {
    if (lib_->glReadBuffer) lib_->glReadBuffer(buf);
}

void GLESBackend::bindBufferBase(uint32_t target, uint32_t index,
                                  uint32_t buffer) {
    if (lib_->glBindBufferBase)
        lib_->glBindBufferBase(target, index, buffer);
}

void GLESBackend::bindBufferRange(uint32_t target, uint32_t index,
                                   uint32_t buffer, intptr_t offset,
                                   intptr_t size) {
    if (lib_->glBindBufferRange)
        lib_->glBindBufferRange(target, index, buffer, offset, size);
}

void GLESBackend::drawArrays(uint32_t mode, int32_t first, int32_t count) {
    if (lib_->glDrawArrays) lib_->glDrawArrays(mode, first, count);
}

void GLESBackend::drawElements(uint32_t mode, int32_t count, uint32_t type,
                               intptr_t indices) {
    if (lib_->glDrawElements)
        lib_->glDrawElements(mode, count, type,
                             reinterpret_cast<const void*>(indices));
}

void GLESBackend::drawArraysInstanced(uint32_t mode, int32_t first,
                                      int32_t count, int32_t primcount) {
    if (lib_->glDrawArraysInstanced)
        lib_->glDrawArraysInstanced(mode, first, count, primcount);
}

void GLESBackend::drawElementsInstanced(uint32_t mode, int32_t count,
                                         uint32_t type, intptr_t indices,
                                         int32_t primcount) {
    if (lib_->glDrawElementsInstanced)
        lib_->glDrawElementsInstanced(mode, count, type,
                                      reinterpret_cast<const void*>(indices),
                                      primcount);
}

void GLESBackend::clear(uint32_t mask) {
    if (lib_->glClear) lib_->glClear(mask);
}

void GLESBackend::flush() {
    if (lib_->glFlush) lib_->glFlush();
}

void GLESBackend::finish() {
    if (lib_->glFinish) lib_->glFinish();
}

void GLESBackend::readPixels(int32_t x, int32_t y, int32_t width, int32_t height,
                             uint32_t format, uint32_t type, void* pixels) {
    if (lib_->glReadPixels)
        lib_->glReadPixels(x, y, width, height, format, type, pixels);
}

void GLESBackend::logicOp(uint32_t mode) {
    if (lib_->glLogicOp) lib_->glLogicOp(mode);
}

void GLESBackend::blitFramebuffer(int32_t srcX0, int32_t srcY0, int32_t srcX1,
                                  int32_t srcY1, int32_t dstX0, int32_t dstY0,
                                  int32_t dstX1, int32_t dstY1, uint32_t mask,
                                  uint32_t filter) {
    if (lib_->glBlitFramebuffer)
        lib_->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                                dstY1, mask, filter);
}

void GLESBackend::invalidateFramebuffer(uint32_t target, int32_t numAttachments,
                                       const uint32_t* attachments, int32_t x,
                                       int32_t y, int32_t width, int32_t height) {
    if (numAttachments <= 0 || !attachments) return;
    if (width != 0 || height != 0) {
        if (lib_->glInvalidateSubFramebuffer)
            lib_->glInvalidateSubFramebuffer(
                target, numAttachments,
                reinterpret_cast<const GLenum*>(attachments), x, y, width, height);
    } else if (lib_->glInvalidateFramebuffer) {
        lib_->glInvalidateFramebuffer(
            target, numAttachments, reinterpret_cast<const GLenum*>(attachments));
    }
}

} // namespace glcompat
