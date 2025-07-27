#include "loader/loader.h"
#include "egl_object.h"
#include "egl_tls.h"

#include "platform/platform.h"

#include "utils.h"
#include "logger.h"

#include <string>
#include <sstream>
#include <memory>
#include <map>
#include <mutex>

using namespace egl_wrapper;

namespace {
std::mutex mutex{};
std::unique_ptr<egl_display_t> g_dpy;
std::map<EGLContext, std::unique_ptr<egl_context_t>> g_ctx_map{};
} // namespace

egl_display_t* egl_display_t::get(EGLDisplay dpy)
{
    std::lock_guard lock{mutex};
    if (g_dpy && g_dpy->dpy == dpy)
        return g_dpy.get();
    return nullptr;
}

egl_display_t::~egl_display_t()
{
    if (wlegl_global)
    {
        delete_server_wlegl(wlegl_global);
        wlegl_global = nullptr;
    }
}

EGLDisplay egl_display_t::getFromNativeDisplay(EGLenum platform,
                                               EGLNativeDisplayType disp,
                                               const EGLAttrib* attrib_list)
{
    logger::log_info() << "call " << __PRETTY_FUNCTION__ << " with platform "
                       << std::showbase << std::hex << platform;

    std::lock_guard lock{mutex};
    auto system = egl_system_t::loader::getInstance().system;
    if (g_dpy)
    {
        if (disp == g_dpy->ndpy)
        {
            return g_dpy->dpy;
        }
        else
        {
            return setError(EGL_BAD_ALLOC, EGL_NO_DISPLAY);
        }
    }

    if (disp != EGL_DEFAULT_DISPLAY)
    {
        EGLenum plat = getNativePlatform(disp);
        if (platform != plat && (platform != EGL_PLATFORM_SURFACELESS_MESA ||
                                 platform != EGL_PLATFORM_GBM_KHR))
        {
            return setError(EGL_BAD_PARAMETER, EGL_NO_DISPLAY);
        }
    }

    std::unique_ptr<platform_wrapper_t> platform_wrapper = nullptr;
    switch (platform)
    {
    case EGL_PLATFORM_ANDROID_KHR:
    case EGL_PLATFORM_SURFACELESS_MESA:
    case EGL_PLATFORM_GBM_KHR:
        break;
    case EGL_PLATFORM_WAYLAND_EXT:
        platform_wrapper =
            std::make_unique<wayland_wrapper_t>((struct wl_display*)disp);
        break;
    }

    EGLDisplay dpy = system->egl.eglGetPlatformDisplay(
        EGL_PLATFORM_ANDROID_KHR, EGL_DEFAULT_DISPLAY, attrib_list);
    if (!dpy)
    {
        return EGL_NO_DISPLAY;
    }

    g_dpy = std::make_unique<egl_display_t>();
    g_dpy->ndpy = disp;
    g_dpy->dpy = dpy;
    g_dpy->platform_initialized = false;
    g_dpy->wlegl_global = nullptr;
    g_dpy->platform_wrapper = std::move(platform_wrapper);
    return dpy;
}

EGLenum egl_display_t::getNativePlatform(EGLNativeDisplayType disp)
{
    logger::log_info() << "call " << __PRETTY_FUNCTION__ << " with display "
                       << disp;
    if (disp == EGL_DEFAULT_DISPLAY)
    {
        if (getenv("WAYLAND_DISPLAY"))
            return EGL_PLATFORM_WAYLAND_EXT;
        return EGL_PLATFORM_ANDROID_KHR;
    }
    else if (check_wayland_display((struct wl_display*)disp))
    {
        return EGL_PLATFORM_WAYLAND_EXT;
    }

    return EGL_PLATFORM_ANDROID_KHR;
}

EGLBoolean egl_display_t::initialize(EGLint* major, EGLint* minor)
{
    std::lock_guard lock{mutex};
    EGLBoolean rval = EGL_TRUE;
    if (platform_wrapper && !platform_initialized &&
        (rval = platform_wrapper->initialize()) != EGL_TRUE)
    {
        platform_wrapper->terminate();
        return setError(EGL_NOT_INITIALIZED, rval);
    }
    platform_initialized = true;

    auto system = egl_system_t::loader::getInstance().system;
    if ((rval = system->egl.eglInitialize(dpy, &this->major, &this->minor)))
    {
        if (major)
            *major = this->major;
        if (minor)
            *minor = this->minor;
    }
    else
    {
        if (platform_wrapper)
            platform_wrapper->terminate();
        platform_initialized = false;
    }

    return rval;
}

EGLBoolean egl_display_t::terminate(EGLDisplay dpy)
{
    std::lock_guard lock{mutex};
    auto system = egl_system_t::loader::getInstance().system;
    if (g_dpy && g_dpy->dpy == dpy)
    {
        if (g_dpy->platform_wrapper && g_dpy->platform_initialized)
        {
            g_dpy->platform_wrapper->terminate();
        }
        g_dpy->platform_initialized = false;

        logger::log_info() << "call native eglTerminate";
        return system->egl.eglTerminate(dpy);
    }
    return setError(EGL_BAD_DISPLAY, EGL_FALSE);
}

