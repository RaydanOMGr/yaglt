#include "host_egl.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

namespace yaglt_shim {

bool loadHostEgl(HostEgl& e, const char* path) {
    if (e.handle) return true;
    void* h = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    if (!h) return false;
    e.handle = h;

#define X(NAME, RET, ARGS, PARAMS, DEF, FWD) \
    e.NAME = reinterpret_cast<decltype(e.NAME)>(dlsym(h, #NAME));
#include "egl_func_list.h"
#undef X

    // KHR-core aliases: a host may expose only the KHR-suffixed variant of the
    // image/sync entry points. Resolve them so the core-named forwarders still
    // work.
    if (!e.eglCreateImage)
        e.eglCreateImage = reinterpret_cast<decltype(e.eglCreateImage)>(
            dlsym(h, "eglCreateImageKHR"));
    if (!e.eglDestroyImage)
        e.eglDestroyImage = reinterpret_cast<decltype(e.eglDestroyImage)>(
            dlsym(h, "eglDestroyImageKHR"));
    if (!e.eglCreateSync)
        e.eglCreateSync = reinterpret_cast<decltype(e.eglCreateSync)>(
            dlsym(h, "eglCreateSyncKHR"));
    if (!e.eglDestroySync)
        e.eglDestroySync = reinterpret_cast<decltype(e.eglDestroySync)>(
            dlsym(h, "eglDestroySyncKHR"));
    if (!e.eglClientWaitSync)
        e.eglClientWaitSync = reinterpret_cast<decltype(e.eglClientWaitSync)>(
            dlsym(h, "eglClientWaitSyncKHR"));
    if (!e.eglGetSyncAttrib)
        e.eglGetSyncAttrib = reinterpret_cast<decltype(e.eglGetSyncAttrib)>(
            dlsym(h, "eglGetSyncAttribKHR"));
    if (!e.eglWaitSync)
        e.eglWaitSync = reinterpret_cast<decltype(e.eglWaitSync)>(
            dlsym(h, "eglWaitSyncKHR"));

    return true;
}

} // namespace yaglt_shim
