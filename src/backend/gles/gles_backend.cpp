#include "glcompat/backend/gles/gles_backend.hpp"

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

void GLESBackend::blendFunc(GLenum sfactor, GLenum dfactor) {
    if (lib_->glBlendFunc) lib_->glBlendFunc(sfactor, dfactor);
}

void GLESBackend::blendEquation(GLenum mode) {
    if (lib_->glBlendEquation) lib_->glBlendEquation(mode);
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

void GLESBackend::cullFace(GLenum mode) {
    if (lib_->glCullFace) lib_->glCullFace(mode);
}

void GLESBackend::frontFace(GLenum mode) {
    if (lib_->glFrontFace) lib_->glFrontFace(mode);
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

} // namespace glcompat
