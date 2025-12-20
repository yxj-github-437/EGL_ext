#ifndef EGL_PLATFORM_BASE_H_
#define EGL_PLATFORM_BASE_H_

#include <EGL/egl.h>

#include "platform_common/base/platform_base.h"

class EGLBaseNativeWindow : public BaseNativeWindow {
  public:
    operator ANativeWindow*()
    {
        auto win = static_cast<ANativeWindow*>(this);
        return win;
    }
};

class platform_wrapper_t {
  public:
    virtual ~platform_wrapper_t() = default;

    virtual EGLBoolean initialize() = 0;
    virtual EGLBoolean terminate() = 0;
    virtual ANativeWindow* create_window(void* native_window) = 0;
    virtual void destroy_window(ANativeWindow* win) = 0;
    virtual void prepare_swap(ANativeWindow* win, const EGLint* rects,
                              EGLint n_rects) = 0;
    virtual void finish_swap(ANativeWindow* win) = 0;
};

#endif // EGL_PLATFORM_BASE_H_
