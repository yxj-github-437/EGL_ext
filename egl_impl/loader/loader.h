#ifndef LOADER_H_
#define LOADER_H_

#include "hooks.h"
#include "utils.h"
#include <EGL/egl.h>
#include <memory>

namespace egl_wrapper {
class egl_vendor_t {
    egl_vendor_t();

  public:
    enum { GLESv1_INDEX = 0, GLESv2_INDEX = 1 };
    gl_hooks_t hooks[2] = {};
    gl_hooks_t::gl_ext_t glext = {};
    egl_t egl = {};

    // Functions implemented or redirected by platform libraries
    platform_impl_t platform;

    class loader {
        void* vndk_support_handle;

        decltype(&dlopen) load_library;
        decltype(&dlclose) unload_library;

        bool is_GLES_driver;
        void* dso[3];
        PFNEGLGETPROCADDRESSPROC getProcAddress;

        loader();
        loader(const loader&) = delete;
        loader& operator=(const loader&) = delete;

        void try_load_driver(const std::string& driver_suffix, bool exact);
        void load_egl_driver();

        void load_egl_api();
        void load_gles_api();

      public:
        static loader& getInstance();
        ~loader() noexcept;

        std::unique_ptr<egl_vendor_t> egl_vendor;
    };
};
} // namespace egl_wrapper

#endif // LOADER_H_
