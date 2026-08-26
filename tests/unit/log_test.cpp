#include "glcompat/core/log.hpp"
#include "glcompat/core/capabilities.hpp"
#include "glcompat/core/capabilities_table.hpp"

#include <cstdio>
#include <string>

#include "test_framework.hpp"

using namespace glcompat;

namespace {
// Capture the logger's FILE* output into a std::string via open_memstream.
struct MemStream {
    char* buf = nullptr;
    size_t sz = 0;
    FILE* f = nullptr;
    MemStream() { f = open_memstream(&buf, &sz); }
    ~MemStream() {
        if (f) fclose(f);
        free(buf);
    }
    std::string str() {
        fflush(f);
        return buf ? std::string(buf) : std::string();
    }
};
} // namespace

TEST_CASE("log: disabled category/level emits nothing") {
    auto& l = Logger::instance();
    MemStream ms;
    l.setStream(ms.f);
    l.setLevel(LogLevel::Error);

    log(LogCategory::Core, LogLevel::Info) << "should not appear";
    EXPECT_EQ(ms.str(), std::string());

    log(LogCategory::Core, LogLevel::Error) << "boom";
    EXPECT_NE(ms.str().find("boom"), std::string::npos);

    l.setLevel(LogLevel::Info); // restore default
}

TEST_CASE("log: per-category filtering") {
    auto& l = Logger::instance();
    MemStream ms;
    l.setStream(ms.f);
    l.setLevel(LogLevel::Debug);
    for (int c = 0; c < 9; ++c)
        l.enableCategory(static_cast<LogCategory>(c), true);
    l.enableCategory(LogCategory::Shader, false);

    EXPECT_FALSE(l.enabled(LogCategory::Shader, LogLevel::Debug));
    EXPECT_TRUE(l.enabled(LogCategory::Core, LogLevel::Debug));

    log(LogCategory::Shader, LogLevel::Debug) << "hidden";
    EXPECT_EQ(ms.str(), std::string());
    log(LogCategory::Core, LogLevel::Debug) << "visible";
    EXPECT_NE(ms.str().find("visible"), std::string::npos);

    for (int c = 0; c < 9; ++c)
        l.enableCategory(static_cast<LogCategory>(c), true);
    l.setLevel(LogLevel::Info); // restore default
}

TEST_CASE("log: category and level names") {
    EXPECT_EQ(std::string(logCategoryName(LogCategory::Emulation)),
              std::string("EMULATION"));
    EXPECT_EQ(std::string(logLevelName(LogLevel::Warn)), std::string("WARN"));
}

TEST_CASE("log: capability report classifies features") {
    CapabilityTable table;
    table.set(Feature::BufferObjects, FeatureSupport::Native);
    table.set(Feature::GeometryShaders, FeatureSupport::Unsupported);
    table.set(Feature::ProgramPipelines, FeatureSupport::Emulated);

    auto& l = Logger::instance();
    MemStream ms;
    l.setStream(ms.f);
    l.setLevel(LogLevel::Debug); // report() emits at Debug
    table.report();
    std::string out = ms.str();
    EXPECT_NE(out.find("BufferObjects -> Native"), std::string::npos);
    EXPECT_NE(out.find("GeometryShaders -> Unsupported"), std::string::npos);
    EXPECT_NE(out.find("ProgramPipelines -> Emulated"), std::string::npos);
    EXPECT_NE(out.find("capability summary"), std::string::npos);
    EXPECT_NE(out.find("emulated=1"), std::string::npos);

    l.setLevel(LogLevel::Info); // restore default
}
