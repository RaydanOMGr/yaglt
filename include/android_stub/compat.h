#ifndef ANDROID_STUB_COMPAT_H
#define ANDROID_STUB_COMPAT_H

// Allows Mesa's android_stub headers to compile with a non-Android host
// compiler (g++/clang on Linux). The annotations are no-ops outside the
// Android NDK toolchain. Force-include this header (or define the macros)
// when compiling any translation unit that pulls in android_stub headers.

#ifndef _Nonnull
#define _Nonnull
#endif
#ifndef _Nullable
#define _Nullable
#endif
#ifndef _Null_unspecified
#define _Null_unspecified
#endif

#ifndef __INTRODUCED_IN
#define __INTRODUCED_IN(...)
#endif
#ifndef __INTRODUCED_IN_32
#define __INTRODUCED_IN_32(...)
#endif
#ifndef __DEPRECATED_IN
#define __DEPRECATED_IN(...)
#endif

#endif // ANDROID_STUB_COMPAT_H
