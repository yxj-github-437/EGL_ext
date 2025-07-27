/*
 ** Copyright 2007, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include "logger.h"
#include <EGL/eglplatform.h>

#include "egl_platform_entries.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <unordered_map>

#include "egl_object.h"
#include "egl_tls.h"
#include "loader/loader.h"

#include "platform.h"

using namespace egl_wrapper;
// ----------------------------------------------------------------------------

namespace egl_wrapper {
/*
 * This is the list of EGL extensions exposed to applications.
 *
 * Some of them (gBuiltinExtensionString) are implemented entirely in this EGL
 * wrapper and are always available.
 *
 * The rest (gExtensionString) depend on support in the EGL driver, and are
 * only available if the driver supports them. However, some of these must be
 * supported because they are used by the Android system itself; these are
 * listed as mandatory below and are required by the CDD. The system *assumes*
 * the mandatory extensions are present and may not function properly if some
 * are missing.
 *
 * NOTE: Both strings MUST have a single space as the last character.
 */

// clang-format off
const char* const gClientExtensionString =
        "EGL_EXT_platform_base "
        "EGL_MESA_platform_surfaceless "
        "EGL_KHR_platform_gbm "
        "EGL_EXT_platform_wayland "
        "EGL_KHR_platform_wayland ";
// clang-format on

__eglMustCastToProperFunctionPointerType findProcAddress(const char* name);

// Note: This only works for existing GLenum's that are all 32bits.
// If you have 64bit attributes (e.g. pointers) you shouldn't be calling this.
inline void convertAttribs(const EGLAttrib* attribList,
                           std::vector<EGLint>& newList)
{
    for (const EGLAttrib* attr = attribList; attr && attr[0] != EGL_NONE;
         attr += 2)
    {
        newList.push_back(static_cast<EGLint>(attr[0]));
        newList.push_back(static_cast<EGLint>(attr[1]));
    }
    newList.push_back(EGL_NONE);
}

inline void convertInts(const EGLint* attrib_list,
                        std::vector<EGLAttrib>& newList)
{
    for (const EGLint* attr = attrib_list; attr && attr[0] != EGL_NONE;
         attr += 2)
    {
        newList.push_back(attr[0]);
        newList.push_back(attr[1]);
    }
    newList.push_back(EGL_NONE);
}

inline void clearError()
{
    egl_tls_t::clearError();
}

// ----------------------------------------------------------------------------

auto setGLHooksThreadSpecific = setGlThreadSpecific;

namespace {
    std::mutex mutex{};
    std::unordered_map<EGLSurface, ANativeWindow*> g_surface_window_map{};
} // namespace

static EGLDisplay eglGetPlatformDisplayTmpl(EGLenum platform,
                                            EGLNativeDisplayType display,
                                            const EGLAttrib* attrib_list)
{
    return egl_display_t::getFromNativeDisplay(platform, display, attrib_list);
}

EGLDisplay eglGetDisplayImpl(EGLNativeDisplayType display)
{
    clearError();
    return eglGetPlatformDisplayTmpl(egl_display_t::getNativePlatform(display),
                                     display, nullptr);
}

EGLDisplay eglGetPlatformDisplayImpl(EGLenum platform, void* native_display,
                                     const EGLAttrib* attrib_list)
{
    clearError();
    return eglGetPlatformDisplayTmpl(
        platform, static_cast<EGLNativeDisplayType>(native_display),
        attrib_list);
}

EGLDisplay eglGetPlatformDisplayEXTImpl(EGLenum platform, void* native_display,
                                        const EGLint* attrib_list)
{
    clearError();
    std::vector<EGLAttrib> convertedInts{};
    convertInts(attrib_list, convertedInts);

    return eglGetPlatformDisplayTmpl(
        platform, static_cast<EGLNativeDisplayType>(native_display),
        convertedInts.data());
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

EGLBoolean eglInitializeImpl(EGLDisplay dpy, EGLint* major, EGLint* minor)
{
    clearError();
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, (EGLBoolean)EGL_FALSE);

    EGLBoolean res = dp->initialize(major, minor);

    return res;
}

EGLBoolean eglTerminateImpl(EGLDisplay dpy)
{
    clearError();
    // NOTE: don't unload the drivers b/c some APIs can be called
    // after eglTerminate() has been called. eglTerminate() only
    // terminates an EGLDisplay, not a EGL itself.

    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, (EGLBoolean)EGL_FALSE);

    EGLBoolean res = egl_display_t::terminate(dpy);
    return res;
}

