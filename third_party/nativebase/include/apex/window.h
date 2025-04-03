/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <nativebase/nativebase.h>

// apex is a superset of the NDK
#include <android/native_window.h>

__BEGIN_DECLS

/*
 * perform bits that can be used with ANativeWindow_perform()
 *
 * This is only to support the intercepting methods below - these should notbe
 * used directly otherwise.
 */
enum ANativeWindowPerform {
    // clang-format off
    ANATIVEWINDOW_PERFORM_SET_USAGE            = 0,
    ANATIVEWINDOW_PERFORM_SET_BUFFERS_GEOMETRY = 5,
    ANATIVEWINDOW_PERFORM_SET_BUFFERS_FORMAT   = 9,
    ANATIVEWINDOW_PERFORM_SET_USAGE64          = 30,
    // clang-format on
};

__END_DECLS
