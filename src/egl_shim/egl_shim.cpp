// YAGLT libEGL drop-in shim.
//
// Built as libEGL.so and placed ahead of the system driver on the library
// search path (LD_LIBRARY_PATH / LD_PRELOAD). It is both the EGL API surface the
// application links against and the dispatcher to the real driver, exactly like
// LTW's libEGL wrapper:
//
//   * Every EGL entry point is forwarded to the real system libEGL, which is
//     resolved at load time via dlopen (path overridable with YAGLT_HOST_EGL,
//     default "libEGL.so.1").
//   * gl* entry points are exposed as C symbols (generated in gl_exports.cpp)
//     that route into YAGLT's desktop-GL frontend.
//   * eglGetProcAddress returns YAGLT's gl* for "gl..." names and forwards
//     everything else to the host.
//
// To render through the application's own EGL surface, eglCreateContext /
// eglMakeCurrent are intercepted to own the YAGLT Context lifecycle: a YAGLT
// Context backed by the GLES backend is created per real EGL context and, on
// MakeCurrent, the backend *adopts* the application's real EGL context so YAGLT
// issues native GL on it (see GLESBackend::setAdopt). The shim stays a thin
// dispatch layer and never becomes a new backend.

// NOTE: the frontend GL type/constant layer (gl_api.hpp -> gl_types.hpp) must be
// included BEFORE the GLES backend headers. The vendored GLES3 headers define
// GL_* as preprocessor macros, which would otherwise expand inside gl_types.hpp
// and break its constexpr declarations (and vice-versa). Including the frontend
// first lets GLES3's macros coexist harmlessly.
#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"

#include "host_egl.hpp"

#include "glcompat/backend/gles/gles_backend.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cstring>
#include <dlfcn.h>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace {

yaglt_shim::HostEgl g_host;

// Per real-EGL-context YAGLT state. The backend is adopted onto the app's real
// context, so YAGLT renders into the application's surface.
struct ShimContext {
    EGLDisplay dpy = EGL_NO_DISPLAY;
    glcompat::GLESBackend* backend = nullptr;
    glcompat::Context* ctx = nullptr;
    bool initialized = false;
};

std::mutex g_mapMutex;
std::unordered_map<void*, ShimContext> g_contexts; // keyed by EGLContext (void*)

ShimContext* findContext(EGLContext ctx) {
    auto it = g_contexts.find(reinterpret_cast<void*>(ctx));
    return it == g_contexts.end() ? nullptr : &it->second;
}

void activateContext(EGLContext ctx) {
    if (ctx == EGL_NO_CONTEXT) {
        glcompat::setCurrentContext(nullptr);
        return;
    }
    ShimContext* sc = findContext(ctx);
    if (!sc) {
        // Context not created through our eglCreateContext; nothing to drive.
        glcompat::setCurrentContext(nullptr);
        return;
    }
    if (!sc->initialized) {
        sc->backend->setAdopt(sc->dpy, ctx);
        if (!sc->backend->initialize()) {
            // No driver -> honest failure. GL calls will report no context.
            sc->initialized = false;
        } else {
            sc->initialized = true;
        }
    }
    glcompat::setCurrentContext(sc->ctx);
}

// Translate a desktop-GL context request into a GLES context request on the
// host. The host (e.g. Mesa softpipe) may only expose GLES; YAGLT implements
// desktop GL on top of a GLES backend, so we must obtain a GLES context and
// present it as desktop GL to the application.
static const EGLint* translateContextAttribs(const EGLint* in) {
    std::vector<EGLint> out;
    bool sawMajor = false, sawMinor = false;
    for (const EGLint* p = in; p && *p != EGL_NONE; p += 2) {
        const EGLint key = p[0];
        const EGLint val = p[1];
        switch (key) {
            case EGL_CONTEXT_MAJOR_VERSION_KHR:
                out.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
                out.push_back(3);
                sawMajor = true;
                break;
            case EGL_CONTEXT_MINOR_VERSION_KHR:
                out.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
                out.push_back(0);
                sawMinor = true;
                break;
            case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
                // Not valid for a GLES context; drop it.
                break;
            default:
                out.push_back(key);
                out.push_back(val);
                break;
        }
    }
    if (!sawMajor) {
        out.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
        out.push_back(3);
    }
    if (!sawMinor) {
        out.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
        out.push_back(0);
    }
    out.push_back(EGL_NONE);
    // Persist for the duration of the createContext call.
    static std::vector<EGLint> storage;
    storage = std::move(out);
    return storage.data();
}

} // namespace

extern "C" {

// The CTS (and any desktop-GL app) chooses a config with an OpenGL (or ES1)
// renderable type, but the host only exposes GLES. Remap the renderable type so
// a usable GLES config is returned.
EGLAPIENTRY EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list,
                                       EGLConfig* configs, EGLint config_size,
                                       EGLint* num_config) {
    if (!g_host.eglChooseConfig) return EGL_FALSE;
    static std::vector<EGLint> storage;
    const EGLint* list = attrib_list;
    if (attrib_list) {
        std::vector<EGLint> out;
        for (const EGLint* p = attrib_list; *p != EGL_NONE; p += 2) {
            EGLint key = p[0], val = p[1];
            if (key == EGL_RENDERABLE_TYPE) {
                EGLint mapped = val;
                if (val & EGL_OPENGL_BIT) mapped = (mapped & ~EGL_OPENGL_BIT) | EGL_OPENGL_ES3_BIT;
                if (val & EGL_OPENGL_ES_BIT) mapped = (mapped & ~EGL_OPENGL_ES_BIT) | EGL_OPENGL_ES3_BIT;
                if (mapped == 0) mapped = EGL_OPENGL_ES3_BIT;
                out.push_back(EGL_RENDERABLE_TYPE);
                out.push_back(mapped);
            } else {
                out.push_back(key);
                out.push_back(val);
            }
        }
        out.push_back(EGL_NONE);
        storage = std::move(out);
        list = storage.data();
    }
    return g_host.eglChooseConfig(dpy, list, configs, config_size, num_config);
}