// ----------------------------------------------------------------------------
// configuration
// ----------------------------------------------------------------------------

EGLBoolean eglGetConfigsImpl(EGLDisplay dpy, EGLConfig* configs,
                             EGLint config_size, EGLint* num_config)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglGetConfigs(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfigImpl(EGLDisplay dpy, const EGLint* attrib_list,
                               EGLConfig* configs, EGLint config_size,
                               EGLint* num_config)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglChooseConfig(dpy, attrib_list, configs, config_size,
                                       num_config);
}

EGLBoolean eglGetConfigAttribImpl(EGLDisplay dpy, EGLConfig config,
                                  EGLint attribute, EGLint* value)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglGetConfigAttrib(dpy, config, attribute, value);
}

// ----------------------------------------------------------------------------
// surfaces
// ----------------------------------------------------------------------------

template <typename AttrType, typename CreateFuncType>
EGLSurface eglCreateWindowSurfaceTmpl(egl_display_t* dp, EGLConfig config,
                                      ANativeWindow* window,
                                      const AttrType* attrib_list,
                                      CreateFuncType createWindowSurfaceFunc)
{
    EGLSurface rval = EGL_NO_SURFACE;

    ANativeWindow* native_window = window;
    if (dp->platform_wrapper)
    {
        native_window = dp->platform_wrapper->create_window(window);
    }

    if (createWindowSurfaceFunc)
    {
        rval = createWindowSurfaceFunc(dp->dpy, config, native_window,
                                       attrib_list);
    }

    if (rval != EGL_NO_SURFACE)
    {
        std::lock_guard lock{mutex};
        g_surface_window_map.insert({rval, native_window});
    }
    else if (dp->platform_wrapper && native_window)
    {
        dp->platform_wrapper->destroy_window(native_window);
    }
    return rval;
}

EGLSurface eglCreateWindowSurfaceImpl(EGLDisplay dpy, EGLConfig config,
                                      NativeWindowType window,
                                      const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_NO_SURFACE);

    return eglCreateWindowSurfaceTmpl(dp, config, window, attrib_list,
                                      system->egl.eglCreateWindowSurface);
}

EGLSurface eglCreatePlatformWindowSurfaceImpl(EGLDisplay dpy, EGLConfig config,
                                              void* native_window,
                                              const EGLAttrib* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_NO_SURFACE);

    std::vector<EGLint> convertedAttribs{};
    convertAttribs(attrib_list, convertedAttribs);
    if (system->egl.eglCreatePlatformWindowSurface)
    {
        return eglCreateWindowSurfaceTmpl(
            dp, config, static_cast<ANativeWindow*>(native_window), attrib_list,
            system->egl.eglCreatePlatformWindowSurface);
    }
    else if (system->egl.ext.eglCreatePlatformWindowSurfaceEXT)
    {
        return eglCreateWindowSurfaceTmpl(
            dp, config, static_cast<ANativeWindow*>(native_window),
            convertedAttribs.data(),
            system->egl.ext.eglCreatePlatformWindowSurfaceEXT);
    }
    else
    {
        return eglCreateWindowSurfaceTmpl(
            dp, config, static_cast<ANativeWindow*>(native_window),
            convertedAttribs.data(), system->egl.eglCreateWindowSurface);
    }
}

EGLSurface eglCreatePlatformPixmapSurfaceImpl(EGLDisplay dpy, EGLConfig config,
                                              void* native_pixmap,
                                              const EGLAttrib* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglCreatePlatformPixmapSurface(
        dpy, config, native_pixmap, attrib_list);
}

