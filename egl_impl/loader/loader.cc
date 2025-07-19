#include "loader.h"
#include "platform/egl_platform_entries.h"

#include <dlfcn.h>
#include <stdlib.h>

#include "logger.h"

using namespace egl_wrapper;

#ifndef SYSTEM_LIB_PATH
#if defined(__LP64__)
#define SYSTEM_LIB_PATH "/system/lib64"
#else
#define SYSTEM_LIB_PATH "/system/lib"
#endif
#endif

namespace {
#undef GL_ENTRY
#undef EGL_ENTRY
#define GL_ENTRY(_r, _api, ...) #_api,
#define EGL_ENTRY(_r, _api, ...) #_api,

// clang-format off
char const * const gl_names[] = {
    #include "entries.in"
    nullptr
};

char const * const gl_names_1[] = {
    #include "entries_gles1.in"
    nullptr
};

char const * const egl_names[] = {
    #include "egl_entries.in"
    nullptr
};

char const * const platform_names[] = {
    #include "platform_entries.in"
    nullptr
};
// clang-format on

#undef GL_ENTRY
#undef EGL_ENTRY

auto& loader = egl_system_t::loader::getInstance();
} // namespace

egl_system_t::loader& egl_system_t::loader::getInstance()
{
    static egl_system_t::loader loader{};
    return loader;
}

egl_system_t::loader::loader() : getProcAddress(nullptr)
{
    // we need system libEGL, but LD_LIBRARY_PATH could make libEGL load failed,
    // so save it and reset when dlopen completed.
    auto restored = systemloader.create_ldenv_restore();

    libEgl = dlopen(SYSTEM_LIB_PATH "/libEGL.so", RTLD_NOW);
    if (!libEgl)
        logger::log_error() << dlerror();
    libGles1 = dlopen(SYSTEM_LIB_PATH "/libGLESv1_CM.so", RTLD_NOW);
    if (!libGles1)
        logger::log_error() << dlerror();
    libGles2 = dlopen(SYSTEM_LIB_PATH "/libGLESv2.so", RTLD_NOW);
    if (!libGles2)
        logger::log_error() << dlerror();

    if (!(libEgl && libGles1 && libGles2))
    {
        logger::log_fatal()
            << "failed load libEGL.so, libGLESv1_CM.so, libGLESv2.so";
        return;
    }

    getProcAddress = (getProcAddressType)dlsym(libEgl, "eglGetProcAddress");
    if (!getProcAddress)
    {
        logger::log_fatal() << "load function eglGetProcAddress failed";
    }

    system = std::shared_ptr<egl_system_t>{new egl_system_t};
    init_libegl_api();
    init_libgles_api();
}

void egl_system_t::loader::init_libegl_api()
{
    auto* egl = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(&system->egl);
    char const* const* api = egl_names;
    while (*api)
    {
        if ((*egl = getProcAddress(*api)) == nullptr)
        {
            logger::log_debug() << "load egl function " << *api << " failed";
        }

        egl++;
        api++;
    }
}

void egl_system_t::loader::init_libgles_api()
{
    auto* gles1 = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &system->hooks[GLESv1_INDEX].gl);
    auto* gles2 = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &system->hooks[GLESv2_INDEX].gl);

    const char* const* api = gl_names;
    const char* const* api_1 = gl_names_1;
    while (*api)
    {
        if (*api_1 && strcmp(*api, *api_1) == 0)
        {
            if ((*gles1 = getProcAddress(*api_1)) == nullptr)
            {
                logger::log_debug()
                    << "load gles1 function " << *api_1 << " failed";
            }
            api_1++;
            gles1++;
        }

        if ((*gles2 = getProcAddress(*api)) == nullptr)
        {
            logger::log_debug() << "load gles2 function " << *api << " failed";
        }
        api++;
        gles2++;
    }
}

egl_system_t::loader::~loader()
{
    getProcAddress = nullptr;

    dlclose(libEgl);
    dlclose(libGles1);
    dlclose(libGles2);
}

egl_system_t::egl_system_t()
{
    fullPlatformImpl(platform);
}

std::shared_ptr<egl_system_t> egl_wrapper::egl_get_system()
{
    return loader.system;
}
