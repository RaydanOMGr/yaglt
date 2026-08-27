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
    if (!lib_->eglGetPlatformDisplay && !lib_->eglGetDisplay) {
        log(LogCategory::GLES, LogLevel::Error)
            << "createContext: neither eglGetPlatformDisplay nor eglGetDisplay resolved";
        return false;
    }

    // Use the plain eglGetDisplay(EGL_DEFAULT_DISPLAY) path. On Mesa's
    // surfaceless/headless build (EGL_PLATFORM=surfaceless) this yields a usable
    // display with no window system. The platform-display entry points
    // (eglGetPlatformDisplay[eglGetPlatformDisplayEXT] with
    // EGL_PLATFORM_SURFACELESS_MESA) route through Mesa's _eglFindDisplay, which
    // over-reads its attrib list and corrupts the stack; we deliberately avoid
    // them on the headless path the project targets.
    if (lib_->eglGetDisplay) {
        display_ = lib_->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (display_ == EGL_NO_DISPLAY) {
        log(LogCategory::GLES, LogLevel::Error)
            << "createContext: eglGetDisplay(EGL_DEFAULT_DISPLAY) failed ("
            << (lib_->eglGetError ? lib_->eglGetError() : 0) << ")";
        return false;
    }

    EGLint major = 0, minor = 0;
    if (!lib_->eglInitialize(display_, &major, &minor)) {
        log(LogCategory::GLES, LogLevel::Error)
            << "createContext: eglInitialize failed (" << lib_->eglGetError() << ")";
        return false;
    }

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
        log(LogCategory::GLES, LogLevel::Error)
            << "createContext: eglChooseConfig failed (" << lib_->eglGetError()
            << ", num=" << num << ")";
        return false;
    }

    static const EGLint ctxAttrs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE};
    context_ = lib_->eglCreateContext(display_, config, EGL_NO_CONTEXT, ctxAttrs);
    if (context_ == EGL_NO_CONTEXT) {
        log(LogCategory::GLES, LogLevel::Error)
            << "createContext: eglCreateContext failed (" << lib_->eglGetError() << ")";
        return false;
    }

    // Surfaceless: bind with no draw/read surfaces.
    if (!lib_->eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
        log(LogCategory::GLES, LogLevel::Error)
            << "createContext: eglMakeCurrent failed (" << lib_->eglGetError() << ")";
        return false;
    }
    lib_->contextAlive = true;
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
    // Mark the context dead first so any backend resource that outlives this
    // call (e.g. a buffer the caller still holds) skips its driver teardown in
    // its destructor instead of calling glDelete* on a terminated context,
    // which is undefined and can corrupt driver heap state.
    if (lib_) lib_->contextAlive = false;
    if (display_ != EGL_NO_DISPLAY) {
        // EGL requires releasing the current context before destroying it or
        // terminating the display; destroying a context that is still current
        // leaves Mesa's internal context state dangling and lets the driver
        // scribble freed memory (corrupting the process heap). Unbind first.
        if (context_ != EGL_NO_CONTEXT && lib_->eglMakeCurrent)
            lib_->eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                 EGL_NO_CONTEXT);
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

void GLESBackend::hint(uint32_t target, uint32_t mode) {
    if (lib_->glHint) lib_->glHint(target, mode);
}

