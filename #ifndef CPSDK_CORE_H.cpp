#ifndef CPSDK_CORE_H
#define CPSDK_CORE_H

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <functional>

#if defined(_WIN32) || defined(_WIN64)
# define CPSDK_WINDOWS 1
# include <windows.h>
#elif defined(__APPLE__)
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#  define CPSDK_IOS 1
# else
#  define CPSDK_MACOS 1
# endif
#else
# define CPSDK_POSIX 1
# include <unistd.h>
#endif

namespace cpsdk {

enum class Result : int32_t {
    Ok = 0,
    InvalidArgument = -1,
    NotSupported = -2,
    OutOfMemory = -3,
    PlatformFailure = -4,
};

struct Version {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;

    constexpr Version(uint16_t maj = 1, uint16_t min = 0, uint16_t pat = 0) noexcept
      : major(maj), minor(min), patch(pat) {}

    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

using LogHandler = std::function<void(LogLevel level, const std::string& message)>;

struct Config {
    Version version;
    LogLevel logLevel;
    LogHandler logHandler;

    Config() noexcept
      : version(), logLevel(LogLevel::Info), logHandler(nullptr) {}
};

class Core {
public:
    static Result initialize(const Config& config = Config()) noexcept {
        if (s_initialized) {
            return Result::Ok;
        }

        s_config = config;
        s_initialized = true;
        log(LogLevel::Info, "cpsdk initialized, version %s", s_config.version.toString().c_str());
        return Result::Ok;
    }

    static Result shutdown() noexcept {
        if (!s_initialized) {
            return Result::Ok;
        }

        log(LogLevel::Info, "cpsdk shutting down");
        s_initialized = false;
        return Result::Ok;
    }

    static bool isInitialized() noexcept {
        return s_initialized;
    }

    static Version version() noexcept {
        return s_config.version;
    }

    static void log(LogLevel level, const char* format, ...) noexcept {
        if (!s_initialized && level < LogLevel::Warn) {
            return;
        }

        char buffer[1024];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        std::string message(buffer);

        if (s_config.logHandler) {
            s_config.logHandler(level, message);
            return;
        }

        defaultLog(level, message);
    }

private:
    static inline Config s_config;
    static inline bool s_initialized = false;

    static void defaultLog(LogLevel level, const std::string& message) noexcept {
        const char* prefix = "[INFO] ";
        switch (level) {
        case LogLevel::Debug: prefix = "[DEBUG] "; break;
        case LogLevel::Info: prefix = "[INFO] "; break;
        case LogLevel::Warn: prefix = "[WARN] "; break;
        case LogLevel::Error: prefix = "[ERROR] "; break;
        }

#if defined(CPSDK_WINDOWS)
        const std::string output = prefix + message + "\n";
        OutputDebugStringA(output.c_str());
        std::fputs(output.c_str(), stdout);
#else
        std::fprintf(stdout, "%s%s\n", prefix, message.c_str());
#endif
    }
};

inline Result platformSleep(uint32_t milliseconds) noexcept {
#if defined(CPSDK_WINDOWS)
    Sleep(milliseconds);
    return Result::Ok;
#elif defined(CPSDK_POSIX) || defined(CPSDK_MACOS) || defined(CPSDK_IOS)
    usleep(static_cast<useconds_t>(milliseconds) * 1000u);
    return Result::Ok;
#else
    return Result::NotSupported;
#endif
}

} // namespace cpsdk

#endif // CPSDK_CORE_H
