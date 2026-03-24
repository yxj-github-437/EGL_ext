#include "egl_display.h"
#include "loader/loader.h"
#include "logger.h"

using namespace egl_wrapper;

namespace {
// clang-format off
constexpr const char* const sVendorString = "Android";
const char* const gClientExtensionString =
        "EGL_EXT_client_extensions "
        "EGL_KHR_platform_android "
        "EGL_EXT_platform_base "
        "EGL_MESA_platform_surfaceless "
        "EGL_KHR_platform_gbm "
        "EGL_EXT_platform_wayland "
        "EGL_KHR_platform_wayland ";
// clang-format on
} // namespace

EGLBoolean egl_display_t::initialize(EGLint* major, EGLint* minor) noexcept
{
    if (initialized)
    {
        /* Update applications version of major and minor if not NULL */
        if ((major != nullptr) && (minor != nullptr))
        {
            *major = Version / 10;
            *minor = Version % 10;
        }
        return EGL_TRUE;
    }

    EGLint major_ = -1, minor_ = -1;
    auto& vendor = egl_vendor_t::loader::getInstance().egl_vendor;
    if (!vendor->egl.eglInitialize(dpy, &major_, &minor_))
    {
        return EGL_FALSE;
    }
    initialized = EGL_TRUE;

    vendor_queried_strings.vendor = vendor->egl.eglQueryString(dpy, EGL_VENDOR);
    vendor_queried_strings.version =
        vendor->egl.eglQueryString(dpy, EGL_VERSION);
    vendor_queried_strings.extensions =
        vendor->egl.eglQueryString(dpy, EGL_EXTENSIONS);
    vendor_queried_strings.clientApi =
        vendor->egl.eglQueryString(dpy, EGL_CLIENT_APIS);

    if (minor_ == 5)
    {
        // full list in egl_entries.in
        if (!vendor->egl.eglCreateImage || !vendor->egl.eglDestroyImage ||
            !vendor->egl.eglGetPlatformDisplay ||
            !vendor->egl.eglCreatePlatformWindowSurface ||
            !vendor->egl.eglCreatePlatformPixmapSurface ||
            !vendor->egl.eglCreateSync || !vendor->egl.eglDestroySync ||
            !vendor->egl.eglClientWaitSync || !vendor->egl.eglGetSyncAttrib ||
            !vendor->egl.eglWaitSync)
        {
            logger::log_error()
                << "Driver indicates EGL 1.5 support, but does not have "
                   "a critical API";
            minor_ = 4;
        }
    }

    Version = major_ * 10 + minor_;
    VersionString = std::to_string(major_) + "." + std::to_string(minor_) +
                    " Android META-EGL";

    ClientAPIs = EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT;
    if (ClientAPIsString.empty())
    {
        if (ClientAPIs & EGL_OPENGL_BIT)
        {
            ClientAPIsString.append("OpenGL ");
        }

        // clang-format off
        if (ClientAPIs & EGL_OPENGL_ES_BIT ||
            ClientAPIs & EGL_OPENGL_ES2_BIT ||
            ClientAPIs & EGL_OPENGL_ES3_BIT_KHR)
        // clang-format on
        {
            ClientAPIsString.append("OpenGL_ES ");
        }

        if (ClientAPIs & EGL_OPENVG_BIT)
        {
            ClientAPIsString.append("OpenVG ");
        }
    }

    /* Update applications version of major and minor if not NULL */
    if ((major != nullptr) && (minor != nullptr))
    {
        *major = major_;
        *minor = minor_;
    }
    return EGL_TRUE;
}

const char* egl_display_t::get_client_extensions() noexcept
{
    return gClientExtensionString;
}