void GLESBackend::bindProgramPipeline(uint32_t pipeline) {
    // GLES has no separable program pipeline object to install (a single linked
    // program drives each draw), so there is no native call to forward. The
    // frontend still tracks the bound pipeline and answers glGetProgramPipelineiv;
    // where separable programs are genuinely available the ProgramPipelines
    // capability reports Emulated and the frontend models the stages (consumed
    // for draws only by backends that supply the per-stage wiring).
    (void)pipeline;
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

void GLESBackend::bindBuffer(uint32_t target, uint32_t buffer) {
    // Resolve the frontend buffer name to the native driver id when known.
    auto it = nativeMap_.find(buffer);
    GLuint native = it != nativeMap_.end() ? it->second : buffer;
    if (lib_->glBindBuffer) lib_->glBindBuffer(target, native);
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

void GLESBackend::vertexAttribDivisor(uint32_t index, uint32_t divisor) {
    if (lib_->glVertexAttribDivisor)
        lib_->glVertexAttribDivisor(static_cast<GLuint>(index),
                                    static_cast<GLuint>(divisor));
}

void GLESBackend::bindNativeObject(uint32_t name, uint32_t nativeId) {
    nativeMap_[name] = nativeId;
}

void GLESBackend::bindFramebuffer(uint32_t target, uint32_t framebuffer) {
    // Resolve the frontend framebuffer name to the native driver id when known.
    auto it = nativeMap_.find(framebuffer);
    GLuint native = it != nativeMap_.end() ? it->second : framebuffer;
    if (lib_->glBindFramebuffer) lib_->glBindFramebuffer(target, native);
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

void GLESBackend::sampleCoverage(float value, bool invert) {
    if (lib_->glSampleCoverage)
        lib_->glSampleCoverage(value, invert ? GL_TRUE : GL_FALSE);
}

void GLESBackend::primitiveRestart(uint32_t index) {
    if (lib_->glPrimitiveRestartIndex)
        lib_->glPrimitiveRestartIndex(static_cast<GLuint>(index));
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

// GLES has no polygon mode (glPolygonMode), sample mask (glSampleMaski), or
// minimum sample shading (glMinSampleShading). These are recorded as applied but
// not forwarded to the driver, matching the honest "Unsupported" capability.
void GLESBackend::polygonMode(uint32_t, uint32_t) {}
void GLESBackend::sampleMaski(uint32_t, uint32_t) {}
void GLESBackend::minSampleShading(float) {}
void GLESBackend::provokingVertex(uint32_t) {}
void GLESBackend::clampColor(uint32_t, uint32_t) {}

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

void GLESBackend::multiDrawArrays(uint32_t mode, const int32_t* firsts,
                                  const int32_t* counts, int32_t drawcount) {
    if (lib_->glMultiDrawArrays)
        lib_->glMultiDrawArrays(
            mode, firsts, counts,
            static_cast<GLsizei>(drawcount));
}

void GLESBackend::multiDrawElements(uint32_t mode, const int32_t* counts,
                                    uint32_t type, const intptr_t* indices,
                                    int32_t drawcount) {
    if (lib_->glMultiDrawElements) {
        // Reinterpret the frontend intptr_t* as the native const void* const*.
        lib_->glMultiDrawElements(
            mode, counts, type,
            reinterpret_cast<const void* const*>(indices),
            static_cast<GLsizei>(drawcount));
    }
}

void GLESBackend::drawRangeElements(uint32_t mode, uint32_t start, uint32_t end,
                                    int32_t count, uint32_t type,
                                    intptr_t indices) {
    if (lib_->glDrawRangeElements)
        lib_->glDrawRangeElements(mode, start, end, count, type,
                                 reinterpret_cast<const void*>(indices));
}

 void GLESBackend::drawElementsBaseVertex(uint32_t mode, int32_t count,
                                          uint32_t type, intptr_t indices,
                                          int32_t basevertex) {
     if (lib_->glDrawElementsBaseVertex)
         lib_->glDrawElementsBaseVertex(mode, count, type,
                                       reinterpret_cast<const void*>(indices),
                                       static_cast<GLint>(basevertex));
 }
 
 void GLESBackend::drawArraysIndirect(uint32_t mode, const void* indirect) {
     if (lib_->glDrawArraysIndirect)
         lib_->glDrawArraysIndirect(mode, indirect);
 }
 
 void GLESBackend::drawElementsIndirect(uint32_t mode, uint32_t type,
                                        const void* indirect) {
     if (lib_->glDrawElementsIndirect)
         lib_->glDrawElementsIndirect(mode, type, indirect);
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
