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

void api_load(
    void* dso, char const* const* api, char const* const* ref_api,
    __eglMustCastToProperFunctionPointerType* curr,
    __eglMustCastToProperFunctionPointerType (*getProcAddress)(const char*))
{
    while (*api)
    {
        char const* name = *api;
        if (ref_api)
        {
            char const* ref_name = *ref_api;
            if (strcmp(name, ref_name) != 0)
            {
                *curr++ = nullptr;
                ref_api++;
                continue;
            }
        }

        auto f = (__eglMustCastToProperFunctionPointerType)dlsym(dso, name);
        if (f == nullptr)
        {
            // couldn't find the entry-point, use eglGetProcAddress()
            f = getProcAddress(name);
        }

        if (!f)
        {
            logger::log_debug() << "load function " << name << " failed";
        }

        *curr++ = f;
        api++;
        if (ref_api)
            ref_api++;
    }
}

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
    egl_t* egl = &system->egl;
    auto curr = (__eglMustCastToProperFunctionPointerType*)egl;
    char const* const* api = egl_names;
    while (*api)
    {
        char const* name = *api;
        auto f = (__eglMustCastToProperFunctionPointerType)dlsym(libEgl, name);
        if (f == nullptr)
        {
            // couldn't find the entry-point, use eglGetProcAddress()
            f = getProcAddress(name);
        }
        if (!f)
        {
            logger::log_debug() << "load function " << name << " failed";
        }

        *curr++ = f;
        api++;
    }
}

void egl_system_t::loader::init_libgles_api()
{
    auto gles1 =
        (__eglMustCastToProperFunctionPointerType*)&system->hooks[GLESv1_INDEX]
            .gl;
    auto gles2 =
        (__eglMustCastToProperFunctionPointerType*)&system->hooks[GLESv2_INDEX]
            .gl;

    api_load(libGles1, gl_names_1, gl_names, gles1, getProcAddress);
    api_load(libGles2, gl_names, nullptr, gles2, getProcAddress);
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
    const char* const* entries = platform_names;
    EGLFuncPointer* curr = reinterpret_cast<EGLFuncPointer*>(&platform);
    while (*entries)
    {
        const char* name = *entries;
        EGLFuncPointer f = FindPlatformImplAddr(name);

        if (f == nullptr)
        {
            // If no entry found, update the lookup table: sPlatformImplMap
            logger::log_error()
                << "No entry found in platform lookup table for " << name;
        }

        *curr++ = f;
        entries++;
    }
}

std::shared_ptr<egl_system_t> egl_wrapper::egl_get_system()
{
    return loader.system;
}
