#ifndef EGL_PLATFORM_WAYLAND_H_
#define EGL_PLATFORM_WAYLAND_H_

#include <deque>
#include <list>
#include <wayland-server-protocol-core.h>
#include <wayland-client-protocol-core.h>
#include <wayland-android-server-protocol-core.h>
#include <wayland-android-client-protocol-core.h>
#include <wayland-egl.h>
#include <EGL/egl.h>

#include "gralloc_adapter.h"
#include "platform_base.h"

#include <mutex>

class WaylandNativeWindowBuffer;
class WaylandNativeWindow : public EGLBaseNativeWindow {
  public:
    WaylandNativeWindow(struct wl_display* display, struct wl_egl_window* win,
                        struct android_wlegl* wlegl);
    ~WaylandNativeWindow();

    void resize(uint32_t width, uint32_t height);
    void releaseBuffer(struct wl_buffer* buf);

    virtual int setSwapInterval(int interval) override;
    void prepare_swap(EGLint* damage_rects, EGLint damage_n_rects);
    void finish_swap();

    struct wl_event_queue* event_queue;
    bool valid;

  protected:
    virtual int dequeueBuffer(BaseNativeWindowBuffer** buffer,
                              int* fenceFd) override;
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer,
                            int fenceFd) override;
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer,
                             int fenceFd) override;
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer) override;

    virtual uint32_t type() const override;
    virtual uint32_t width() const override;
    virtual uint32_t height() const override;
    virtual uint32_t format() const override;
    virtual uint32_t defaultWidth() const override;
    virtual uint32_t defaultHeight() const override;
    virtual uint32_t queueLength() const override;
    virtual uint32_t transformHint() const override;
    virtual uint32_t getUsage() const override;
    // perform interfaces
    virtual int setBuffersFormat(int format) override;
    virtual int setBuffersDimensions(int width, int height) override;
    virtual int setUsage(uint64_t usage) override;
    virtual int setBufferCount(int cnt) override;

  private:
    void removeAllBuffers();

    mutable std::mutex m_mutex;
    struct wl_display* m_display;
    struct wl_display* m_display_wrapper;
    struct wl_egl_window* m_window;
    struct android_wlegl* m_wlegl_wrapper;
    struct wl_surface* m_surface_wrapper;

    std::list<egl_wrap::sp<WaylandNativeWindowBuffer>> m_bufList;
    std::list<WaylandNativeWindowBuffer*> posted;
    std::deque<WaylandNativeWindowBuffer*> queue;
    WaylandNativeWindowBuffer* m_lastBuffer;

    int m_width;
    int m_height;
    int m_format;
    uint32_t m_defaultWidth;
    uint32_t m_defaultHeight;
    uint64_t m_usage;
    int m_swap_interval;

    int m_bufCount;
    EGLint *m_damage_rects, m_damage_n_rects;
    struct wl_callback* throttle_callback;
    static struct wl_callback_listener throttle_listener;
};

class WaylandNativeWindowBuffer : public BaseNativeWindowBuffer {
    friend class WaylandNativeWindow;

    std::shared_ptr<gralloc_buffer> m_buffer;
    struct wl_buffer* wlbuffer;
    bool busy; // this buffer is being used;
    bool youngest; // this buffer isn't rendered

  protected:
    WaylandNativeWindowBuffer(uint32_t width, uint32_t height, uint32_t format,
                              uint64_t usage)
    {
        ANativeWindowBuffer::width = width;
        ANativeWindowBuffer::height = height;
        ANativeWindowBuffer::format = format;
        ANativeWindowBuffer::usage_deprecated = usage;
        ANativeWindowBuffer::usage = usage;
        auto adapter = gralloc_loader::getInstance().get_adapter();
        m_buffer = adapter->allocate_buffer(width, height, format, usage);
        if (m_buffer)
        {
            ANativeWindowBuffer::handle = m_buffer->handle;
            ANativeWindowBuffer::stride = m_buffer->stride;
        }
        busy = false;
//        youngest = true;
        wlbuffer = nullptr;
    }
    void create_wl_buffer(struct wl_display* display,
                          struct android_wlegl* wlegl,
                          struct wl_event_queue* queue);

  public:
    ~WaylandNativeWindowBuffer();
};

class wayland_wrapper_t : public platform_wrapper_t {
    struct wl_display* display;
    struct wl_display* display_wrapper;
    bool own_display;
    struct wl_event_queue* event_queue;
    struct wl_registry* registry;
    struct android_wlegl* wlegl;

  public:
    wayland_wrapper_t(struct wl_display* display);
    ~wayland_wrapper_t();

    virtual EGLBoolean initialize() override;
    virtual ANativeWindow* create_window(void* native_window) override;
    virtual void destroy_window(ANativeWindow* win) override;
    virtual void prepare_swap(ANativeWindow* win, EGLint* rects,
                              EGLint n_rects) override;
    virtual void finish_swap(ANativeWindow* win) override;
};

// form server
// in wayland server, create from struct wl_resource* buffer
class RemoteWindowBuffer : public BaseNativeWindowBuffer {
    std::shared_ptr<gralloc_buffer> const m_buffer;

  public:
    RemoteWindowBuffer(std::shared_ptr<gralloc_buffer> buffer) :
        m_buffer(std::move(buffer))
    {
        width = m_buffer->width;
        height = m_buffer->height;
        stride = m_buffer->stride;
        format = m_buffer->format;
        usage = m_buffer->usage;
        ANativeWindowBuffer::handle = m_buffer->handle;
    }
};

bool check_wayland_display(struct wl_display* display);
struct server_wlegl
{
    struct wl_display* display;
    struct wl_global* global;
};
struct server_wlegl* create_server_wlegl(struct wl_display* display);
void delete_server_wlegl(struct server_wlegl* wlegl);
EGLClientBuffer get_buffer_from_resource(struct wl_resource* resource);

#endif // EGL_PLATFORM_WAYLAND_H_
