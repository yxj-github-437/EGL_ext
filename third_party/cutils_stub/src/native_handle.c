/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <cutils/native_handle.h>

native_handle_t* native_handle_create(int numFds, int numInts) {
    native_handle_t* h =
        malloc(sizeof(native_handle_t) + sizeof(int) * (numFds + numInts));

    h->version = sizeof(native_handle_t);
    h->numFds = numFds;
    h->numInts = numInts;
    return h;
}

int native_handle_delete(native_handle_t* h) {
    if (h) {
        if (h->version != sizeof(native_handle_t))
            return -EINVAL;
        free(h);
    }
    return 0;
}

int native_handle_close(const native_handle_t* h) {
    if (h->version != sizeof(native_handle_t))
        return -EINVAL;

    for (int i = 0; i < h->numFds; i++) {
        close(h->data[i]);
    }
    return 0;
}

native_handle_t* native_handle_clone(const native_handle_t* handle) {
    native_handle_t* clone =
        native_handle_create(handle->numFds, handle->numInts);
    if (clone == NULL)
        return NULL;

    for (int i = 0; i < handle->numFds; i++) {
        clone->data[i] = dup(handle->data[i]);
        if (clone->data[i] == -1) {
            clone->numFds = i;
            native_handle_close(clone);
            native_handle_delete(clone);
            return NULL;
        }
    }

    memcpy(&clone->data[handle->numFds], &handle->data[handle->numFds],
           sizeof(int) * handle->numInts);

    return clone;
}
