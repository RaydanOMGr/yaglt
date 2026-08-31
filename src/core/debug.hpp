#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>

// Lightweight debug instrumentation for YAGLT.
//
// Enabled at runtime by setting the environment variable YAGLT_DEBUG to any
// non-empty value (e.g. `YAGLT_DEBUG=1 ./app`), or unconditionally at compile
// time by defining the YAGLT_DEBUG preprocessor macro. When disabled the macros
// expand to no-ops and emit no code, so there is zero runtime cost in a normal
// build.
//
// YAGLT_DEBUG(...)      printf-style log line prefixed with [YAGLT-DEBUG].
// YAGLT_DEBUG_DUMP(n,s) write buffer `s` to /tmp/yaglt_<n> for offline inspection
//                       (used for dumping shader sources before/after translation).

namespace glcompat {
inline bool yagltDebugEnabled() {
#ifdef YAGLT_DEBUG
    return true;
#else
    static const bool on = []() {
        const char* v = std::getenv("YAGLT_DEBUG");
        return v != nullptr && v[0] != '\0';
    }();
    return on;
#endif
}
} // namespace glcompat

#define YAGLT_DEBUG(...)                                                     \
    do {                                                                     \
        if (glcompat::yagltDebugEnabled()) {                                 \
            std::fprintf(stderr, "[YAGLT-DEBUG] " __VA_ARGS__);              \
            std::fprintf(stderr, "\n");                                      \
        }                                                                    \
    } while (0)

#define YAGLT_DEBUG_DUMP(name, src)                                          \
    do {                                                                     \
        if (glcompat::yagltDebugEnabled()) {                                 \
            std::string _yaglt_path = std::string("/tmp/yaglt_") + (name);   \
            FILE* _yaglt_f = std::fopen(_yaglt_path.c_str(), "wb");          \
            if (_yaglt_f) {                                                  \
                std::fwrite((src).data(), 1, (src).size(), _yaglt_f);        \
                std::fclose(_yaglt_f);                                       \
                YAGLT_DEBUG("dumped %s (%zu bytes) -> %s",                   \
                            static_cast<const char*>(name), (src).size(),    \
                            _yaglt_path.c_str());                            \
            }                                                                \
        }                                                                    \
    } while (0)
