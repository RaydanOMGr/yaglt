#pragma once

// Minimal self-contained test framework for YAGLT.
// No external dependencies; compiles under a standard C++17 toolchain so the
// headless Linux CI environment needs nothing beyond a compiler and CMake.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace yaglt {
namespace test {
namespace detail {

inline std::string toStr(const std::string& v) { return v; }
inline std::string toStr(const char* v) { return std::string(v); }
inline std::string toStr(std::nullptr_t) { return "nullptr"; }

template <typename T>
inline std::string toStr(T* v) {
    return "0x" + std::to_string(reinterpret_cast<std::uintptr_t>(v));
}

template <typename T>
inline std::string toStr(T v) {
    if constexpr (std::is_enum_v<T>)
        return std::to_string(static_cast<long long>(v));
    else
        return std::to_string(v);
}

} // namespace detail
} // namespace test
} // namespace yaglt

namespace yaglt {
namespace test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

// C++17 inline globals: guaranteed single instance shared across all TUs.
inline std::vector<TestCase> g_registry;
inline int g_failures = 0;

inline void registerTest(const std::string& name, std::function<void()> fn) {
    g_registry.push_back({name, std::move(fn)});
}

inline void fail(const char* file, int line, const std::string& expr,
                 const std::string& extra) {
    ++g_failures;
    std::fprintf(stderr, "  FAIL %s:%d: %s%s\n", file, line, expr.c_str(),
                 extra.empty() ? "" : (" | " + extra).c_str());
}

inline int runAll() {
    const char* filter = std::getenv("YAGLT_TEST_FILTER");
    int passed = 0;
    for (auto& tc : g_registry) {
        if (filter && std::strstr(tc.name.c_str(), filter) == nullptr) continue;
        std::printf("[ RUN ] %s\n", tc.name.c_str());
        try {
            tc.fn();
            ++passed;
            std::printf("[ PASS ] %s\n", tc.name.c_str());
        } catch (const std::exception& e) {
            ++g_failures;
            std::fprintf(stderr, "  FAIL %s: threw %s\n", tc.name.c_str(), e.what());
        } catch (...) {
            ++g_failures;
            std::fprintf(stderr, "  FAIL %s: threw unknown\n", tc.name.c_str());
        }
    }
    int total = static_cast<int>(g_registry.size());
    std::printf("\n%d/%d tests passed, %d failed\n", passed, total, g_failures);
    return g_failures == 0 ? 0 : 1;
}

} // namespace test
} // namespace yaglt

#define YAGLT_TEST_CONCAT_(a, b) a##b
#define YAGLT_TEST_CONCAT(a, b) YAGLT_TEST_CONCAT_(a, b)

// Register a test case. Body uses EXPECT_* macros.
// The registrar is a file-static function marked constructor; it runs once at
// load. Everything is file-static (internal linkage) so the __LINE__-based
// names never collide across translation units (no ODR issues).
#define TEST_CASE(name)                                                        \
    static void YAGLT_TEST_CONCAT(yaglt_test_, __LINE__)();                    \
    static void YAGLT_TEST_CONCAT(yaglt_reg_, __LINE__)(void)                  \
        __attribute__((constructor));                                         \
    static void YAGLT_TEST_CONCAT(yaglt_reg_, __LINE__)(void) {                \
        ::yaglt::test::registerTest(name, &YAGLT_TEST_CONCAT(yaglt_test_, __LINE__)); \
    }                                                                         \
    static void YAGLT_TEST_CONCAT(yaglt_test_, __LINE__)()

#define EXPECT_TRUE(expr)                                                      \
    do {                                                                       \
        if (!(expr))                                                           \
            ::yaglt::test::fail(__FILE__, __LINE__, #expr, "");                \
    } while (0)

#define EXPECT_FALSE(expr)                                                     \
    do {                                                                       \
        if ((expr))                                                            \
            ::yaglt::test::fail(__FILE__, __LINE__, #expr, "expected false");  \
    } while (0)

#define EXPECT_EQ(a, b)                                                        \
    do {                                                                       \
        auto _va = (a);                                                        \
        auto _vb = (b);                                                        \
        if (!(_va == _vb))                                                     \
            ::yaglt::test::fail(__FILE__, __LINE__, #a " == " #b,              \
                                ::yaglt::test::detail::toStr(_va) + " vs " +   \
                                ::yaglt::test::detail::toStr(_vb)); \
    } while (0)

#define EXPECT_NE(a, b)                                                        \
    do {                                                                       \
        auto _va = (a);                                                        \
        auto _vb = (b);                                                        \
        if (!(_va != _vb))                                                     \
            ::yaglt::test::fail(__FILE__, __LINE__, #a " != " #b, "");         \
    } while (0)
