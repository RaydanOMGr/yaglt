// Real system libEGL loader for the YAGLT libEGL drop-in shim.
//
// The shim is built as libEGL.so and dropped ahead of the system driver on the
// library search path. At load time it opens the *real* system libEGL (via
// dlopen, path overridable with YAGLT_HOST_EGL) and resolves the EGL entry
// points it forwards. Every EGL call the application makes is then redirected
// to that real driver; the GL calls are routed to YAGLT's frontend instead.
//
// This mirrors LTW's libEGL wrapper trick: be both the API surface the app
// links against and the dispatcher to the real driver.

#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <dlfcn.h>

// EGLDeviceEXT is not present in every vendored eglext.h; it is an opaque
// pointer on every real implementation, so a local typedef is ABI-safe.
#ifndef EGL_EGLEXT_LEGACY
typedef void* EGLDeviceEXT;
#endif
// EGLAttrib (EGL 1.5 core) is frequently only present as EGLAttribKHR in
// vendored headers; alias it so the function-list types resolve.
#ifndef EGL_EGLEXT_LEGACY
typedef EGLAttribKHR EGLAttrib;
#endif

namespace yaglt_shim {

struct HostEgl {
#define X(NAME, RET, ARGS, PARAMS, DEF, FWD) RET (*NAME) ARGS = nullptr;
#include "egl_func_list.h"
#undef X
    void* handle = nullptr;
};

// Load the real system libEGL from `path` and resolve every forwarded entry
// point. Returns false if the library cannot be opened (the shim then fails
// honestly: EGL calls return safe defaults and GL calls report no context).
bool loadHostEgl(HostEgl& e, const char* path);

} // namespace yaglt_shim