EGLSurface eglCreatePixmapSurfaceImpl(EGLDisplay dpy, EGLConfig config,
                                      NativePixmapType pixmap,
                                      const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurfaceImpl(EGLDisplay dpy, EGLConfig config,
                                       const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLSurface eglCreatePlatformWindowSurfaceEXTImpl(EGLDisplay dpy,
                                                 EGLConfig config,
                                                 void* native_window,
                                                 const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_NO_SURFACE);

    EGLSurface rval = EGL_NO_SURFACE;
    if (system->egl.eglCreatePlatformWindowSurface)
    {
        std::vector<EGLAttrib> convertedAttribs{};
        convertInts(attrib_list, convertedAttribs);
        rval = eglCreateWindowSurfaceTmpl(
            dp, config, static_cast<ANativeWindow*>(native_window),
            convertedAttribs.data(),
            system->egl.eglCreatePlatformWindowSurface);
    }
    else if (system->egl.ext.eglCreatePlatformWindowSurfaceEXT)
    {
        rval = eglCreateWindowSurfaceTmpl(
            dp, config, static_cast<ANativeWindow*>(native_window), attrib_list,
            system->egl.ext.eglCreatePlatformWindowSurfaceEXT);
    }
    else
    {
        rval = eglCreateWindowSurfaceTmpl(
            dp, config, static_cast<ANativeWindow*>(native_window), attrib_list,
            system->egl.eglCreateWindowSurface);
    }
    return rval;
}

EGLBoolean eglDestroySurfaceImpl(EGLDisplay dpy, EGLSurface surface)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    EGLBoolean rval = system->egl.eglDestroySurface(dpy, surface);
    if (rval == EGL_TRUE)
    {
        std::lock_guard lock{mutex};
        ANativeWindow* native_window = nullptr;
        if (auto iter = g_surface_window_map.find(surface);
            iter != g_surface_window_map.end())
        {
            native_window = iter->second;
            g_surface_window_map.erase(iter);
        }
        if (native_window && dp->platform_wrapper)
        {
            dp->platform_wrapper->destroy_window(native_window);
        }
    }
    return rval;
}

EGLBoolean eglQuerySurfaceImpl(EGLDisplay dpy, EGLSurface surface,
                               EGLint attribute, EGLint* value)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglQuerySurface(dpy, surface, attribute, value);
}

// ----------------------------------------------------------------------------
// Contexts
// ----------------------------------------------------------------------------

EGLContext eglCreateContextImpl(EGLDisplay dpy, EGLConfig config,
                                EGLContext share_list,
                                const EGLint* attrib_list)
{
    clearError();
    return egl_context_t::createNativeContext(dpy, config, share_list,
                                              attrib_list);
}

EGLBoolean eglDestroyContextImpl(EGLDisplay dpy, EGLContext ctx)
{
    clearError();
    return egl_context_t::destroy(dpy, ctx);
}

EGLBoolean eglMakeCurrentImpl(EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                              EGLContext ctx)
{
    clearError();
    if (!egl_display_t::get(dpy))
        return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    egl_context_t* ctx_wrap = nullptr;
    if (ctx)
    {
        if (ctx_wrap = egl_context_t::get(ctx);
            !ctx_wrap || ctx_wrap->display != dpy)
            return setError(EGL_BAD_CONTEXT, EGL_FALSE);
    }

    auto system = egl_system_t::loader::getInstance().system;
    EGLBoolean rval = system->egl.eglMakeCurrent(dpy, draw, read, ctx);
    if (rval == EGL_TRUE)
    {
        if (ctx)
        {
            ctx_wrap->makeCurrent(draw, read);
            setGLHooksThreadSpecific(&system->hooks[ctx_wrap->version]);
        }
    }
    return rval;
}

EGLBoolean eglQueryContextImpl(EGLDisplay dpy, EGLContext ctx, EGLint attribute,
                               EGLint* value)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglQueryContext(dpy, ctx, attribute, value);
}

EGLContext eglGetCurrentContextImpl(void)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglGetCurrentContext();
}

EGLSurface eglGetCurrentSurfaceImpl(EGLint readdraw)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplayImpl(void)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglGetCurrentDisplay();
}

EGLBoolean eglWaitGLImpl(void)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return system->egl.eglWaitGL();
}

EGLBoolean eglWaitNativeImpl(EGLint engine)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglWaitNative(engine);
}

EGLint eglGetErrorImpl(void)
{
    EGLint err = EGL_SUCCESS;
    auto system = egl_system_t::loader::getInstance().system;
    err = system->egl.eglGetError();
    if (err == EGL_SUCCESS)
    {
        err = egl_tls_t::getError();
    }
    return err;
}

__eglMustCastToProperFunctionPointerType
eglGetProcAddressImpl(const char* procname)
{
    clearError();
    __eglMustCastToProperFunctionPointerType addr;
    addr = findProcAddress(procname);
    if (addr)
        return addr;

    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglGetProcAddress)
        addr = system->egl.eglGetProcAddress(procname);

    return addr;
}

