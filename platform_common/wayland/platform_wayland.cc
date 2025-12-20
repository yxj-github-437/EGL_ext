#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <wayland-client-protocol-core.h>

#include "platform_wayland.h"

namespace {
// copy from https://github.com/NVIDIA/egl-wayland.git
bool check_memory_is_readable(const void* p, size_t len)
{
    int fds[2], result = -1;
    if (pipe(fds) == -1)
        return false;

    if (fcntl(fds[1], F_SETFL, O_NONBLOCK) == -1)
    {
        goto done;
    }

    result = write(fds[1], p, len);
    assert(result != -1 || errno != EFAULT);

done:
    close(fds[0]);
    close(fds[1]);
    return result != -1;
}
}

bool check_wayland_display(struct wl_display* display)
{
    if (!check_memory_is_readable(display, sizeof(void*)))
        return false;

    const char name[] = "wl_display";
    auto interface = static_cast<struct wl_interface*>(*(void**)display);
    return interface == &wl_display_interface ||
           (check_memory_is_readable(interface->name, sizeof(name)) &&
            memcmp(interface->name, name, sizeof(name)) == 0);
}
