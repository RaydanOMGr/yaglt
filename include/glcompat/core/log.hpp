#pragma once

#include <cstdio>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace glcompat {

// Structured logging categories for debugging translation problems (SPEC §20).
enum class LogCategory {
    Core,       // frontend core / dispatch
    State,      // pipeline state tracking + flush
    Resource,   // object / resource lifecycle
    Shader,     // shader translation + compilation
    Backend,    // backend selection / init
    GLES,       // OpenGL ES backend specifics
    Vulkan,     // future Vulkan backend
    Platform,   // platform capabilities (Android/Linux)
    Emulation   // emulation path selection / fallbacks
};

// Severity levels, ascending.
enum class LogLevel { Debug, Info, Warn, Error };

const char* logCategoryName(LogCategory c);
const char* logLevelName(LogLevel l);

// Process-wide structured logger. Output is configurable (stream, minimum
// level, per-category enable) and defaults to stderr at the Info level so the
// release configuration does not spam (SPEC §20). A Debug build or the
// YAGLT_LOG_LEVEL / YAGLT_LOG_CATS environment variables raise the detail.
class Logger {
public:
    static Logger& instance();

    // Minimum level that will be emitted (messages below are dropped).
    void setLevel(LogLevel min) { minLevel_ = min; }
    LogLevel level() const { return minLevel_; }

    // Enable/disable a single category. Categories default to enabled.
    void enableCategory(LogCategory c, bool on = true);
    bool categoryEnabled(LogCategory c) const;

    // Redirect output. Pass nullptr to silence all logging.
    void setStream(FILE* f) { stream_ = f; }

    // True when a message with this category/level would actually be written.
    bool enabled(LogCategory c, LogLevel l) const;

    // Emit a fully-formed line (category + level + message + newline).
    void log(LogCategory c, LogLevel l, const std::string& msg);

    // Read back the last raw line written (used by tests; empty if silenced).
    const std::string& lastLine() const { return lastLine_; }

private:
    Logger();

    FILE* stream_ = stderr;
    LogLevel minLevel_ = LogLevel::Info;
    bool cats_[9] = {true, true, true, true, true, true, true, true, true};
    std::string lastLine_;
    mutable std::mutex mtx_;
};

// Streaming log proxy. Usage: `log(Category, Level) << "x=" << value;`
// The line is emitted on destruction. If the category/level is disabled the
// message is never built or written (SPEC §20: no spam in release).
class LogStream {
public:
    LogStream(Logger& l, LogCategory c, LogLevel lv)
        : logger_(l), cat_(c), lvl_(lv), active_(l.enabled(c, lv)) {}
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    ~LogStream() {
        if (active_) logger_.log(cat_, lvl_, ss_.str());
    }

    template <typename T>
    LogStream& operator<<(const T& v) {
        if (active_) ss_ << v;
        return *this;
    }

private:
    Logger& logger_;
    LogCategory cat_;
    LogLevel lvl_;
    bool active_;
    std::ostringstream ss_;
};

inline LogStream log(LogCategory c, LogLevel l) {
    return LogStream(Logger::instance(), c, l);
}

} // namespace glcompat
