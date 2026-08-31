#!/bin/bash
# Run the VK-GL-CTS GL suite (glcts) against the YAGLT libEGL drop-in shim,
# which translates desktop GL onto the host Mesa GLES (softpipe) backend.
set -u
YAGLT=/home/andre/projects/yaglt
MESA=/home/andre/projects/mesa-26.2.1/install
CTS=/home/andre/projects/VK-GL-CTS
GLCTS_BIN="$CTS/build_glcts/external/openglcts/modules/glcts"

export LD_PRELOAD="$YAGLT/build_tx/src/egl_shim/libEGL.so"
# Provide an unversioned libGL.so that resolves to the YAGLT shim. deqp's
# surfaceless GL platform dlopens "libGL.so" for the *core* GL function table
# (initCoreFunctions) and there is no system libGL.so (only libGL.so.1). Our
# shim's SONAME is libEGL.so, so the loader reuses the already-loaded shim
# instance (shared state) instead of loading a second copy. Without this, core
# GL pointers are NULL and the first glClear* call crashes with SIGSEGV (0x0).
export LD_LIBRARY_PATH="$YAGLT/build_tx/src/egl_shim:$MESA/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBGL_DRIVERS_PATH="$MESA/lib/x86_64-linux-gnu/dri"
export GALLIUM_DRIVER=softpipe
export EGL_PLATFORM=surfaceless
export YAGLT_HOST_EGL=libEGL.so.1

if [ ! -x "$GLCTS_BIN" ]; then
    echo "glcts binary not found at $GLCTS_BIN" >&2
    exit 1
fi

echo "[harness] LD_PRELOAD=$LD_PRELOAD"
echo "[harness] LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
exec "$GLCTS_BIN" "$@"