EGLBoolean eglSwapBuffersWithDamageKHRImpl(EGLDisplay dpy, EGLSurface draw,
                                           const EGLint* rects, EGLint n_rects)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    EGLBoolean rval = EGL_TRUE;
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    ANativeWindow* native_window = nullptr;
    if (auto iter = g_surface_window_map.find(draw);
        iter != g_surface_window_map.end())
    {
        native_window = iter->second;
    }

    if (native_window && dp->platform_wrapper)
    {
        dp->platform_wrapper->prepare_swap(native_window, rects, n_rects);
    }
    if (system->egl.ext.eglSwapBuffersWithDamageKHR)
    {
        rval = system->egl.ext.eglSwapBuffersWithDamageKHR(dpy, draw, rects,
                                                           n_rects);
    }
    else
    {
        rval = system->egl.eglSwapBuffers(dpy, draw);
    }

    if (rval != EGL_TRUE)
    {
        logger::log_error() << "eglSwapBuffers with error " << std::showbase
                            << std::hex << system->egl.eglGetError();
        return rval;
    }

    if (system->egl.ext.eglCreateSyncKHR)
    {
        auto sync = system->egl.ext.eglCreateSyncKHR(dpy, EGL_SYNC_FENCE_KHR, nullptr);
        system->egl.ext.eglWaitSyncKHR(dpy, sync,
                                       EGL_SYNC_FLUSH_COMMANDS_BIT_KHR);
        system->egl.ext.eglDestroySyncKHR(dpy, sync);
    }

    if (native_window && dp->platform_wrapper)
    {
        dp->platform_wrapper->finish_swap(native_window);
    }
    return rval;
}

EGLBoolean eglSwapBuffersImpl(EGLDisplay dpy, EGLSurface surface)
{
    clearError();
    return eglSwapBuffersWithDamageKHRImpl(dpy, surface, nullptr, 0);
}

EGLBoolean eglCopyBuffersImpl(EGLDisplay dpy, EGLSurface surface,
                              NativePixmapType target)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglCopyBuffers(dpy, surface, target);
}

const char* eglQueryStringImpl(EGLDisplay dpy, EGLint name)
{
    clearError();
    static std::string extensions;
    auto system = egl_system_t::loader::getInstance().system;
    if (dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS)
    {
        // Return list of client extensions
        extensions = system->egl.eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
        extensions += " ";
        extensions += gClientExtensionString;
        return extensions.c_str();
    }

    extensions = system->egl.eglQueryString(dpy, name);
    if (name == EGL_EXTENSIONS)
    {
        extensions += " EGL_WL_bind_wayland_display";
    }

    return extensions.c_str();
}

// ----------------------------------------------------------------------------
// EGL 1.1
// ----------------------------------------------------------------------------

EGLBoolean eglSurfaceAttribImpl(EGLDisplay dpy, EGLSurface surface,
                                EGLint attribute, EGLint value)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImageImpl(EGLDisplay dpy, EGLSurface surface,
                               EGLint buffer)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImageImpl(EGLDisplay dpy, EGLSurface surface,
                                  EGLint buffer)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglReleaseTexImage(dpy, surface, buffer);
}

EGLBoolean eglSwapIntervalImpl(EGLDisplay dpy, EGLint interval)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    return system->egl.eglSwapInterval(dpy, interval);
}

// ----------------------------------------------------------------------------
// EGL 1.2
// ----------------------------------------------------------------------------

EGLBoolean eglWaitClientImpl(void)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;

    EGLBoolean res;
    if (system->egl.eglWaitClient)
    {
        res = system->egl.eglWaitClient();
    }
    else
    {
        res = system->egl.eglWaitGL();
    }
    return res;
}

EGLBoolean eglBindAPIImpl(EGLenum api)
{
    clearError();
    // bind this API on all EGLs
    EGLBoolean res = EGL_TRUE;
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglBindAPI)
    {
        res = system->egl.eglBindAPI(api);
    }
    return res;
}

EGLenum eglQueryAPIImpl(void)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglQueryAPI)
    {
        return system->egl.eglQueryAPI();
    }

    // or, it can only be OpenGL ES
    return EGL_OPENGL_ES_API;
}

EGLBoolean eglReleaseThreadImpl(void)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglReleaseThread)
    {
        system->egl.eglReleaseThread();
    }

    egl_tls_t::clearTLS();
    return EGL_TRUE;
}

EGLSurface eglCreatePbufferFromClientBufferImpl(EGLDisplay dpy, EGLenum buftype,
                                                EGLClientBuffer buffer,
                                                EGLConfig config,
                                                const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglCreatePbufferFromClientBuffer)
    {
        return system->egl.eglCreatePbufferFromClientBuffer(
            dpy, buftype, buffer, config, attrib_list);
    }
    return setError(EGL_BAD_CONFIG, EGL_NO_SURFACE);
}

