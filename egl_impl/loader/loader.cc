#include "loader.h"
#include "platform/egl_platform_entries.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <filesystem>

// defined in <sys/system_properties.h>
#define PROP_VALUE_MAX 92

#include "logger.h"

using namespace egl_wrapper;

#ifndef SYSTEM_LIB_PATH
#if defined(__LP64__)
#define SYSTEM_LIB_PATH "/system/lib64"
#else
#define SYSTEM_LIB_PATH "/system/lib"
#endif
#endif

#ifdef __HYBRIS__
#include <hybris/dlfcn/dlfcn.h>
#define dlopen hybris_dlopen
#define dlsym hybris_dlsym
#define dlclose hybris_dlclose
#define dlerror hybris_dlerror

namespace {
int __system_property_get(const char* __name, char* __value) noexcept
{
    return property_get(__name, __value, "");
}
} // namespace
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

char const * const egl_ext_names[] = {
    #include "egl_ext_entries.in"
    nullptr
};

char const* const gl_ext_name[] = {
    #include "gles_ext_entries.in"
    nullptr
};

char const * const platform_names[] = {
    #include "platform_entries.in"
    nullptr
};
// clang-format on

#undef GL_ENTRY
#undef EGL_ENTRY
} // namespace


namespace {
auto& loader_ = egl_vendor_t::loader::getInstance();
}

egl_vendor_t::egl_vendor_t() = default;

egl_vendor_t::loader& egl_vendor_t::loader::getInstance()
{
    static egl_vendor_t::loader loader{};
    return loader;
}

template <class T>
void load_symbol(void* handle, const char* symbol, T& func) noexcept
{
    if (!handle)
        return;
    func = reinterpret_cast<std::decay_t<T>>(dlsym(handle, symbol));
}

egl_vendor_t::loader::loader() :
    vndk_support_handle(
        dlopen(SYSTEM_LIB_PATH "/libvndksupport.so", RTLD_NOW | RTLD_LOCAL)),
    load_library(dlopen),
    unload_library(dlclose),
    is_GLES_driver(false),
    getProcAddress(nullptr),
    egl_vendor(new egl_vendor_t{})
{
    memset(dso, 0, sizeof(dso));
    load_symbol(vndk_support_handle, "android_load_sphal_library",
                load_library);
    load_symbol(vndk_support_handle, "android_unload_sphal_library",
                unload_library);

    load_egl_driver();
    if (!(is_GLES_driver && dso[0]) &&
        !(!is_GLES_driver && dso[0] && dso[1] && dso[2]))
    {
        logger::log_fatal() << "cannot load vendor driver";
    }

    load_egl_api();
    load_gles_api();
}

egl_vendor_t::loader::~loader() noexcept
{
    for (size_t i = 0; i < std::size(dso); i++)
    {
        if (!dso[i])
            continue;

        /** ??? cannot unload vendor when dlclose libEGL
         * try to do this will coredump
         */
        // unload_library(dso[i]);
        dso[i] = nullptr;
    }

    load_library = nullptr;
    unload_library = nullptr;

    if (vndk_support_handle)
    {
        dlclose(vndk_support_handle);
        vndk_support_handle = nullptr;
    }
}

constexpr const char* PERSIST_DRIVER_SUFFIX_PROPERTY = "persist.graphics.egl";
constexpr const char* RO_DRIVER_SUFFIX_PROPERTY = "ro.hardware.egl";
constexpr const char* RO_BOARD_PLATFORM_PROPERTY = "ro.board.platform";

constexpr const char* HAL_SUBNAME_KEY_PROPERTIES[3] = {
    PERSIST_DRIVER_SUFFIX_PROPERTY,
    RO_DRIVER_SUFFIX_PROPERTY,
    RO_BOARD_PLATFORM_PROPERTY,
};

constexpr const char* const VENDOR_LIB_EGL_DIR =
#if defined(__LP64__)
    "/vendor/lib64/egl";
#else
    "/vendor/lib/egl";
#endif

namespace fs = std::filesystem;
using namespace std::string_literals;

static std::string findLibrary(const std::string& libraryName,
                               const fs::path& searchPath, const bool exact)
{
    if (exact)
    {
        fs::path absolutePath = searchPath / (libraryName + ".so");
        if (fs::exists(absolutePath))
        {
            logger::log_info() << "find library " << absolutePath;
            return absolutePath;
        }
        return {};
    }

    for (auto& entry : fs::directory_iterator{searchPath})
    {
        if (entry.is_directory())
            continue;

        auto path = entry.path();
        auto filename = path.filename();

        if (!filename.has_extension() || filename.extension() != ".so")
            continue;

        if (filename.native() == "libGLES_android.so")
            continue;

        if (filename.native().compare(0, libraryName.length(), libraryName) ==
            0)
        {
            logger::log_info() << "find library " << path;
            return path;
        }
    }

    // Driver not found. gah.
    return {};
}

