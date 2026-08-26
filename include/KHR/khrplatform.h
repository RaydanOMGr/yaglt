#ifndef __khrplatform_h_
#define __khrplatform_h_

/*
** Copyright (c) 2008-2018 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License") or the
** MIT license; this is the canonical Khronos platform header required by
** GL/glext.h, GLES*/gl*.h and EGL/egl.h.
*/

#include <stdint.h>

#define KHRONOS_APIENTRY
#define KHRONOS_APIENTRYP *
#define KHRONOS_APIENTRYP_RENAME KHRONOS_APIENTRYP

#if defined(_WIN64)
#define KHRONOS_PTR_BITS 64
#else
#define KHRONOS_PTR_BITS 32
#endif

typedef int32_t                 khronos_int32_t;
typedef uint32_t                khronos_uint32_t;
typedef int64_t                 khronos_int64_t;
typedef uint64_t                khronos_uint64_t;
typedef unsigned int            khronos_uint_least32_t;
typedef int                     khronos_int_least32_t;
typedef khronos_uint_least32_t  khronos_usize_t;
typedef khronos_int_least32_t   khronos_ssize_t;

#if KHRONOS_PTR_BITS == 64
typedef khronos_uint64_t        khronos_uintptr_t;
typedef khronos_int64_t         khronos_intptr_t;
typedef khronos_uint64_t        khronos_size_t;
#else
typedef khronos_uint32_t        khronos_uintptr_t;
typedef khronos_int32_t         khronos_intptr_t;
typedef khronos_uint32_t        khronos_size_t;
#endif

typedef float                   khronos_float_t;
typedef khronos_int32_t        khronos_time_ns_t;

#define KHRONOS_APICALL
#define KHRONOS_APIENTRY
#define KHRONOS_APIATTRIBUTES

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1
#endif

#ifndef KHRONOS_MAX_ENUM
#define KHRONOS_MAX_ENUM 0x7FFFFFFF
#endif

typedef enum {
    KHRONOS_FALSE = 0,
    KHRONOS_TRUE  = 1,
    KHRONOS_BOOLEAN_ENUM_FORCE_UINT = KHRONOS_MAX_ENUM
} khronos_boolean_enum_t;

#endif /* __khrplatform_h_ */