EGLBoolean eglBindWaylandDisplayWLImpl(EGLDisplay dpy,
                                       struct wl_display* display)
{
    clearError();
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    if (dp->wlegl_global)
        return EGL_FALSE;

    dp->wlegl_global = create_server_wlegl(display);
    return EGL_TRUE;
}

EGLBoolean eglUnbindWaylandDisplayWLImpl(EGLDisplay dpy,
                                         struct wl_display* display)
{
    clearError();
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    if (!dp->wlegl_global)
        return EGL_FALSE;

    delete_server_wlegl(dp->wlegl_global);
    dp->wlegl_global = nullptr;
    return EGL_TRUE;
}

EGLBoolean eglQueryWaylandBufferWLImpl(EGLDisplay dpy,
                                       struct wl_resource* buffer,
                                       EGLint attribute, EGLint* value)
{
    clearError();
    egl_display_t* dp = get_display(dpy);
    if (!dp)
        return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    auto native_buffer =
        static_cast<ANativeWindowBuffer*>(get_buffer_from_resource(buffer));
    if (!native_buffer)
        return EGL_FALSE;

    switch (attribute)
    {
    case EGL_TEXTURE_FORMAT:
        switch (native_buffer->format)
        {
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
            *value = EGL_TEXTURE_RGB;
            break;
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
            *value = EGL_TEXTURE_RGBA;
            break;
        default:
            *value = EGL_TEXTURE_EXTERNAL_WL;
            break;
        }
        return EGL_TRUE;
    case EGL_WIDTH:
        *value = native_buffer->width;
        return EGL_TRUE;
    case EGL_HEIGHT:
        *value = native_buffer->height;
        return EGL_TRUE;
    }
    return EGL_FALSE;
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 3
// ----------------------------------------------------------------------------

EGLBoolean eglLockSurfaceKHRImpl(EGLDisplay dpy, EGLSurface surface,
                                 const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.ext.eglLockSurfaceKHR)
    {
        return system->egl.ext.eglLockSurfaceKHR(dpy, surface, attrib_list);
    }
    return setError(EGL_BAD_DISPLAY, (EGLBoolean)EGL_FALSE);
}

EGLBoolean eglUnlockSurfaceKHRImpl(EGLDisplay dpy, EGLSurface surface)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.ext.eglUnlockSurfaceKHR)
    {
        return system->egl.ext.eglUnlockSurfaceKHR(dpy, surface);
    }
    return setError(EGL_BAD_DISPLAY, (EGLBoolean)EGL_FALSE);
}

// Note: EGLImageKHR and EGLImage are the same thing so no need
// to templatize that.
template <typename AttrType, typename FuncType>
EGLImageKHR eglCreateImageTmpl(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                               EGLClientBuffer buffer,
                               const AttrType* attrib_list,
                               FuncType eglCreateImageFunc)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;
    if (target == EGL_WAYLAND_BUFFER_WL)
    {
        buffer = get_buffer_from_resource((struct wl_resource*)buffer);
        target = EGL_NATIVE_BUFFER_ANDROID;
        ctx = EGL_NO_CONTEXT;
        attrib_list = nullptr;
    }

    if (eglCreateImageFunc)
    {
        result = eglCreateImageFunc(dpy, ctx, target, buffer, attrib_list);
    }
    return result;
}

EGLImageKHR eglCreateImageKHRImpl(EGLDisplay dpy, EGLContext ctx,
                                  EGLenum target, EGLClientBuffer buffer,
                                  const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglCreateImageTmpl(dpy, ctx, target, buffer, attrib_list,
                              system->egl.ext.eglCreateImageKHR);
}

EGLImage eglCreateImageImpl(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                            EGLClientBuffer buffer,
                            const EGLAttrib* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglCreateImage)
    {
        return eglCreateImageTmpl(dpy, ctx, target, buffer, attrib_list,
                                  system->egl.eglCreateImage);
    }

    std::vector<EGLint> convertedAttribs;
    convertAttribs(attrib_list, convertedAttribs);
    return eglCreateImageTmpl(dpy, ctx, target, buffer, convertedAttribs.data(),
                              system->egl.ext.eglCreateImageKHR);
}

EGLBoolean eglDestroyImageTmpl(EGLDisplay dpy, EGLImageKHR img,
                               PFNEGLDESTROYIMAGEKHRPROC destroyImageFunc)
{
    clearError();
    EGLBoolean result = EGL_FALSE;
    if (destroyImageFunc)
    {
        result = destroyImageFunc(dpy, img);
    }
    return result;
}

