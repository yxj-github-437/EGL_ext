#ifndef ANDROID_EGL_TLS_H
#define ANDROID_EGL_TLS_H

#include "loader/hooks.h"

#include <EGL/egl.h>

namespace egl_wrapper {
class egl_tls_t {
    EGLint error = EGL_SUCCESS;
    EGLenum currentAPI = EGL_OPENGL_ES_API;
    const gl_hooks_t* hooks = nullptr;

  public:
    static void clearError() noexcept;
    static EGLint getError() noexcept;
    static EGLenum getCurrentAPI() noexcept;
    static void setErrorImpl(EGLint) noexcept;

    static void setGlHooks(gl_hooks_t const* value) noexcept;
    static const gl_hooks_t* getGlHooks() noexcept;

    static void clearTLS() noexcept;
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
