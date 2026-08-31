// X-macro list of EGL entry points the libEGL drop-in shim forwards to the real
// system driver.
//
// Fields: NAME, RETURN, (ARG_TYPES), (CALL_ARGS), DEFAULT, AUTO_FORWARD
//   AUTO_FORWARD=1 -> generate a plain forwarding wrapper.
//   AUTO_FORWARD=0 -> resolved from the host but implemented manually in
//                     egl_shim.cpp (eglGetProcAddress / the context lifecycle).
//
// KHR-core aliases (eglCreateImage/eglCreateSync/...) are additionally resolved
// in host_egl.cpp so a host exposing only the KHR variant still works.

X(eglGetError, EGLint, (void), (), 0, 1)
X(eglGetDisplay, EGLDisplay, (EGLNativeDisplayType display_id), (display_id), EGL_NO_DISPLAY, 1)
X(eglGetPlatformDisplay, EGLDisplay, (EGLenum platform, void* native_display, const EGLAttrib* attrib_list), (platform, native_display, attrib_list), EGL_NO_DISPLAY, 1)
X(eglGetPlatformDisplayEXT, EGLDisplay, (EGLenum platform, void* native_display, const EGLAttrib* attrib_list), (platform, native_display, attrib_list), EGL_NO_DISPLAY, 1)
X(eglInitialize, EGLBoolean, (EGLDisplay dpy, EGLint* major, EGLint* minor), (dpy, major, minor), EGL_FALSE, 1)
X(eglTerminate, EGLBoolean, (EGLDisplay dpy), (dpy), EGL_FALSE, 1)
X(eglQueryString, const char*, (EGLDisplay dpy, EGLint name), (dpy, name), nullptr, 1)
X(eglGetConfigs, EGLBoolean, (EGLDisplay dpy, EGLConfig* configs, EGLint config_size, EGLint* num_config), (dpy, configs, config_size, num_config), EGL_FALSE, 1)
X(eglChooseConfig, EGLBoolean, (EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config), (dpy, attrib_list, configs, config_size, num_config), EGL_FALSE, 0)
X(eglGetConfigAttrib, EGLBoolean, (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value), (dpy, config, attribute, value), EGL_FALSE, 1)
X(eglCreateContext, EGLContext, (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list), (dpy, config, share_context, attrib_list), EGL_NO_CONTEXT, 0)
X(eglDestroyContext, EGLBoolean, (EGLDisplay dpy, EGLContext ctx), (dpy, ctx), EGL_FALSE, 0)
X(eglMakeCurrent, EGLBoolean, (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx), (dpy, draw, read, ctx), EGL_FALSE, 0)
X(eglGetCurrentContext, EGLContext, (void), (), EGL_NO_CONTEXT, 1)
X(eglGetCurrentDisplay, EGLDisplay, (void), (), EGL_NO_DISPLAY, 1)
X(eglGetCurrentSurface, EGLSurface, (EGLint readdraw), (readdraw), EGL_NO_SURFACE, 1)
X(eglGetProcAddress, void*, (const char* procname), (procname), nullptr, 0)
X(eglSwapBuffers, EGLBoolean, (EGLDisplay dpy, EGLSurface surface), (dpy, surface), EGL_FALSE, 1)
X(eglSwapInterval, EGLBoolean, (EGLDisplay dpy, EGLint interval), (dpy, interval), EGL_FALSE, 1)
X(eglCreateWindowSurface, EGLSurface, (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint* attrib_list), (dpy, config, win, attrib_list), EGL_NO_SURFACE, 1)
X(eglCreatePbufferSurface, EGLSurface, (EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list), (dpy, config, attrib_list), EGL_NO_SURFACE, 1)
X(eglCreatePixmapSurface, EGLSurface, (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint* attrib_list), (dpy, config, pixmap, attrib_list), EGL_NO_SURFACE, 1)
X(eglDestroySurface, EGLBoolean, (EGLDisplay dpy, EGLSurface surface), (dpy, surface), EGL_FALSE, 1)
X(eglSurfaceAttrib, EGLBoolean, (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value), (dpy, surface, attribute, value), EGL_FALSE, 1)
X(eglQuerySurface, EGLBoolean, (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value), (dpy, surface, attribute, value), EGL_FALSE, 1)
X(eglBindAPI, EGLBoolean, (EGLenum api), (api), EGL_FALSE, 0)
X(eglQueryAPI, EGLenum, (void), (), 0, 1)
X(eglWaitClient, EGLBoolean, (void), (), EGL_FALSE, 1)
X(eglWaitNative, EGLBoolean, (EGLint engine), (engine), EGL_FALSE, 1)
X(eglReleaseThread, EGLBoolean, (void), (), EGL_FALSE, 1)
X(eglCopyBuffers, EGLBoolean, (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target), (dpy, surface, target), EGL_FALSE, 1)
X(eglCreateImage, EGLImage, (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib* attrib_list), (dpy, ctx, target, buffer, attrib_list), EGL_NO_IMAGE, 1)
X(eglDestroyImage, EGLBoolean, (EGLDisplay dpy, EGLImage image), (dpy, image), EGL_FALSE, 1)
X(eglCreateSync, EGLSync, (EGLDisplay dpy, EGLenum type, const EGLAttrib* attrib_list), (dpy, type, attrib_list), EGL_NO_SYNC, 1)
X(eglDestroySync, EGLBoolean, (EGLDisplay dpy, EGLSync sync), (dpy, sync), EGL_FALSE, 1)
X(eglClientWaitSync, EGLint, (EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout), (dpy, sync, flags, timeout), 0, 1)
X(eglGetSyncAttrib, EGLBoolean, (EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib* value), (dpy, sync, attribute, value), EGL_FALSE, 1)
X(eglWaitSync, EGLBoolean, (EGLDisplay dpy, EGLSync sync, EGLint flags), (dpy, sync, flags), EGL_FALSE, 1)
X(eglCreatePlatformWindowSurface, EGLSurface, (EGLDisplay dpy, EGLConfig config, void* native_window, const EGLAttrib* attrib_list), (dpy, config, native_window, attrib_list), EGL_NO_SURFACE, 1)
X(eglCreatePlatformPixmapSurface, EGLSurface, (EGLDisplay dpy, EGLConfig config, void* native_pixmap, const EGLAttrib* attrib_list), (dpy, config, native_pixmap, attrib_list), EGL_NO_SURFACE, 1)
X(eglQueryDevicesEXT, EGLBoolean, (EGLint max_devices, EGLDeviceEXT* devices, EGLint* num_devices), (max_devices, devices, num_devices), EGL_FALSE, 1)
X(eglQueryDeviceAttribEXT, EGLBoolean, (EGLDeviceEXT device, EGLint attribute, EGLAttrib* value), (device, attribute, value), EGL_FALSE, 1)
X(eglQueryDeviceStringEXT, const char*, (EGLDeviceEXT device, EGLint name), (device, name), nullptr, 1)
X(eglQueryDisplayAttribEXT, EGLBoolean, (EGLDisplay dpy, EGLint attribute, EGLAttrib* value), (dpy, attribute, value), EGL_FALSE, 1)