EGLBoolean eglDestroyImageKHRImpl(EGLDisplay dpy, EGLImageKHR img)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglDestroyImageTmpl(dpy, img, system->egl.ext.eglDestroyImageKHR);
}

EGLBoolean eglDestroyImageImpl(EGLDisplay dpy, EGLImageKHR img)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglDestroyImage)
    {
        return eglDestroyImageTmpl(dpy, img, system->egl.eglDestroyImage);
    }

    return eglDestroyImageTmpl(dpy, img, system->egl.ext.eglDestroyImageKHR);
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 5
// ----------------------------------------------------------------------------

// NOTE: EGLSyncKHR and EGLSync are identical, no need to templatize
template <typename AttrType, typename FuncType>
EGLSyncKHR eglCreateSyncTmpl(EGLDisplay dpy, EGLenum type,
                             const AttrType* attrib_list,
                             FuncType eglCreateSyncFunc)
{
    clearError();
    EGLSyncKHR result = EGL_NO_SYNC_KHR;
    if (eglCreateSyncFunc)
    {
        result = eglCreateSyncFunc(dpy, type, attrib_list);
    }
    return result;
}

typedef EGLSurface(EGLAPIENTRYP PFNEGLCREATESYNC)(EGLDisplay dpy, EGLenum type,
                                                  const EGLAttrib* attrib_list);

EGLSyncKHR eglCreateSyncKHRImpl(EGLDisplay dpy, EGLenum type,
                                const EGLint* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglCreateSyncTmpl(dpy, type, attrib_list,
                             system->egl.ext.eglCreateSyncKHR);
}

EGLSync eglCreateSyncImpl(EGLDisplay dpy, EGLenum type,
                          const EGLAttrib* attrib_list)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglCreateSync)
    {
        return eglCreateSyncTmpl(dpy, type, attrib_list,
                                 system->egl.eglCreateSync);
    }

    std::vector<EGLint> convertedAttribs;
    convertAttribs(attrib_list, convertedAttribs);
    return eglCreateSyncTmpl(dpy, type, convertedAttribs.data(),
                             system->egl.ext.eglCreateSyncKHR);
}

EGLBoolean eglDestroySyncTmpl(EGLDisplay dpy, EGLSyncKHR sync,
                              PFNEGLDESTROYSYNCKHRPROC eglDestroySyncFunc)
{
    clearError();
    EGLBoolean result = EGL_FALSE;
    if (eglDestroySyncFunc)
    {
        result = eglDestroySyncFunc(dpy, sync);
    }
    return result;
}

EGLBoolean eglDestroySyncKHRImpl(EGLDisplay dpy, EGLSyncKHR sync)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglDestroySyncTmpl(dpy, sync, system->egl.ext.eglDestroySyncKHR);
}

EGLBoolean eglDestroySyncImpl(EGLDisplay dpy, EGLSyncKHR sync)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglDestroySync)
    {
        return eglDestroySyncTmpl(dpy, sync, system->egl.eglDestroySync);
    }

    return eglDestroySyncTmpl(dpy, sync, system->egl.ext.eglDestroySyncKHR);
}

EGLBoolean eglSignalSyncKHRImpl(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
{
    clearError();
    EGLBoolean result = EGL_FALSE;
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.ext.eglSignalSyncKHR)
    {
        result = system->egl.ext.eglSignalSyncKHR(dpy, sync, mode);
    }
    return result;
}

EGLint eglClientWaitSyncTmpl(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags,
                             EGLTimeKHR timeout,
                             PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncFunc)
{
    clearError();
    EGLint result = EGL_FALSE;
    if (eglClientWaitSyncFunc)
    {
        result = eglClientWaitSyncFunc(dpy, sync, flags, timeout);
    }
    return result;
}

EGLint eglClientWaitSyncKHRImpl(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags,
                                EGLTimeKHR timeout)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglClientWaitSyncTmpl(dpy, sync, flags, timeout,
                                 system->egl.ext.eglClientWaitSyncKHR);
}

EGLint eglClientWaitSyncImpl(EGLDisplay dpy, EGLSync sync, EGLint flags,
                             EGLTimeKHR timeout)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglClientWaitSync)
    {
        return eglClientWaitSyncTmpl(dpy, sync, flags, timeout,
                                     system->egl.eglClientWaitSync);
    }

    return eglClientWaitSyncTmpl(dpy, sync, flags, timeout,
                                 system->egl.ext.eglClientWaitSyncKHR);
}