void egl_vendor_t::loader::try_load_driver(const std::string& driver_suffix,
                                           bool exact)
{
    // clang-format off
    std::vector<const char*> GLES_prefixs = {"libGLES"};
    std::vector<const char*> EGL_prefixs = {"libEGL",
                                            "libGLESv1_CM",
                                            "libGLESv2"};
    // clang-format on
    auto&& try_load_lib = [this](const std::vector<const char*>& prefixs,
                                 const std::string& driver_suffix, bool exact) {
        for (size_t i = 0; i < std::size(prefixs); i++)
        {
            std::string driver_path;
            if (exact)
            {
                driver_path =
                    driver_suffix.empty()
                        ? findLibrary(prefixs[i], VENDOR_LIB_EGL_DIR, exact)
                        : findLibrary(prefixs[i] + "_"s + driver_suffix,
                                      VENDOR_LIB_EGL_DIR, exact);
            }
            else
            {
                driver_path =
                    findLibrary(prefixs[i] + "_"s, VENDOR_LIB_EGL_DIR, exact);
            }

            if (driver_path.empty())
                return;

            dso[i] = load_library(driver_path.c_str(), RTLD_NOW | RTLD_LOCAL);
            logger::log_info()
                << "load " << driver_path << ", handle: " << dso[i];
        }
    };

    is_GLES_driver = true;
    try_load_lib(GLES_prefixs, driver_suffix, exact);
    if (dso[0] && !dso[1] && !dso[2])
        return;

    is_GLES_driver = false;
    try_load_lib(EGL_prefixs, driver_suffix, exact);
}

void egl_vendor_t::loader::load_egl_driver()
{
    if (dso[0])
        return;

    char suffix[PROP_VALUE_MAX] = "";
    if (__system_property_get(RO_DRIVER_SUFFIX_PROPERTY, suffix) > 0)
    {
        try_load_driver(suffix, true);
    }
    if (dso[0])
        return;

    for (auto prop : HAL_SUBNAME_KEY_PROPERTIES)
    {
        if (__system_property_get(prop, suffix) <= 0)
            continue;

        try_load_driver(suffix, true);
    }
    if (dso[0])
        return;

    try_load_driver({}, false);
}

void egl_vendor_t::loader::load_egl_api()
{
    load_symbol(dso[0], "eglGetProcAddress", getProcAddress);
    if (getProcAddress == nullptr)
    {
        logger::log_fatal()
            << "cannot get symbol eglGetProcAddress from vendor lib";
        return;
    }

    logger::log_info() << "eglGetProcAddress " << (void*)getProcAddress;

    auto* egl = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &egl_vendor->egl);
    char const* const* api = egl_names;
    while (*api)
    {
        // For EGL <= 1.4, the eglGetProcAddress only get ext function; the
        // EGL1.5 can get the any function.
        load_symbol(dso[0], *api, *egl);
        if (!(*egl) && (*egl = getProcAddress(*api)) == nullptr)
        {
            logger::log_debug() << "load egl function " << *api << " failed";
        }

        egl++;
        api++;
    }

    api = egl_ext_names;
    auto* ext = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &egl_vendor->egl.ext);
    while (*api)
    {
        if ((*ext = getProcAddress(*api)) == nullptr)
        {
            logger::log_debug() << "load egl function " << *api << " failed";
        }

        ext++;
        api++;
    }
}

void egl_vendor_t::loader::load_gles_api()
{
    auto* gles1 = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &egl_vendor->hooks[GLESv1_INDEX].gl);
    auto* gles2 = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &egl_vendor->hooks[GLESv2_INDEX].gl);

    const char* const* api = gl_names;
    const char* const* api_1 = gl_names_1;
    while (*api)
    {
        if (*api_1 && strcmp(*api, *api_1) == 0)
        {
            load_symbol(is_GLES_driver ? dso[0] : dso[1], *api_1, *gles1);
            if (!(*gles1) && (*gles1 = getProcAddress(*api_1)) == nullptr)
            {
                logger::log_debug()
                    << "load glesv1 function " << *api_1 << " failed";
            }
            api_1++;
        }
        gles1++;

        {
            load_symbol(is_GLES_driver ? dso[0] : dso[2], *api, *gles2);
            if (!(*gles2) && (*gles2 = getProcAddress(*api)) == nullptr)
            {
                logger::log_debug()
                    << "load glesv2 function " << *api << " failed";
            }
        }
        api++;
        gles2++;
    }

    api = gl_ext_name;
    auto* glext = reinterpret_cast<__eglMustCastToProperFunctionPointerType*>(
        &egl_vendor->glext);
    while (*api)
    {
        if ((*glext = getProcAddress(*api)) == nullptr)
        {
            logger::log_debug()
                << "load gles ext function " << *api << " failed";
        }
        api++;
        glext++;
    }
}
