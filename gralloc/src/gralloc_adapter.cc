#include "gralloc_adapter.h"

#include <hardware/gralloc.h>

#include <dlfcn.h>
#include <memory>

#ifdef __HYBRIS__
#include <hybris/dlfcn/dlfcn.h>
#define dlopen hybris_dlopen
#define dlsym hybris_dlsym
#define dlclose hybris_dlclose
#endif

#include "gralloc_libharware.h"
#include "gralloc_nativewindow.h"

namespace {
auto& loader_ = gralloc_adapter_t::loader::getInstance();
}

gralloc_loader& gralloc_loader::getInstance()
{
    static gralloc_loader loader{};
    return loader;
};

gralloc_loader::loader()
{
    gralloc_module = nullptr;

    bool try_libnativewindow = true;
    if (getenv("ALWAYS_USE_LIBHARDWARE") || android_get_device_api_level() < 29)
    {
        try_libnativewindow = false;
    }

    if (try_libnativewindow &&
        (nativewindow_handle = dlopen("libnativewindow.so", RTLD_NOW)) &&
        dlsym(nativewindow_handle, "AHardwareBuffer_createFromHandle"))
    {
        adapter = std::make_shared<gralloc_nativewindow>(nativewindow_handle);
        backend = backend_type::gralloc_nativewindow;
    }
    else if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &gralloc_module) == 0)
    {
        adapter = std::make_shared<gralloc_libhareware>(gralloc_module);
        backend = backend_type::gralloc_libhardware;
    }
    else
    {
        adapter = nullptr;
        backend = backend_type::gralloc_none;
    }

    if (backend != backend_type::gralloc_nativewindow)
    {
        if (nativewindow_handle)
        {
            dlclose(nativewindow_handle);
            nativewindow_handle = nullptr;
        }
    }
}

gralloc_loader::~loader()
{
    adapter = nullptr;
}

std::shared_ptr<gralloc_adapter_t> gralloc_loader::get_adapter() const
{
    return adapter;
}

gralloc_buffer::~buffer()
{
    handle = nullptr;
}

#define GETVPTRFUNC(vptr, func)                                                \
    vptr.func = reinterpret_cast<decltype(vptr.func)>(dlsym(handle, #func))

gralloc_adapter_t::cutils_loader::cutils_loader() noexcept :
    handle(dlopen("libcutils.so", RTLD_NOW))
{
    GETVPTRFUNC(vptr, native_handle_close);
    GETVPTRFUNC(vptr, native_handle_init);
    GETVPTRFUNC(vptr, native_handle_create);
    GETVPTRFUNC(vptr, native_handle_clone);
    GETVPTRFUNC(vptr, native_handle_delete);
}
gralloc_adapter_t::cutils_loader::~cutils_loader() noexcept
{
    memset(&vptr, 0, sizeof(vptr));
    if (!handle)
        return;
    dlclose(handle);
}

#undef GETVPTRFUNC
