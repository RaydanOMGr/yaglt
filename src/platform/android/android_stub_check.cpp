// Compile-only smoke check that Mesa's android_stub headers build on a
// non-Android host when paired with compat.h. No Android runtime calls are
// made (the NDK libs are absent on host), so this only proves the headers
// are usable. Built only when YAGLT_ANDROID_STUB=ON (see src/CMakeLists.txt).
#include <android/log.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>

namespace {
volatile int kAndroidStubCheck = 1;
} // namespace
