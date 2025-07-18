#include "platform_wayland.h"
#include "gralloc_adapter.h"
#include "logger.h"
#include "wayland-android-server-protocol-core.h"
#include "wayland-server-protocol-core.h"

#include <assert.h>
#include <fcntl.h>
#include <android/hardware_buffer.h>
#include <sync/sync.h>
#include <wayland-egl-backend.h>
#include <EGL/egl.h>
#include <mutex>

namespace {
struct wl_buffer_listener wlbuffer_listener = {
    // clang-format off
    .release = +[](void* data, struct wl_buffer* buffer) {
        auto window = static_cast<WaylandNativeWindow*>(data);
        window->releaseBuffer(buffer);
    },
    // clang-format on
};

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
} // namespace

WaylandNativeWindow::WaylandNativeWindow(struct wl_display* display,
                                         struct wl_egl_window* win,
                                         struct android_wlegl* wlegl) :
    m_window(win),
    m_width(win->width),
    m_height(win->height),
    m_defaultWidth(win->width),
    m_defaultHeight(win->height),
    m_usage(AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
            AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER),
    m_format(AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM),
    m_damage_rects(nullptr),
    m_damage_n_rects(0),
    m_lastBuffer(nullptr),
    throttle_callback(nullptr),
    m_display(display),
    m_display_wrapper((struct wl_display*)wl_proxy_create_wrapper(display)),
    m_wlegl_wrapper((struct android_wlegl*)wl_proxy_create_wrapper(wlegl)),
    m_surface_wrapper(
        (struct wl_surface*)wl_proxy_create_wrapper(win->surface)),
    event_queue(
        wl_display_create_queue_with_name(display, "egl surface queue")),
    valid(true)
{
    wl_proxy_set_queue((struct wl_proxy*)m_display_wrapper, event_queue);
    wl_proxy_set_queue((struct wl_proxy*)m_wlegl_wrapper, event_queue);
    wl_proxy_set_queue((struct wl_proxy*)m_surface_wrapper, event_queue);
    if (wl_display_roundtrip(display) < 0)
    {
        valid = false;
        return;
    }

    m_window->driver_private = this;
    m_window->resize_callback = +[](struct wl_egl_window* window, void*) {
        auto self = static_cast<WaylandNativeWindow*>(window->driver_private);
        self->resize(window->width, window->height);
    };
    m_window->destroy_window_callback = +[](void* data) {
        auto self = static_cast<WaylandNativeWindow*>(data);
        std::lock_guard lock{self->m_mutex};
        self->m_window = nullptr;
    };

    setBufferCount(3);
}

WaylandNativeWindow::~WaylandNativeWindow()
{
    removeAllBuffers();

    if (throttle_callback)
    {
        wl_callback_destroy(throttle_callback);
        throttle_callback = nullptr;
    }

    if (m_window)
    {
        m_window->driver_private = nullptr;
        m_window->resize_callback = nullptr;
        m_window->destroy_window_callback = nullptr;
    }

    if (m_surface_wrapper)
    {
        wl_proxy_wrapper_destroy(m_surface_wrapper);
        m_surface_wrapper = nullptr;
    }
    if (m_wlegl_wrapper)
    {
        wl_proxy_wrapper_destroy(m_wlegl_wrapper);
        m_wlegl_wrapper = nullptr;
    }
    if (m_display_wrapper)
    {
        wl_proxy_wrapper_destroy(m_display_wrapper);
        m_display_wrapper = nullptr;
    }
    if (event_queue)
    {
        wl_event_queue_destroy(event_queue);
        event_queue = nullptr;
    }
}

void WaylandNativeWindow::resize(uint32_t width, uint32_t height)
{
    std::lock_guard lock{m_mutex};

    m_defaultWidth = m_width = width;
    m_defaultHeight = m_height = height;
}

void WaylandNativeWindow::releaseBuffer(struct wl_buffer* buf)
{
    std::lock_guard lock{m_mutex};
    auto iter = posted.begin();
    for (; iter != posted.end(); iter++)
    {
        if ((*iter)->wlbuffer == buf)
            break;
    }

    if (iter != posted.end())
    {
        auto native_buffer = *iter;
        posted.erase(iter);
        native_buffer->busy = false;
        return;
    }

    auto buf_iter = m_bufList.begin();
    for (; buf_iter != m_bufList.end(); buf_iter++)
    {
        if ((*buf_iter)->wlbuffer == buf)
            break;
    }
    if (buf_iter == m_bufList.end())
        return;

    auto& native_buffer = *buf_iter;
    native_buffer->busy = false;
}

