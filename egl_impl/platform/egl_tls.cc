#include "egl_tls.h"

namespace egl_wrapper {

thread_local egl_tls_t egl_tls{};

void egl_tls_t::clearError()
{
    egl_tls.error = EGL_SUCCESS;
}

EGLint egl_tls_t::getError()
{
    return egl_tls.error;
}

void egl_tls_t::setErrorImpl(EGLint err)
{
    egl_tls.error = err;
}

void egl_tls_t::clearTLS()
{
    egl_tls.error = EGL_SUCCESS;
}

thread_local gl_hooks_t const* tls_hook{};

void setGlThreadSpecific(gl_hooks_t const* value)
{
    tls_hook = value;
}

gl_hooks_t const* getGlThreadSpecific()
{
    return tls_hook;
}

} // namespace egl_wrapper