// --- Auto-forwarded EGL entry points -----------------------------------------
// FWD==1 emits a forwarding wrapper; FWD==0 is implemented manually below.
#define X(NAME, RET, ARGS, PARAMS, DEF, FWD) \
    YAGLT_FWD_##FWD(NAME, RET, ARGS, PARAMS, DEF)
#define YAGLT_FWD_1(NAME, RET, ARGS, PARAMS, DEF)                        \
    EGLAPIENTRY RET NAME ARGS {                                          \
        if (!g_host.NAME) return DEF;                                    \
        return g_host.NAME PARAMS;                                       \
    }
#define YAGLT_FWD_0(NAME, RET, ARGS, PARAMS, DEF)
#include "egl_func_list.h"
#undef X
#undef YAGLT_FWD_1
#undef YAGLT_FWD_0

// --- Manual EGL entry points ------------------------------------------------

// The CTS (and any desktop-GL app) binds EGL_OPENGL_API, but the host only
// exposes GLES. Translate the bind so the host creates a GLES context that
// YAGLT drives as desktop GL.
EGLAPIENTRY EGLBoolean eglBindAPI(EGLenum api) {
    if (!g_host.eglBindAPI) return EGL_FALSE;
    if (api == EGL_OPENGL_API) api = EGL_OPENGL_ES_API;
    return g_host.eglBindAPI(api);
}

EGLAPIENTRY EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
                                        EGLContext share_context,
                                        const EGLint* attrib_list) {
    if (!g_host.eglCreateContext) return EGL_NO_CONTEXT;
    // The application asked for a desktop-GL context (EGL_OPENGL_API, possibly
    // with a CORE/COMPAT profile mask). The host only provides GLES, so rewrite
    // the request into a GLES3 context that YAGLT will drive as desktop GL.
    const EGLint* hostAttribs = translateContextAttribs(attrib_list);
    EGLContext realCtx =
        g_host.eglCreateContext(dpy, config, share_context, hostAttribs);
    if (realCtx == EGL_NO_CONTEXT) return realCtx;

    std::lock_guard<std::mutex> lock(g_mapMutex);
    ShimContext sc;
    sc.dpy = dpy;
    sc.backend = new glcompat::GLESBackend();
    sc.ctx = new glcompat::Context(*sc.backend);
    sc.initialized = false;
    g_contexts[reinterpret_cast<void*>(realCtx)] = std::move(sc);
    return realCtx;
}

EGLAPIENTRY EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
    EGLBoolean ok = EGL_TRUE;
    if (g_host.eglDestroyContext) ok = g_host.eglDestroyContext(dpy, ctx);

    std::lock_guard<std::mutex> lock(g_mapMutex);
    auto it = g_contexts.find(reinterpret_cast<void*>(ctx));
    if (it != g_contexts.end()) {
        ShimContext& sc = it->second;
        if (sc.backend) {
            sc.backend->shutdown();
            delete sc.ctx;
            delete sc.backend;
        }
        g_contexts.erase(it);
    }
    return ok;
}

EGLAPIENTRY EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                      EGLSurface read, EGLContext ctx) {
    if (!g_host.eglMakeCurrent) return EGL_FALSE;
    EGLBoolean ok = g_host.eglMakeCurrent(dpy, draw, read, ctx);
    if (!ok) return EGL_FALSE;
    activateContext(ctx);
    return EGL_TRUE;
}

EGLAPIENTRY __eglMustCastToProperFunctionPointerType eglGetProcAddress(const char* procname) {
    if (!procname) return nullptr;
    // Desktop GL (and the GL subset YAGLT implements) is served by YAGLT's
    // generated C wrappers, which are exported by this shim.
    if (strncmp(procname, "gl", 2) == 0) {
        void* fn = dlsym(RTLD_DEFAULT, procname);
        if (fn) return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(fn);
    }
    // EGL extensions and GLES-only entry points fall through to the host.
    if (g_host.eglGetProcAddress)
        return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(
            g_host.eglGetProcAddress(procname));
    return nullptr;
}

// Desktop GL applications often resolve GL functions through GLX even when they
// render on EGL. Route those to the same resolver.
void (*glXGetProcAddress(const GLubyte* name))(void) {
    return reinterpret_cast<void (*)(void)>(
        eglGetProcAddress(reinterpret_cast<const char*>(name)));
}
void (*glXGetProcAddressARB(const GLubyte* name))(void) {
    return glXGetProcAddress(name);
}

} // extern "C"

namespace {

__attribute__((constructor)) void shim_init() {
    const char* path = std::getenv("YAGLT_HOST_EGL");
    if (!path) path = "libEGL.so.1";
    if (!yaglt_shim::loadHostEgl(g_host, path)) {
        std::fprintf(stderr,
                     "[yaglt-shim] WARNING: failed to load host libEGL '%s': %s\n",
                     path, dlerror());
        // No real driver: the shim is inert but must not abort the process.
        // EGL calls return safe defaults; GL calls report no current context.
    }
}

} // namespace