int WaylandNativeWindow::setSwapInterval(int interval)
{
    if (interval < 0)
        interval = 0;
    if (interval > 1)
        interval = 1;

    std::lock_guard lock{m_mutex};
    m_swap_interval = interval;
    return 0;
}

struct wl_callback_listener WaylandNativeWindow::throttle_listener = {
    // clang-format off
    .done = +[](void* data, struct wl_callback* callback, uint32_t) {
        auto window = static_cast<WaylandNativeWindow*>(data);
        window->throttle_callback = nullptr;
        wl_callback_destroy(callback);
    },
    // clang-format on
};

void WaylandNativeWindow::prepare_swap(const EGLint* damage_rects,
                                       EGLint damage_n_rects)
{
    std::lock_guard lock{m_mutex};
    m_damage_rects = damage_rects;
    m_damage_n_rects = damage_n_rects;
}

void WaylandNativeWindow::finish_swap()
{
    std::unique_lock lock{m_mutex};
    if (!m_window)
        return;

    auto native_buffer = queue.front();
    if (!native_buffer)
    {
        native_buffer = m_lastBuffer;
    }
    else
    {
        queue.pop_front();
    }

    m_lastBuffer = native_buffer;
    native_buffer->busy = true;

    lock.unlock();
    while (throttle_callback)
    {
        if (wl_display_dispatch_queue(m_display, event_queue) == -1)
            return;
    }

    if (native_buffer->wlbuffer == nullptr)
    {
        native_buffer->create_wl_buffer(m_display_wrapper, m_wlegl_wrapper,
                                        event_queue);
        wl_buffer_add_listener(native_buffer->wlbuffer, &wlbuffer_listener,
                               this);
        wl_proxy_set_queue((struct wl_proxy*)native_buffer->wlbuffer,
                           event_queue);
    }

    if (m_swap_interval > 0)
    {
        throttle_callback = wl_surface_frame(m_surface_wrapper);
        wl_callback_add_listener(throttle_callback, &throttle_listener, this);
        wl_proxy_set_queue((struct wl_proxy*)throttle_callback, event_queue);
    }

    wl_surface_attach(m_surface_wrapper, native_buffer->wlbuffer, 0, 0);
    if (m_damage_n_rects != 0 && m_damage_rects != nullptr)
    {
        for (int i = 0; i < m_damage_n_rects; i++)
        {
            const EGLint* rect = &m_damage_rects[4 * i];
            int inv_y = native_buffer->height - (rect[1] + rect[3]);
            wl_surface_damage(m_surface_wrapper, rect[0], inv_y, rect[2],
                              rect[3]);
        }
    }
    else
    {
        wl_surface_damage(m_surface_wrapper, 0, 0, INT32_MAX, INT32_MAX);
    }
    wl_surface_commit(m_surface_wrapper);

    if (throttle_callback == nullptr)
    {
        throttle_callback = wl_display_sync(m_display_wrapper);
        wl_callback_add_listener(throttle_callback, &throttle_listener, this);
    }

    wl_display_flush(m_display);

    lock.lock();
    posted.push_back(native_buffer);

    m_window->attached_width = native_buffer->width;
    m_window->attached_height = native_buffer->height;

    m_damage_rects = nullptr;
    m_damage_n_rects = 0;
}

int WaylandNativeWindow::dequeueBuffer(BaseNativeWindowBuffer** buffer,
                                       int* fenceFd)
{
    if (fenceFd)
    {
        *fenceFd = -1;
    }

    wl_display_dispatch_queue_pending(m_display, event_queue);

    std::unique_lock lock{m_mutex};
    auto iter = m_bufList.begin();
    for (; iter != m_bufList.begin(); iter++)
    {
        if ((*iter)->busy)
            continue;
        break;
    }

    while (iter == m_bufList.end())
    {
        if (m_bufList.size() < m_bufCount)
        {
            m_bufList.push_back(new WaylandNativeWindowBuffer(
                m_width, m_height, m_format, m_usage));
            iter = --m_bufList.end();
            break;
        }

        for (iter = m_bufList.begin(); iter != m_bufList.end(); iter++)
        {
            if ((*iter)->busy)
                continue;
            break;
        }

        if (iter == m_bufList.end())
        {
            lock.unlock();
            if (wl_display_dispatch_queue(m_display, event_queue) == -1)
            {
                logger::log_error() << "waiting for a free buffer failed";
                return -1;
            }
            lock.lock();
        }
    }

    auto& native_buffer = *iter;
    if (native_buffer->width != m_width || native_buffer->height != m_height ||
        native_buffer->format != m_format || native_buffer->usage != m_usage)
    {
        native_buffer =
            new WaylandNativeWindowBuffer(m_width, m_height, m_format, m_usage);
    }

    *buffer = native_buffer;
    native_buffer->busy = true;
    native_buffer->youngest = true;
    queue.push_back(native_buffer);
    return NO_ERROR;
}

int WaylandNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer,
                                     int fenceFd)
{
    std::lock_guard lock{m_mutex};
    if (fenceFd >= 0)
    {
        sync_wait(fenceFd, -1);
        close(fenceFd);
    }
    return NO_ERROR;
}

int WaylandNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer,
                                      int fenceFd)
{
    return 0;
}

int WaylandNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer)
{
    return NO_ERROR;
}

uint32_t WaylandNativeWindow::type() const
{
    return NATIVE_WINDOW_SURFACE;
}

uint32_t WaylandNativeWindow::width() const
{
    std::lock_guard lock{m_mutex};
    return m_width;
}

uint32_t WaylandNativeWindow::height() const
{
    std::lock_guard lock{m_mutex};
    return m_height;
}

uint32_t WaylandNativeWindow::format() const
{
    std::lock_guard lock{m_mutex};
    return m_format;
}

uint32_t WaylandNativeWindow::defaultWidth() const
{
    std::lock_guard lock{m_mutex};
    return m_defaultWidth;
}

uint32_t WaylandNativeWindow::defaultHeight() const
{
    std::lock_guard lock{m_mutex};
    return m_defaultHeight;
}

uint32_t WaylandNativeWindow::queueLength() const
{
    return 1;
}

uint32_t WaylandNativeWindow::transformHint() const
{
    return 0;
}

uint32_t WaylandNativeWindow::getUsage() const
{
    std::lock_guard lock{m_mutex};
    return m_usage;
}

int WaylandNativeWindow::setBuffersFormat(int format)
{
    std::lock_guard lock{m_mutex};
    if (m_format != format)
    {
        m_format = format;
    }
    return NO_ERROR;
}

int WaylandNativeWindow::setBuffersDimensions(int width, int height)
{
    std::lock_guard lock{m_mutex};

    if (m_width != width || m_height != height)
    {
        m_width = width;
        m_height = height;
    }

    return NO_ERROR;
}

int WaylandNativeWindow::setUsage(uint64_t usage)
{
    std::lock_guard lock{m_mutex};
    if ((usage | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE) != m_usage)
    {
        m_usage = usage | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    }

    return NO_ERROR;
}

int WaylandNativeWindow::setBufferCount(int cnt)
{
    std::lock_guard lock{m_mutex};
    m_bufCount = cnt;

    if ((int)m_bufList.size() > cnt)
    {
        /* Decreasing buffer count, remove from beginning */
        for (int i = 0; i <= (int)m_bufList.size() - cnt; i++)
        {
            m_bufList.pop_front();
        }
    }
    return NO_ERROR;
}

void WaylandNativeWindow::removeAllBuffers()
{
    m_bufList.clear();
}

void WaylandNativeWindowBuffer::create_wl_buffer(struct wl_display* display,
                                                 struct android_wlegl* wlegl,
                                                 struct wl_event_queue* queue)
{
    struct android_wlegl_handle* wlegl_handle = nullptr;
    struct wl_array ints;
    wl_array_init(&ints);
    void* ints_data = wl_array_add(&ints, handle->numInts * sizeof(int));
    memcpy(ints_data, &handle->data[handle->numFds],
           handle->numInts * sizeof(int));
    wlegl_handle = android_wlegl_create_handle(wlegl, handle->numFds, &ints);
    wl_array_release(&ints);

    if (!wlegl_handle)
        return;

    for (int i = 0; i < handle->numFds; i++)
    {
        android_wlegl_handle_add_fd(wlegl_handle, handle->data[i]);
    }

    wlbuffer = android_wlegl_create_buffer(wlegl, width, height, stride, format,
                                           usage, wlegl_handle);
    android_wlegl_handle_destroy(wlegl_handle);
    wlegl_handle = nullptr;

    if (!wlbuffer)
        return;

    wl_proxy_set_queue((struct wl_proxy*)wlbuffer, queue);
}

WaylandNativeWindowBuffer::~WaylandNativeWindowBuffer()
{
    if (wlbuffer)
    {
        wl_buffer_destroy(wlbuffer);
        wlbuffer = nullptr;
    }
}

// for server
wayland_wrapper_t::wayland_wrapper_t(struct wl_display* display) :
    display(display),
    display_wrapper(nullptr),
    own_display(false),
    event_queue(nullptr),
    registry(nullptr),
    wlegl(nullptr)
{
}