template <typename AttrType, typename FuncType>
EGLBoolean eglGetSyncAttribTmpl(EGLDisplay dpy, EGLSyncKHR sync,
                                EGLint attribute, AttrType* value,
                                FuncType eglGetSyncAttribFunc)
{
    EGLBoolean result = EGL_FALSE;
    if (eglGetSyncAttribFunc)
    {
        result = eglGetSyncAttribFunc(dpy, sync, attribute, value);
    }
    return result;
}

EGLBoolean eglGetSyncAttribImpl(EGLDisplay dpy, EGLSync sync, EGLint attribute,
                                EGLAttrib* value)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglGetSyncAttrib)
    {
        return eglGetSyncAttribTmpl(dpy, sync, attribute, value,
                                    system->egl.eglGetSyncAttrib);
    }

    // Fallback to KHR, ask for EGLint attribute and cast back to EGLAttrib
    EGLint attribValue;
    EGLBoolean ret = eglGetSyncAttribTmpl(dpy, sync, attribute, &attribValue,
                                          system->egl.ext.eglGetSyncAttribKHR);
    if (ret)
    {
        *value = static_cast<EGLAttrib>(attribValue);
    }
    return ret;
}

EGLBoolean eglGetSyncAttribKHRImpl(EGLDisplay dpy, EGLSyncKHR sync,
                                   EGLint attribute, EGLint* value)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglGetSyncAttribTmpl(dpy, sync, attribute, value,
                                system->egl.ext.eglGetSyncAttribKHR);
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 15
// ----------------------------------------------------------------------------

// Need to template function type because return type is different
template <typename ReturnType, typename FuncType>
ReturnType eglWaitSyncTmpl(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags,
                           FuncType eglWaitSyncFunc)
{
    ReturnType result = EGL_FALSE;
    auto system = egl_system_t::loader::getInstance().system;
    if (eglWaitSyncFunc)
    {
        result = eglWaitSyncFunc(dpy, sync, flags);
    }
    return result;
}

typedef EGLBoolean(EGLAPIENTRYP PFNEGLWAITSYNC)(EGLDisplay dpy, EGLSync sync,
                                                EGLint flags);

EGLint eglWaitSyncKHRImpl(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    return eglWaitSyncTmpl<EGLint>(dpy, sync, flags,
                                   system->egl.ext.eglWaitSyncKHR);
}

EGLBoolean eglWaitSyncImpl(EGLDisplay dpy, EGLSync sync, EGLint flags)
{
    clearError();
    auto system = egl_system_t::loader::getInstance().system;
    if (system->egl.eglWaitSync)
    {
        return eglWaitSyncTmpl<EGLBoolean>(dpy, sync, flags,
                                           system->egl.eglWaitSync);
    }

    return static_cast<EGLBoolean>(eglWaitSyncTmpl<EGLint>(
        dpy, sync, flags, system->egl.ext.eglWaitSyncKHR));
}

const GLubyte* glGetStringImpl(GLenum name)
{
    if (name == GL_EXTENSIONS)
    {
        auto system = egl_system_t::loader::getInstance().system;
        EGLContext ctx = system->egl.eglGetCurrentContext();
        if (ctx)
        {
            if (auto ctx_wrap = egl_context_t::get(ctx); ctx_wrap)
            {
                return (const GLubyte*)ctx_wrap->gl_extensions.c_str();
            }
        }
    }

    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;
    const GLubyte* ret{};
    if (_c)
        ret = _c->glGetString(name);
    return ret;
}

const GLubyte* glGetStringiImpl(GLenum name, GLuint index)
{
    if (name == GL_EXTENSIONS)
    {
        auto system = egl_system_t::loader::getInstance().system;
        EGLContext ctx = system->egl.eglGetCurrentContext();
        if (ctx)
        {
            auto ctx_wrap = egl_context_t::get(ctx);
            if (ctx_wrap && index < ctx_wrap->tokenized_gl_extensions.size())
            {
                return (const GLubyte*)ctx_wrap->tokenized_gl_extensions[index]
                    .c_str();
            }
        }
    }

    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;
    const GLubyte* ret{};
    if (_c)
        ret = _c->glGetStringi(name, index);
    return ret;
}

