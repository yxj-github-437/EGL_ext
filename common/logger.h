#ifndef LOGGER_H_
#define LOGGER_H_

#include <sstream>

namespace logger {
enum log_priority_t {
    LOG_VERBOSE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
};

class log_t {
    std::stringstream sstream;
    enum log_priority_t prio;
    log_priority_t get_default_level();

  public:
    template <typename T>
    [[gnu::always_inline]]
    log_t& operator<<(T&& arg)
    {
        if (prio >= get_default_level())
            sstream << std::forward<T>(arg);
        return *this;
    }
    log_t(log_priority_t prio) : prio(prio) {}
    log_t(log_t&&) = default;     // default move ctor
    log_t(const log_t&) = delete; // delete copy ctor
    ~log_t();
};

// clang-format off
inline log_t log_verbose() { return log_t{LOG_VERBOSE}; }
inline log_t log_debug()   { return log_t{LOG_DEBUG}; }
inline log_t log_info()    { return log_t{LOG_INFO}; }
inline log_t log_warn()    { return log_t{LOG_WARN}; }
inline log_t log_error()   { return log_t{LOG_ERROR}; }
inline log_t log_fatal()   { return log_t{LOG_FATAL}; }
// clang-format on
} // namespace logger

#endif // LOGGER_H_
