#include "utils.h"
#include "logger.h"

#include <unistd.h>
#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "EGL"
#endif

#include <time.h>
#include <iostream>
#include <mutex>

namespace {
std::ostream& format_time(std::ostream& os)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char now[32] = {};
    size_t offset = strftime(now, sizeof(now), "%F %T", localtime(&ts.tv_sec));
    snprintf(now + offset, sizeof(now) - offset, ".%06ld", ts.tv_nsec / 1000);

    os << '[' << now << ']';
    return os;
}

std::ostream& gen_pid_tid(std::ostream& os)
{
    os << "[" << getpid() << "-" << gettid() << "]";
    return os;
}

const char* prio_str(logger::log_priority_t prio)
{
#define LIGHT_GRAY "\e[37m"
#define LIGHT_CYAN "\e[96m"
#define LIGHT_GREEN "\e[92m"
#define LIGHT_YELLOW "\e[93m"
#define LIGHT_RED "\e[91m"
#define BACK_RED "\e[41m"
#define COLOR_NULL "\e[m"
    switch (prio)
    {
    case logger::LOG_VERBOSE:
        return "[" LIGHT_GRAY "V" COLOR_NULL "] ";
    case logger::LOG_DEBUG:
        return "[" LIGHT_CYAN "D" COLOR_NULL "] ";
    case logger::LOG_INFO:
        return "[" LIGHT_GREEN "I" COLOR_NULL "] ";
    case logger::LOG_WARN:
        return "[" LIGHT_YELLOW "W" COLOR_NULL "] ";
    case logger::LOG_ERROR:
        return "[" LIGHT_RED "E" COLOR_NULL "] ";
    case logger::LOG_FATAL:
        return "[" BACK_RED "F" COLOR_NULL "] ";
    default:
        return "[-] ";
    }

#undef LIGHT_GRAY
#undef LIGHT_CYAN
#undef LIGHT_GREEN
#undef LIGHT_YELLOW
#undef LIGHT_RED
#undef BACK_RED
#undef COLOR_NULL
}

#ifdef __ANDROID__
android_LogPriority prio_cast(logger::log_priority_t prio)
{
    switch (prio)
    {
    case logger::LOG_VERBOSE:
        return ANDROID_LOG_VERBOSE;
    case logger::LOG_DEBUG:
        return ANDROID_LOG_DEBUG;
    case logger::LOG_INFO:
        return ANDROID_LOG_INFO;
    case logger::LOG_WARN:
        return ANDROID_LOG_WARN;
    case logger::LOG_ERROR:
        return ANDROID_LOG_ERROR;
    case logger::LOG_FATAL:
        return ANDROID_LOG_FATAL;
    default:
        return ANDROID_LOG_DEFAULT;
    }
}
#endif
} // namespace

logger::log_priority_t logger::log_t::get_default_level()
{
    // clang-format off
    static auto log_level = utils::gen_env_option<log_priority_t>(
        "LOG_LEVEL",
        {{"verbose", logger::LOG_VERBOSE},
         {"debug",   logger::LOG_DEBUG  },
         {"info",    logger::LOG_INFO   },
         {"warning", logger::LOG_WARN   },
         {"error",   logger::LOG_ERROR  }},
        LOG_ERROR);
    // clang-format on
    return log_level;
};

logger::log_t::~log_t()
{
    std::string stream = sstream.str();
    if (stream.empty())
        return;

    static std::mutex& mutex = []() -> std::mutex& {
        alignas(alignof(std::mutex)) static char buf[sizeof(std::mutex)];
        return *new (buf) std::mutex{};
    }();

    std::lock_guard _l{mutex};
#ifdef __ANDROID__
    if (getenv("RUN_IN_ANDROID"))
        __android_log_write(prio_cast(prio), LOG_TAG, stream.c_str());
    else
#endif
    {
        std::ostream& out = (prio < LOG_ERROR) ? std::cout : std::cerr;
        out << format_time << gen_pid_tid << prio_str(prio) << stream
            << std::endl;
    }

    if (prio == LOG_FATAL)
        abort();
}
