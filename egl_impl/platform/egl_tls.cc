#include "egl_tls.h"
#include <memory>

namespace egl_wrapper {
thread_local std::unique_ptr<egl_tls_t> egl_tls{};

egl_tls_t* get_tls_data() noexcept
{
    if (!egl_tls)
    {
        egl_tls = std::make_unique<egl_tls_t>();
    }
    return egl_tls.get();
}

void egl_tls_t::clearError() noexcept
{
    get_tls_data()->error = EGL_SUCCESS;
}

EGLint egl_tls_t::getError() noexcept
{
    return get_tls_data()->error;
}

EGLenum egl_tls_t::getCurrentAPI() noexcept
{
    return get_tls_data()->currentAPI;
}

void egl_tls_t::setErrorImpl(EGLint err) noexcept
{
    get_tls_data()->error = err;
}

void egl_tls_t::clearTLS() noexcept
{
    egl_tls.reset();
}

void egl_tls_t::setGlHooks(gl_hooks_t const* value) noexcept
{
    get_tls_data()->hooks = value;
}

const gl_hooks_t* egl_tls_t::getGlHooks() noexcept
{
    return get_tls_data()->hooks;
}

void setGlThreadSpecific(gl_hooks_t const* value)
{
    egl_tls_t::setGlHooks(value);
}

gl_hooks_t const* getGlThreadSpecific()
{
    return egl_tls_t::getGlHooks();
}
} // namespace egl_wrapper