egl_context_t* egl_context_t::get(EGLContext ctx)
{
    std::lock_guard lock{mutex};
    if (auto it = g_ctx_map.find(ctx); it != g_ctx_map.end())
    {
        return it->second.get();
    }
    return nullptr;
}

EGLContext egl_context_t::createNativeContext(EGLDisplay dpy, EGLConfig config,
                                              EGLContext share_list,
                                              const EGLint* attrib_list)
{
    std::lock_guard lock{mutex};
    auto system = egl_system_t::loader::getInstance().system;
    auto context =
        system->egl.eglCreateContext(dpy, config, share_list, attrib_list);
    if (!context)
        return EGL_NO_CONTEXT;

    auto uctx = std::make_unique<egl_context_t>();
    uctx->display = dpy;
    uctx->ctx = context;
    uctx->version = egl_system_t::GLESv1_INDEX;
    if (attrib_list)
    {
        while (*attrib_list != EGL_NONE)
        {
            EGLint attr = *attrib_list++;
            EGLint value = *attrib_list++;
            if (attr == EGL_CONTEXT_CLIENT_VERSION)
            {
                if (value == 1)
                {
                    uctx->version = egl_system_t::GLESv1_INDEX;
                }
                else if (value == 2 || value == 3)
                {
                    uctx->version = egl_system_t::GLESv2_INDEX;
                }
            }
        }
    }
    g_ctx_map.insert({context, std::move(uctx)});
    return context;
}

EGLBoolean egl_context_t::destroy(EGLDisplay dpy, EGLContext ctx)
{
    std::lock_guard lock{mutex};
    auto system = egl_system_t::loader::getInstance().system;

    EGLBoolean rval = EGL_FALSE;
    if (auto iter = g_ctx_map.find(ctx); iter != g_ctx_map.end())
    {
        if ((rval = system->egl.eglDestroyContext(dpy, ctx)) == EGL_TRUE)
        {
            g_ctx_map.erase(iter);
        }
    }
    return rval;
}

void egl_context_t::makeCurrent(EGLSurface draw, EGLSurface read)
{
    auto system = egl_system_t::loader::getInstance().system;
    if (gl_extensions.empty())
    {
        gl_extensions =
            (const char*)system->hooks[version].gl.glGetString(GL_EXTENSIONS);

        if (size_t pos = 0;
            (pos = gl_extensions.find("GL_EXT_read_format_bgra")) !=
            gl_extensions.npos)
        {
            static bool read_format_bgra_check = false;
            static bool read_format_bgra_valid = true;
            if (!read_format_bgra_check)
            {
                read_format_bgra_check = true;
                // clang-format off
                if (utils::gen_env_option<bool>("READ_FORMAT_BGRA_CHECK",
                                                {{"1", true}})) [[unlikely]]
                // clang-format on
                {
                    GLuint texture{};
                    GLuint fbo{};
                    system->hooks[version].gl.glGenTextures(1, &texture);
                    system->hooks[version].gl.glBindTexture(GL_TEXTURE_2D,
                                                            texture);
                    system->hooks[version].gl.glTexImage2D(
                        GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, NULL);
                    system->hooks[version].gl.glBindTexture(GL_TEXTURE_2D, 0);
                    auto glGenFramebuffers =
                        system->hooks[version].gl.glGenFramebuffers;
                    auto glBindFramebuffer =
                        system->hooks[version].gl.glBindFramebuffer;
                    auto glFramebufferTexture2D =
                        system->hooks[version].gl.glFramebufferTexture2D;
                    auto glDeleteFramebuffers =
                        system->hooks[version].gl.glDeleteFramebuffers;
                    if (!system->hooks[version].gl.glGenFramebuffers)
                    {
                        glGenFramebuffers =
                            system->glext.glGenFramebuffersOES;
                        glBindFramebuffer =
                            system->glext.glBindFramebufferOES;
                        glFramebufferTexture2D =
                            system->glext.glFramebufferTexture2DOES;
                        glDeleteFramebuffers =
                            system->glext.glDeleteFramebuffersOES;
                    }
                    glGenFramebuffers(1, &fbo);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                           GL_TEXTURE_2D, texture, 0);
                    uint8_t bytes[4] = {};
                    system->hooks[version].gl.glReadPixels(
                        0, 0, 1, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bytes);
                    GLenum use_bgra_rval =
                        system->hooks[version].gl.glGetError();
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glDeleteFramebuffers(1, &fbo);
                    system->hooks[version].gl.glDeleteTextures(1, &texture);

                    if (use_bgra_rval == GL_INVALID_OPERATION)
                    {
                        read_format_bgra_valid = false;
                        logger::log_error()
                            << "GL_EXT_read_format_BGRA is not be implemented";
                    }
                }
                else
                {
                    read_format_bgra_valid = false;
                }
            }

            if (!read_format_bgra_valid)
            {
                do
                {
                    gl_extensions.erase(pos, 23);
                } while ((pos = gl_extensions.find("GL_EXT_read_format_bgra",
                                                   pos)) != std::string::npos);
            }
        }

        std::stringstream ss{gl_extensions};
        std::string str{};
        while (ss >> str)
        {
            tokenized_gl_extensions.push_back(str);
        }
    }
}
