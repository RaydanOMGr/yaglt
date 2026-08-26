#include "glcompat/core/log.hpp"

#include <cstdlib>
#include <cstring>

namespace glcompat {

const char* logCategoryName(LogCategory c) {
    switch (c) {
    case LogCategory::Core: return "CORE";
    case LogCategory::State: return "STATE";
    case LogCategory::Resource: return "RESOURCE";
    case LogCategory::Shader: return "SHADER";
    case LogCategory::Backend: return "BACKEND";
    case LogCategory::GLES: return "GLES";
    case LogCategory::Vulkan: return "VULKAN";
    case LogCategory::Platform: return "PLATFORM";
    case LogCategory::Emulation: return "EMULATION";
    }
    return "?";
}

const char* logLevelName(LogLevel l) {
    switch (l) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "?";
}

static LogLevel parseLevel(const char* s, LogLevel fallback) {
    if (!s) return fallback;
    if (std::strcmp(s, "debug") == 0) return LogLevel::Debug;
    if (std::strcmp(s, "info") == 0) return LogLevel::Info;
    if (std::strcmp(s, "warn") == 0 || std::strcmp(s, "warning") == 0)
        return LogLevel::Warn;
    if (std::strcmp(s, "error") == 0) return LogLevel::Error;
    return fallback;
}

Logger::Logger() {
    const char* lvl = std::getenv("YAGLT_LOG_LEVEL");
    minLevel_ = parseLevel(lvl, LogLevel::Info);

    const char* cats = std::getenv("YAGLT_LOG_CATS");
    if (cats) {
        std::string list(cats);
        bool all = (list == "all" || list == "ALL");
        bool none = (list == "none" || list == "NONE");
        for (int i = 0; i < 9; ++i) cats_[i] = none ? false : all;
        if (!all && !none) {
            // Enable only the named categories.
            for (int i = 0; i < 9; ++i) cats_[i] = false;
            size_t start = 0;
            while (start < list.size()) {
                size_t comma = list.find(',', start);
                std::string tok =
                    list.substr(start, comma == std::string::npos
                                          ? std::string::npos
                                          : comma - start);
                while (!tok.empty() && (tok.front() == ' ' || tok.front() == '\t'))
                    tok.erase(tok.begin());
                while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\t'))
                    tok.pop_back();
                for (int i = 0; i < 9; ++i) {
                    if (tok == logCategoryName(static_cast<LogCategory>(i)))
                        cats_[i] = true;
                }
                if (comma == std::string::npos) break;
                start = comma + 1;
            }
        }
    }
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::enableCategory(LogCategory c, bool on) {
    if (static_cast<int>(c) >= 0 && static_cast<int>(c) < 9)
        cats_[static_cast<int>(c)] = on;
}

bool Logger::categoryEnabled(LogCategory c) const {
    int i = static_cast<int>(c);
    return i >= 0 && i < 9 && cats_[i];
}

bool Logger::enabled(LogCategory c, LogLevel l) const {
    return stream_ != nullptr && l >= minLevel_ && categoryEnabled(c);
}

void Logger::log(LogCategory c, LogLevel l, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (stream_ == nullptr) return;
    if (l < minLevel_ || !categoryEnabled(c)) return;
    std::string line = std::string("[YAGLT ") + logLevelName(l) + " " +
                       logCategoryName(c) + "] " + msg + "\n";
    lastLine_ = line;
    std::fputs(line.c_str(), stream_);
    std::fflush(stream_);
}

} // namespace glcompat