wayland_wrapper_t::~wayland_wrapper_t()
{
    wayland_wrapper_t::terminate();
}

EGLBoolean wayland_wrapper_t::initialize()
{
    if (!display)
    {
        display = wl_display_connect(nullptr);
        own_display = true;
        if (!display)
        {
            logger::log_error() << "cannot connect wayland display";
            return EGL_FALSE;
        }
    }

    event_queue =
        wl_display_create_queue_with_name(display, "egl display queue");
    display_wrapper =
        static_cast<struct wl_display*>(wl_proxy_create_wrapper(display));

    wl_proxy_set_queue((struct wl_proxy*)display_wrapper, event_queue);
    if (own_display)
    {
        wl_display_dispatch_pending(display);
    }
    registry = wl_display_get_registry(display_wrapper);

    static const wl_registry_listener registry_listener = {
        // clang-format off
        .global = +[] (void *data, struct wl_registry *wl_registry,
                       uint32_t name, const char *interface, uint32_t version) {
            auto self = static_cast<wayland_wrapper_t*>(data);

            if (strcmp(interface, "android_wlegl") == 0) {
                self->wlegl = static_cast<struct android_wlegl*>(wl_registry_bind(wl_registry, name,
                    &android_wlegl_interface, std::min(1U, version)));
            }
        },
        // clang-format on
    };
    wl_registry_add_listener(registry, &registry_listener, this);
    if (wl_display_roundtrip_queue(display, event_queue) < 0 ||
        wlegl == nullptr)
    {
        logger::log_error() << "cannot find android_wlegl";
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLBoolean wayland_wrapper_t::terminate()
{
    if (wlegl)
    {
        android_wlegl_destroy(wlegl);
        wlegl = nullptr;
    }
    if (registry)
    {
        wl_registry_destroy(registry);
        registry = nullptr;
    }

    if (display_wrapper)
    {
        wl_proxy_wrapper_destroy(display_wrapper);
        display_wrapper = nullptr;
    }
    if (event_queue)
    {
        wl_event_queue_destroy(event_queue);
        event_queue = nullptr;
    }

    if (own_display && display)
    {
        wl_display_disconnect(display);
        display = nullptr;
    }

    return EGL_TRUE;
}

ANativeWindow* wayland_wrapper_t::create_window(void* native_window)
{
    auto window = static_cast<EGLNativeWindowType>(native_window);
    WaylandNativeWindow* wayland_window =
        new WaylandNativeWindow{display, window, wlegl};
    if (!wayland_window->valid)
    {
        delete wayland_window;
        return nullptr;
    }

    wayland_window->common.incRef(&wayland_window->common);
    return wayland_window;
}

void wayland_wrapper_t::destroy_window(ANativeWindow* win)
{
    win->common.decRef(&win->common);
}

void wayland_wrapper_t::prepare_swap(ANativeWindow* win, const EGLint* rects,
                                     EGLint n_rects)
{
    auto wayland_window = static_cast<WaylandNativeWindow*>(win);
    wayland_window->prepare_swap(rects, n_rects);
}

void wayland_wrapper_t::finish_swap(ANativeWindow* win)
{
    auto wayland_window = static_cast<WaylandNativeWindow*>(win);
    wayland_window->finish_swap();
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

// server wl_egl impl
namespace {
struct wlegl_handle
{
    struct wl_resource* resource;
    std::vector<int> ints;
    std::vector<int> fds;
    int num_ints;
    int num_fds;
    native_handle_t* native_handle = nullptr;

    ~wlegl_handle()
    {
        for (auto fd : fds)
        {
            close(fd);
        }

        if (native_handle)
        {
            native_handle_close(native_handle);
            native_handle_delete(native_handle);
        }
    }
    native_handle_t* get_native_buffer()
    {
        if (native_handle)
            return native_handle;

        native_handle = native_handle_create(num_fds, num_ints);
        memcpy(&native_handle->data[0], fds.data(), fds.size() * sizeof(int));
        memcpy(&native_handle->data[num_fds], ints.data(),
               ints.size() * sizeof(int));
        return native_handle;
    }
    static wlegl_handle* from(struct wl_resource* handle);
};

struct android_wlegl_handle_interface wlegl_handle_impl = {
    // clang-format off
    .add_fd = +[](struct wl_client *client, struct wl_resource *resource,
                  int32_t fd) {
        auto self = static_cast<wlegl_handle*>(wl_resource_get_user_data(resource));
        if (self->fds.size() >= self->num_fds) {
            close(fd);
            wl_resource_post_error(resource, ANDROID_WLEGL_HANDLE_ERROR_TOO_MANY_FDS,
                                   "too many fd");
            return;
        }
        self->fds.push_back(fd);
    },
    .destroy = +[](struct wl_client *client, struct wl_resource *resource) {
        wl_resource_destroy(resource);
    },
    // clang-format on
};

wlegl_handle* wlegl_handle::from(struct wl_resource* handle)
{
    if (wl_resource_instance_of(handle, &android_wlegl_handle_interface,
                                &wlegl_handle_impl))
        return static_cast<wlegl_handle*>(wl_resource_get_user_data(handle));
    else
        return nullptr;
}

struct wlegl_buffer
{
    struct wl_resource* resource;
    server_wlegl* wlegl;
    egl_wrap::sp<RemoteWindowBuffer> buf;

    ~wlegl_buffer() = default;
    static wlegl_buffer* from(struct wl_resource* buffer);
};

struct wl_buffer_interface wlegl_buffer_impl = {
    // clang-format off
    .destroy = +[](struct wl_client *client, struct wl_resource *resource) {
        wl_resource_destroy(resource);
    },
    // clang-format on
};

wlegl_buffer* wlegl_buffer::from(struct wl_resource* resource)
{
    if (wl_resource_instance_of(resource, &wl_buffer_interface,
                                &wlegl_buffer_impl))
        return static_cast<wlegl_buffer*>(wl_resource_get_user_data(resource));
    else
        return nullptr;
}

struct android_wlegl_interface server_wlegl_impl = {
    // clang-format off
    .create_handle = +[](struct wl_client *client, struct wl_resource *resource,
                         uint32_t id, int32_t num_fds,
                         struct wl_array *ints) {
        if (num_fds < 0) {
            wl_resource_post_error(resource, ANDROID_WLEGL_ERROR_BAD_VALUE,
                                    "num_fds %d is invalid.", num_fds);
            return;
        }
        auto handle = new wlegl_handle{};
        handle->num_fds = num_fds;
        handle->num_ints = ints->size / sizeof(int);
        handle->ints.resize(handle->num_ints);
        memcpy(handle->ints.data(), ints->data, handle->ints.size() * sizeof(int));
        handle->resource = wl_resource_create(client, &android_wlegl_handle_interface,
                                              android_wlegl_interface.version, id);
        wl_resource_set_implementation(handle->resource, &wlegl_handle_impl, handle,
                                       +[](struct wl_resource *resource) {
            delete static_cast<wlegl_handle*>(wl_resource_get_user_data(resource));
        });
    },
    .create_buffer = +[](struct wl_client *client, struct wl_resource *resource,
                         uint32_t id, int32_t width, int32_t height, int32_t stride,
                         int32_t format, int32_t usage,
                         struct wl_resource* native_handle) {
        auto self = static_cast<server_wlegl*>(wl_resource_get_user_data(resource));
        auto handle = wlegl_handle::from(native_handle);
        auto buffer = new wlegl_buffer{};
        auto adapter = gralloc_loader::getInstance().get_adapter();
        auto out_buffer = adapter->import_buffer(handle->get_native_buffer(),
                                                 width, height, stride, format, usage);
        if (!out_buffer) {
            delete buffer;
            return;
        }
        buffer->buf = new RemoteWindowBuffer{out_buffer};
        buffer->wlegl = self;
        buffer->resource = wl_resource_create(client, &wl_buffer_interface, wl_buffer_interface.version, id);
        wl_resource_set_implementation(buffer->resource, &wlegl_buffer_impl, buffer,
                                       +[](struct wl_resource *resource) {
            delete static_cast<wlegl_buffer*>(wl_resource_get_user_data(resource));
        });
    },
    // clang-format on
};
} // namespace

struct server_wlegl* create_server_wlegl(struct wl_display* display)
{
    auto wlegl = new server_wlegl{};
    wlegl->display = display;
    // clang-format off
    wlegl->global = wl_global_create(display,
        &android_wlegl_interface, android_wlegl_interface.version,
        wlegl, +[](struct wl_client* client, void* data, uint32_t version,
                              uint32_t id) {
        auto wlegl = static_cast<server_wlegl*>(data);
        auto resource = wl_resource_create(client, &android_wlegl_interface, version, id);
        wl_resource_set_implementation(resource, &server_wlegl_impl, wlegl, nullptr);
    });
    // clang-format on

    return wlegl;
}

void delete_server_wlegl(struct server_wlegl* wlegl)
{
    wl_global_destroy(wlegl->global);
    delete wlegl;
}

EGLClientBuffer get_buffer_from_resource(struct wl_resource* resource)
{
    auto buffer = wlegl_buffer::from(resource);
    return buffer->buf->getNativeBuffer();
}
