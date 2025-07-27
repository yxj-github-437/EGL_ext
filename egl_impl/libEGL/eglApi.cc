/*
 ** Copyright 2018, The Android Open Source Project
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


#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "loader/loader.h"

using namespace egl_wrapper;

EGLDisplay eglGetDisplay(EGLNativeDisplayType display)
{
    // Call down the chain, which usually points directly to the impl
    // but may also be routed through layers
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetDisplay(display);
}

EGLDisplay eglGetPlatformDisplay(EGLenum platform, EGLNativeDisplayType display,
                                 const EGLAttrib* attrib_list)
{
    // Call down the chain, which usually points directly to the impl
    // but may also be routed through layers
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetPlatformDisplay(platform, display, attrib_list);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglInitialize(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglTerminate(dpy);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig* configs, EGLint config_size,
                         EGLint* num_config)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetConfigs(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list,
                           EGLConfig* configs, EGLint config_size,
                           EGLint* num_config)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglChooseConfig(dpy, attrib_list, configs, config_size,
                                         num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                              EGLint attribute, EGLint* value)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetConfigAttrib(dpy, config, attribute, value);
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                                  NativeWindowType window,
                                  const EGLint* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreateWindowSurface(dpy, config, window,
                                                attrib_list);
}

EGLSurface eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config,
                                          void* native_window,
                                          const EGLAttrib* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreatePlatformWindowSurface(
        dpy, config, native_window, attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
                                  NativePixmapType pixmap,
                                  const EGLint* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreatePixmapSurface(dpy, config, pixmap,
                                                attrib_list);
}

EGLSurface eglCreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config,
                                          void* native_pixmap,
                                          const EGLAttrib* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreatePlatformPixmapSurface(
        dpy, config, native_pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                                   const EGLint* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglDestroySurface(dpy, surface);
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute,
                           EGLint* value)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglQuerySurface(dpy, surface, attribute, value);
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
                            EGLContext share_list, const EGLint* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreateContext(dpy, config, share_list, attrib_list);
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglDestroyContext(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                          EGLContext ctx)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglMakeCurrent(dpy, draw, read, ctx);
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute,
                           EGLint* value)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglQueryContext(dpy, ctx, attribute, value);
}

EGLContext eglGetCurrentContext(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetCurrentContext();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetCurrentDisplay();
}

EGLBoolean eglWaitGL(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglWaitGL();
}

EGLBoolean eglWaitNative(EGLint engine)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglWaitNative(engine);
}

EGLint eglGetError(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetError();
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char* procname)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetProcAddress(procname);
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglSwapBuffers(dpy, surface);
}

EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
                          NativePixmapType target)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCopyBuffers(dpy, surface, target);
}

const char* eglQueryString(EGLDisplay dpy, EGLint name)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglQueryString(dpy, name);
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
                            EGLint attribute, EGLint value)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglReleaseTexImage(dpy, surface, buffer);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglSwapInterval(dpy, interval);
}

EGLBoolean eglWaitClient(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglWaitClient();
}

EGLBoolean eglBindAPI(EGLenum api)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglBindAPI(api);
}

EGLenum eglQueryAPI(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglQueryAPI();
}

EGLBoolean eglReleaseThread(void)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglReleaseThread();
}

EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype,
                                            EGLClientBuffer buffer,
                                            EGLConfig config,
                                            const EGLint* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreatePbufferFromClientBuffer(dpy, buftype, buffer,
                                                          config, attrib_list);
}

EGLImage eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                        EGLClientBuffer buffer, const EGLAttrib* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreateImage(dpy, ctx, target, buffer, attrib_list);
}

EGLBoolean eglDestroyImage(EGLDisplay dpy, EGLImageKHR img)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglDestroyImage(dpy, img);
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 5
// ----------------------------------------------------------------------------

EGLSyncKHR eglCreateSync(EGLDisplay dpy, EGLenum type,
                         const EGLAttrib* attrib_list)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglCreateSync(dpy, type, attrib_list);
}

EGLBoolean eglDestroySync(EGLDisplay dpy, EGLSyncKHR sync)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglDestroySync(dpy, sync);
}

EGLint eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags,
                         EGLTimeKHR timeout)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}


EGLBoolean eglGetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute,
                            EGLAttrib* value)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglGetSyncAttrib(dpy, sync, attribute, value);
}

EGLBoolean eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags)
{
    auto system = egl_system_t::loader::getInstance().system;
    return system->platform.eglWaitSync(dpy, sync, flags);
}
