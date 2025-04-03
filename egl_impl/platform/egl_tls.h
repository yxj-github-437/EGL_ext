#ifndef ANDROID_EGL_TLS_H
#define ANDROID_EGL_TLS_H

#include "loader/hooks.h"

#include <EGL/egl.h>

namespace egl_wrapper {
class egl_tls_t {
    EGLint error = EGL_SUCCESS;

  public:
    static void clearError();
    static EGLint getError();
    static void setErrorImpl(EGLint);

    static void clearTLS();
};

#define setError(_e, _r)                                                       \
    ({                                                                         \
        ::egl_wrapper::egl_tls_t::setErrorImpl(_e);                            \
        _r;                                                                    \
    })

EGLAPI void setGlThreadSpecific(gl_hooks_t const* value);
EGLAPI gl_hooks_t const* getGlThreadSpecific();
} // namespace egl_wrapper

#endif
