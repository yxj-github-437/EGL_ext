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

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <memory>

#include "logger.h"
#include "loader/loader.h"
#include "platform/egl_tls.h"

using namespace egl_wrapper;

class check_gl_rval {
    std::string func;
    std::stringstream ss{};

    template <typename T, typename... Args>
    inline std::ostream& format_arguments(std::ostream& os, T&& t,
                                          Args&&... args)
    {
        if constexpr (sizeof(T) == 1)
        {
            os << (uint32_t)std::forward<T>(t);
        }
        else if constexpr (std::is_same_v<T, const char*> ||
                           std::is_same_v<T, char*>)
        {
            os << "\"" << std::forward<T>(t) << "\"";
        }
        else
        {
            os << std::forward<T>(t);
        }
        os << ", ";
        return format_arguments(os, std::forward<Args>(args)...);
    }

    inline std::ostream& format_arguments(std::ostream& os) { return os; }

    template <typename T>
    inline std::ostream& format_arguments(std::ostream& os, T&& t)
    {
        if constexpr (sizeof(T) == 1)
        {
            os << (uint32_t)std::forward<T>(t);
        }
        else
        {
            os << std::forward<T>(t);
        }
        return os;
    }

  public:
    template <typename... Args>
    check_gl_rval(std::string func, Args&&... args) : func{func}
    {
        ss << std::showbase << std::hex;
        format_arguments(ss, std::forward<Args>(args)...);
    }
    ~check_gl_rval()
    {
        gl_hooks_t::gl_t const* const gl = &getGlThreadSpecific()->gl;
        logger::log_info() << "call " << func << "(" << ss.str() << ")"
                           << " with glError: " << std::showbase << std::hex
                           << (uint32_t)gl->glGetError();
    }
};

// ----------------------------------------------------------------------------
// Actual GL entry-points
// ----------------------------------------------------------------------------

#undef API_ENTRY
#undef CALL_GL_API
#undef CALL_GL_API_INTERNAL_CALL
#undef CALL_GL_API_INTERNAL_SET_RETURN_VALUE
#undef CALL_GL_API_INTERNAL_DO_RETURN
#undef CALL_GL_API_RETURN

#define API_ENTRY(_api) _api

#define CALL_GL_API_INTERNAL_CALL(_api, ...)                                   \
    gl_hooks_t::gl_t const* const _c = &getGlThreadSpecific()->gl;             \
    /* check_gl_rval ignore{__func__, __VA_ARGS__}; */                         \
    if (_c) [[likely]]                                                         \
        return _c->_api(__VA_ARGS__);                                          \
    else                                                                       \
        logger::log_warn() << __FUNCTION__ << " is not be implemented";

#define CALL_GL_API_INTERNAL_SET_RETURN_VALUE return 0;

// This stays blank, since void functions will implicitly return, and
// all of the other functions will return 0 based on the previous macro.
#define CALL_GL_API_INTERNAL_DO_RETURN

#define CALL_GL_API(_api, ...)                                                 \
    CALL_GL_API_INTERNAL_CALL(_api, __VA_ARGS__)                               \
    CALL_GL_API_INTERNAL_DO_RETURN

#define CALL_GL_API_RETURN(_api, ...)                                          \
    CALL_GL_API_INTERNAL_CALL(_api, __VA_ARGS__)                               \
    CALL_GL_API_INTERNAL_SET_RETURN_VALUE                                      \
    CALL_GL_API_INTERNAL_DO_RETURN

extern "C" {
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "gl2_api.in"
#include "gl2ext_api.in"
#pragma GCC diagnostic warning "-Wunused-parameter"
}

#undef API_ENTRY
#undef CALL_GL_API
#undef CALL_GL_API_INTERNAL_CALL
#undef CALL_GL_API_INTERNAL_SET_RETURN_VALUE
#undef CALL_GL_API_INTERNAL_DO_RETURN
#undef CALL_GL_API_RETURN

const GLubyte* glGetString(GLenum name)
{
    auto system = egl_get_system();
    return system->platform.glGetString(name);
}

const GLubyte* glGetStringi(GLenum name, GLuint index)
{
    auto system = egl_get_system();
    return system->platform.glGetStringi(name, index);
}

void glGetBooleanv(GLenum pname, GLboolean* data)
{
    auto system = egl_get_system();
    return system->platform.glGetBooleanv(pname, data);
}

void glGetFloatv(GLenum pname, GLfloat* data)
{
    auto system = egl_get_system();
    return system->platform.glGetFloatv(pname, data);
}

void glGetIntegerv(GLenum pname, GLint* data)
{
    auto system = egl_get_system();
    return system->platform.glGetIntegerv(pname, data);
}

void glGetInteger64v(GLenum pname, GLint64* data)
{
    auto system = egl_get_system();
    return system->platform.glGetInteger64v(pname, data);
}
