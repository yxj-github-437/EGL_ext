#ifndef ANDROID_EGL_DISPLAY_H
#define ANDROID_EGL_DISPLAY_H

#include <EGL/egl.h>
#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>

#include "platform/platform.h"

namespace egl_wrapper {

class egl_display_t {
    const int magic{'_dpy'};
    class shared_locked_view {
        std::shared_lock<std::shared_mutex> terminate_lock;
        std::unique_lock<std::mutex> lock;

        shared_locked_view(std::shared_mutex& terminate_mutex,
                           std::mutex& mutex, egl_display_t* disp) noexcept :
            terminate_lock(terminate_mutex),
            lock(mutex),
            display(disp)
        {
        }

      public:
        shared_locked_view() noexcept { display = nullptr; }
        ~shared_locked_view() noexcept { display = nullptr; }
        egl_display_t* display;
        friend class egl_display_t;
    };

    class locked_view {
        std::unique_lock<std::shared_mutex> terminate_lock;
        std::unique_lock<std::mutex> lock;

        locked_view(std::shared_mutex& terminate_mutex, std::mutex& mutex,
                    egl_display_t* disp) noexcept :
            terminate_lock(terminate_mutex),
            lock(mutex),
            display(disp)
        {
        }

      public:
        locked_view() noexcept { display = nullptr; }
        ~locked_view() noexcept { display = nullptr; }
        egl_display_t* display;
        friend class egl_display_t;
    };

    std::shared_mutex terminate_mutex;
    std::mutex mutex;

  public:
    EGLNativeDisplayType ndpy;
    EGLDisplay dpy;
    EGLint major;
    EGLint minor;

    bool initialized;

    std::unique_ptr<platform_wrapper_t> platform_wrapper;
    struct server_wlegl* wlegl_global;

    struct {
        char const* vendor;
        char const* version;
        char const* clientApi;
        char const* extensions;
    } vendor_queried_strings;

    EGLint Version;    /**< EGL version major*10+minor */
    EGLint ClientAPIs; /**< Bitmask of APIs supported (EGL_xxx_BIT) */

    /* these fields are derived from above */
    std::string VersionString;    /**< EGL_VERSION */
    std::string ClientAPIsString; /**< EGL_CLIENT_APIS */
    std::string ExtensionsString; /**< EGL_EXTENSIONS */

    egl_display_t() = default;
    ~egl_display_t();

    EGLBoolean initialize(EGLint* major, EGLint* minor) noexcept;
    const char* get_version() const noexcept{ return VersionString.c_str(); }
    const char* get_client_apis() const noexcept { return ClientAPIsString.c_str(); }
    const char* get_extensions() const noexcept { return ExtensionsString.c_str(); }
    static const char* get_client_extensions() noexcept;



    shared_locked_view shared_lock() noexcept
    {
        return {terminate_mutex, mutex, this};
    }
    locked_view lock() noexcept { return {terminate_mutex, mutex, this}; }
};

} // namespace egl_wrapper

#endif
