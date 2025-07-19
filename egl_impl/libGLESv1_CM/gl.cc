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

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "logger.h"
#include "loader/loader.h"
#include "platform/egl_tls.h"

using namespace egl_wrapper;

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
#include "gl_api.in"
// #include "glext_api.in"
#pragma GCC diagnostic warning "-Wunused-parameter"
}

#undef API_ENTRY
#undef CALL_GL_API
#undef CALL_GL_API_INTERNAL_CALL
#undef CALL_GL_API_INTERNAL_SET_RETURN_VALUE
#undef CALL_GL_API_INTERNAL_DO_RETURN
#undef CALL_GL_API_RETURN

/*
 * glGetString() is special because we expose some extensions in the wrapper
 */

extern "C" const GLubyte* __glGetString(GLenum name);

const GLubyte* glGetString(GLenum name)
{
    auto system = egl_get_system();
    return system->platform.glGetString(name);
}
