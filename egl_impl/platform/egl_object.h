#ifndef ANDROID_EGL_DISPLAY_H
#define ANDROID_EGL_DISPLAY_H

#include <EGL/egl.h>
#include <string>
#include <vector>

#include "platform/platform.h"

class egl_object_t {
  public:
    ~egl_object_t() = default;
};

class egl_display_t {
  public:
    EGLNativeDisplayType ndpy;
    EGLDisplay dpy;
    EGLint major;
    EGLint minor;
    uint32_t ref_count;

    std::unique_ptr<platform_wrapper_t> platform_wrapper;
    struct server_wlegl* wlegl_global;

    egl_display_t() = default;
    ~egl_display_t();

    static EGLDisplay getFromNativeDisplay(EGLenum platform,
                                           EGLNativeDisplayType disp,
                                           const EGLAttrib* attrib_list);
    EGLBoolean initialize(EGLint* major, EGLint* minor);
    static EGLBoolean terminate(EGLDisplay dpy);

    static EGLenum getNativePlatform(EGLNativeDisplayType disp);
    static egl_display_t* get(EGLDisplay dpy);
};

class egl_context_t : public egl_object_t {
  public:
    EGLDisplay display;
    EGLContext ctx;

    EGLint version;
    std::string gl_extensions;
    std::vector<std::string> tokenized_gl_extensions;

    egl_context_t() = default;
    ~egl_context_t() = default;

    static EGLContext createNativeContext(EGLDisplay dpy, EGLConfig config,
        EGLContext share_list,
        const EGLint* attrib_list);
    void makeCurrent(EGLSurface draw, EGLSurface read);
    static EGLBoolean destroy(EGLDisplay dpy, EGLContext ctx);

    static egl_context_t* get(EGLContext ctx);
};

inline egl_display_t* get_display(EGLDisplay dpy)
{
    return egl_display_t::get(dpy);
}

#endif
