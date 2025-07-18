#ifndef EGL_PLATFORM_BASE_H_
#define EGL_PLATFORM_BASE_H_

#include <system/window.h>
#include <android/native_window.h>
#include <atomic>
#include <EGL/egl.h>

#define NO_ERROR (0)
#define BAD_VALUE (-1)

namespace egl_wrap {
template <class T>
class sp {
    T* m_ptr;

  public:
    sp() : m_ptr(nullptr) {}
    sp(T* p) : m_ptr(p)
    {
        if (m_ptr)
        {
            m_ptr->incStrong(this);
        }
    }
    ~sp()
    {
        if (m_ptr)
        {
            m_ptr->decStrong(this);
        }
    }
    sp(sp<T>& other) : sp(other.m_ptr) {}
    sp(sp<T>&& other) : sp(other.m_ptr)
    {
        if (other.m_ptr)
        {
            other.m_ptr->decStrong(&other);
            other.m_ptr = nullptr;
        }
    }

    sp& operator=(sp<T>& other)
    {
        if (m_ptr)
        {
            m_ptr->decStrong(this);
        }
        m_ptr = other.m_ptr;
        if (m_ptr)
        {
            m_ptr->incStrong(this);
        }
        return *this;
    }
    sp& operator=(sp<T>&& other)
    {
        if (m_ptr)
        {
            m_ptr->decStrong(this);
        }
        m_ptr = other.m_ptr;
        if (m_ptr)
        {
            m_ptr->incStrong(this);
            other.m_ptr->decStrong(&other);
            other.m_ptr = nullptr;
        }
        return *this;
    }

    operator T*() { return m_ptr; }
    T* operator->() { return m_ptr; }
    operator bool() { return m_ptr; }
};
} // namespace egl_wrap

class BaseNativeWindowBuffer : public ANativeWindowBuffer {
  protected:
    BaseNativeWindowBuffer();
    virtual ~BaseNativeWindowBuffer();

  public:
    ANativeWindowBuffer* getNativeBuffer() const;

  private:
    std::atomic_uint32_t refcount;
};

class BaseNativeWindow : public ANativeWindow {
  protected:
    BaseNativeWindow();
    virtual ~BaseNativeWindow();

    // these have to be implemented in the concrete implementation, eg. FBDEV or
    // offscreen window
    virtual int setSwapInterval(int interval) = 0;

    virtual int dequeueBuffer(BaseNativeWindowBuffer** buffer,
                              int* fenceFd) = 0;
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd) = 0;
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer, int fenceFd) = 0;
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer) = 0; // DEPRECATED

    virtual uint32_t type() const = 0;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual uint32_t format() const = 0;
    virtual uint32_t defaultWidth() const = 0;
    virtual uint32_t defaultHeight() const = 0;
    virtual uint32_t queueLength() const = 0;
    virtual uint32_t transformHint() const = 0;
    virtual uint32_t getUsage() const = 0;
    // perform interfaces
    virtual int setBuffersFormat(int format) = 0;
    virtual int setBuffersDimensions(int width, int height) = 0;
    virtual int setUsage(uint64_t usage) = 0;
    virtual int setBufferCount(int cnt) = 0;

  private:
    std::atomic_uint32_t refcount;

    static int _query(const struct ANativeWindow* window, int what, int* value);
    static int _perform(struct ANativeWindow* window, int operation, ...);
};

class EGLBaseNativeWindow : public BaseNativeWindow {
  public:
    operator ANativeWindow*()
    {
        auto win = static_cast<ANativeWindow*>(this);
        return win;
    }
};

class platform_wrapper_t {
  public:
    virtual ~platform_wrapper_t() = default;

    virtual EGLBoolean initialize() = 0;
    virtual EGLBoolean terminate() = 0;
    virtual ANativeWindow* create_window(void* native_window) = 0;
    virtual void destroy_window(ANativeWindow* win) = 0;
    virtual void prepare_swap(ANativeWindow* win, const EGLint* rects,
                              EGLint n_rects) = 0;
    virtual void finish_swap(ANativeWindow* win) = 0;
};

#endif // EGL_PLATFORM_BASE_H_