void glGetBooleanvImpl(GLenum pname, GLboolean* data)
{
    if (pname == GL_NUM_EXTENSIONS)
    {
        auto system = egl_system_t::loader::getInstance().system;
        EGLContext ctx = system->egl.eglGetCurrentContext();
        if (ctx)
        {
            if (auto ctx_wrap = egl_context_t::get(ctx); ctx_wrap)
            {
                *data = (GLboolean)ctx_wrap->tokenized_gl_extensions.size() > 0;
                return;
            }
        }
    }

    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;
    if (_c)
        _c->glGetBooleanv(pname, data);
}

void glGetFloatvImpl(GLenum pname, GLfloat* data)
{
    if (pname == GL_NUM_EXTENSIONS)
    {
        auto system = egl_system_t::loader::getInstance().system;
        EGLContext ctx = system->egl.eglGetCurrentContext();
        if (ctx)
        {
            auto ctx_wrap = egl_context_t::get(ctx);
            if (ctx_wrap)
            {
                *data = (GLfloat)ctx_wrap->tokenized_gl_extensions.size();
                return;
            }
        }
    }

    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;
    if (_c)
        _c->glGetFloatv(pname, data);
}

void glGetIntegervImpl(GLenum pname, GLint* data)
{
    if (pname == GL_NUM_EXTENSIONS)
    {
        auto system = egl_system_t::loader::getInstance().system;
        EGLContext ctx = system->egl.eglGetCurrentContext();
        if (ctx)
        {
            if (auto ctx_wrap = egl_context_t::get(ctx); ctx_wrap)
            {
                *data = (GLint)ctx_wrap->tokenized_gl_extensions.size();
                return;
            }
        }
    }

    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;
    if (_c)
        _c->glGetIntegerv(pname, data);
}

void glGetInteger64vImpl(GLenum pname, GLint64* data)
{
    if (pname == GL_NUM_EXTENSIONS)
    {
        auto system = egl_system_t::loader::getInstance().system;
        EGLContext ctx = system->egl.eglGetCurrentContext();
        if (ctx)
        {
            if (auto ctx_wrap = egl_context_t::get(ctx); ctx_wrap)
            {
                *data = (GLint64)ctx_wrap->tokenized_gl_extensions.size();
                return;
            }
        }
    }

    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;
    if (_c)
        _c->glGetInteger64v(pname, data);
}

#undef GL_ENTRY
#undef EGL_ENTRY
#define GL_ENTRY(_r, _api, ...) _api ## Impl,
#define EGL_ENTRY(_r, _api, ...) _api ## Impl,
void fullPlatformImpl(platform_impl_t& pImpl)
{
    // clang-format off
    platform_impl_t impl = {
        #include "loader/platform_entries.in"
    };
    // clang-format on

    memcpy(&pImpl, &impl, sizeof(platform_impl_t));
}
#undef GL_ENTRY
#undef EGL_ENTRY

/*
 * EGL entry-points exposed to 3rd party applications
 *
 */

// clang-format off
static const std::unordered_map<std::string_view, __eglMustCastToProperFunctionPointerType> sEntryMap = {
#undef EGL_ENTRY
#undef GL_ENTRY
#define EGL_ENTRY(_r, _api, ...) {#_api, (__eglMustCastToProperFunctionPointerType)_api ## Impl},
#define GL_ENTRY(_r, _api, ...)  {#_api, (__eglMustCastToProperFunctionPointerType)_api ## Impl},

#include "loader/platform_entries.in"

#undef GL_ENTRY
#undef EGL_ENTRY

    // EGL_EXT_platform_base
    { "eglGetPlatformDisplayEXT", (__eglMustCastToProperFunctionPointerType)eglGetPlatformDisplayEXTImpl },
    { "eglCreatePlatformWindowSurfaceEXT", (__eglMustCastToProperFunctionPointerType)eglCreatePlatformWindowSurfaceEXTImpl },

    // EGL_WL_bind_wayland_display
    { "eglBindWaylandDisplayWL", (__eglMustCastToProperFunctionPointerType)eglBindWaylandDisplayWLImpl },
    { "eglUnbindWaylandDisplayWL", (__eglMustCastToProperFunctionPointerType)eglUnbindWaylandDisplayWLImpl },
    { "eglQueryWaylandBufferWL", (__eglMustCastToProperFunctionPointerType)eglQueryWaylandBufferWLImpl },
};
// clang-format on

__eglMustCastToProperFunctionPointerType findProcAddress(const char* name)
{
    if (auto iter = sEntryMap.find(name); iter != sEntryMap.end())
        return iter->second;
    return nullptr;
}

} // namespace egl_wrapper
