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

#ifndef GLES_CM_HOOKS_H_
#define GLES_CM_HOOKS_H_

#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>

struct ANativeWindow;

// ----------------------------------------------------------------------------
namespace egl_wrapper {
// ----------------------------------------------------------------------------

// GL / EGL hooks
#undef GL_ENTRY
#undef EGL_ENTRY
#define GL_ENTRY(_r, _api, ...) _r (*(_api))(__VA_ARGS__);
#define EGL_ENTRY(_r, _api, ...) _r (*(_api))(__VA_ARGS__);

// clang-format off
struct platform_impl_t {
    #include "platform_entries.in"
};

#define EGLNativeWindowType struct ANativeWindow*
#define NativeWindowType struct ANativeWindow*
struct egl_t {
    #include "egl_entries.in"
    struct {
        #include "egl_ext_entries.in"
    } ext;
};
#undef NativeWindowType
#undef EGLNativeWindowType

struct gl_hooks_t {
    struct gl_t {
        #include "entries.in"
    } gl;
    struct gl_ext_t {
        #include "gles_ext_entries.in"
    };
};
// clang-format on

#undef GL_ENTRY
#undef EGL_ENTRY
// ----------------------------------------------------------------------------
}; // namespace wrapper
// ----------------------------------------------------------------------------

#endif /* ANDROID_GLES_CM_HOOKS_H */
