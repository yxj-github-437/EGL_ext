#include "utils.h"
#include "logger.h"

#ifdef __ANDROID__
#include <unistd.h>
#include <android/log.h>
#define LOG_TAG "EGL"
#endif

#include <time.h>
#include <iostream>
#include <mutex>

namespace {
std::mutex& mutex = []() -> std::mutex& {
    alignas(alignof(std::mutex)) static char buf[sizeof(std::mutex)];
    return *new (buf) std::mutex{};
}();

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
    switch (prio)
    {
    case logger::LOG_VERBOSE:
        return "[V] ";
    case logger::LOG_DEBUG:
        return "[D] ";
    case logger::LOG_INFO:
        return "[I] ";
    case logger::LOG_WARN:
        return "[W] ";
    case logger::LOG_ERROR:
        return "[E] ";
    case logger::LOG_FATAL:
        return "[F] ";
    default:
        return "[-] ";
    }
}

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
