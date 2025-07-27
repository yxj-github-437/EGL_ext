#ifndef LOADER_H_
#define LOADER_H_

#include "hooks.h"
#include "utils.h"
#include <EGL/egl.h>
#include <memory>

namespace egl_wrapper {
class egl_system_t {
    egl_system_t();
    friend class loader;

  public:
    enum { GLESv1_INDEX = 0, GLESv2_INDEX = 1 };
    gl_hooks_t hooks[2] = {};
    gl_hooks_t::gl_ext_t glext = {};
    egl_t egl = {};

    // Functions implemented or redirected by platform libraries
    platform_impl_t platform;

    class loader {
        using getProcAddressType =
            __eglMustCastToProperFunctionPointerType (*)(const char*);

        getProcAddressType getProcAddress;
        utils::systemlib_loader systemloader;

        loader();

        void init_libegl_api();
        void init_libgles_api();

      public:
        static loader& getInstance();
        ~loader();

        loader(const loader&) = delete;
        loader& operator=(const loader&) = delete;

        void* libEgl;
        void* libGles1;
        void* libGles2;
        std::shared_ptr<egl_system_t> system;
    };
};

EGLAPI std::shared_ptr<egl_system_t> egl_get_system();
} // namespace egl_wrapper

#endif // LOADER_H_
