#ifndef GRALLOC_ADAPTER_NATIVEWINDOW_H_
#define GRALLOC_ADAPTER_NATIVEWINDOW_H_

#include "gralloc_adapter.h"
#include "logger.h"
#include <string.h>
#include <dlfcn.h>

// clang-format off
enum CreateFromHandleMethod {
    // enum values chosen to match internal GraphicBuffer::HandleWrapMethod
    AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_REGISTER = 2,
    AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE = 3,
};

struct nativewindow_vptr {
    // NDK
    int (*AHardwareBuffer_allocate) (const AHardwareBuffer_Desc*, AHardwareBuffer**);
    void (*AHardwareBuffer_acquire) (AHardwareBuffer*);
    void (*AHardwareBuffer_release) (AHardwareBuffer*);
    void (*AHardwareBuffer_describe)(const AHardwareBuffer*, AHardwareBuffer_Desc*);
    int (*AHardwareBuffer_lock) (AHardwareBuffer*, uint64_t, int32_t, const ARect*, void**);
    int (*AHardwareBuffer_unlock) (AHardwareBuffer*, int32_t*);

    // VNDK
    const native_handle_t* (*AHardwareBuffer_getNativeHandle) (const AHardwareBuffer*);
    int (*AHardwareBuffer_createFromHandle) (const AHardwareBuffer_Desc*, buffer_handle_t,
                                             int32_t, AHardwareBuffer**);
};
// clang-format on

void (*dumpAllocationLog)() = nullptr;

#define GETVPTRFUNC(vptr, func)                                                \
    vptr.func = reinterpret_cast<decltype(vptr.func)>(                         \
        dlsym(nativewindow_handle, #func))

class gralloc_nativewindow;
class gralloc_buffer_nativewindow;

class gralloc_nativewindow
    : public gralloc_adapter_t,
      public std::enable_shared_from_this<gralloc_nativewindow> {
    void* nativewindow_handle;
    nativewindow_vptr vptr;
    friend class gralloc_buffer_nativewindow;

  public:
    gralloc_nativewindow(void* nativewindow_handle) :
        nativewindow_handle(nativewindow_handle)
    {
        memset(&vptr, 0, sizeof(vptr));
        logger::log_info() << "impl gralloc by libnativewindow.so, so handle: "
                           << nativewindow_handle;

        GETVPTRFUNC(vptr, AHardwareBuffer_allocate);
        GETVPTRFUNC(vptr, AHardwareBuffer_acquire);
        GETVPTRFUNC(vptr, AHardwareBuffer_release);
        GETVPTRFUNC(vptr, AHardwareBuffer_describe);
        GETVPTRFUNC(vptr, AHardwareBuffer_lock);
        GETVPTRFUNC(vptr, AHardwareBuffer_unlock);
        GETVPTRFUNC(vptr, AHardwareBuffer_getNativeHandle);
        GETVPTRFUNC(vptr, AHardwareBuffer_createFromHandle);

        dumpAllocationLog = reinterpret_cast<decltype(dumpAllocationLog)>(
            dlsym(nativewindow_handle,
                  "_ZN7android13GraphicBuffer26dumpAllocationsToSystemLogEv"));
    }
    ~gralloc_nativewindow()
    {
        memset(&vptr, 0, sizeof(vptr));
        dumpAllocationLog = nullptr;

        dlclose(nativewindow_handle);
    }

    std::shared_ptr<gralloc_buffer>
    allocate_buffer(int width, int height, int format, uint64_t usage) override;
    std::shared_ptr<gralloc_buffer> import_buffer(buffer_handle_t handle,
                                                  int width, int height,
                                                  int stride, int format,
                                                  uint64_t usage) override;
};

#undef GETVPTRFUNC

class gralloc_buffer_nativewindow : public gralloc_buffer {
    std::shared_ptr<gralloc_nativewindow> adapter;
    struct AHardwareBuffer* ahb;
    bool locked;

    friend class gralloc_nativewindow;

  public:
    int lock(uint64_t usage, const ARect& rect, void** vaddr) override
    {
        int rval =
            adapter->vptr.AHardwareBuffer_lock(ahb, usage, -1, &rect, vaddr);
        if (rval == 0)
            locked = true;
        return rval;
    }

    int unlock() override
    {
        int fance = -1;
        int rval = adapter->vptr.AHardwareBuffer_unlock(ahb, &fance);
        if (rval == 0)
            locked = false;
        return rval;
    }

    virtual ~gralloc_buffer_nativewindow()
    {
        if (!ahb)
            return;

        if (locked)
            unlock();

        logger::log_info() << "delete buffer: " << ahb;
        adapter->vptr.AHardwareBuffer_release(ahb);
        ahb = nullptr;
    }
};

inline std::shared_ptr<gralloc_buffer>
gralloc_nativewindow::allocate_buffer(int width, int height, int format,
                                      uint64_t usage)
{
    int rval = 0;
    auto buf = std::make_shared<gralloc_buffer_nativewindow>();
    buf->adapter = shared_from_this();
    buf->layerCount = 1;

    AHardwareBuffer_Desc desc = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .layers = static_cast<uint32_t>(buf->layerCount),
        .format = static_cast<uint32_t>(format),
        .usage = usage,
    };
    rval = vptr.AHardwareBuffer_allocate(&desc, &buf->ahb);
    dumpAllocationLog();

    if (rval != 0)
    {
        logger::log_error() << "allocate buffer failed, errno: " << rval;
        return nullptr;
    }

    auto new_handle = vptr.AHardwareBuffer_getNativeHandle(buf->ahb);
    logger::log_info() << "get buffer: " << (void*)buf->ahb
                       << ", handle: " << (void*)new_handle
                       << ", width: " << width << ", height: " << height
                       << std::showbase << std::hex << ", format: " << format
                       << ", usage:" << usage;

    vptr.AHardwareBuffer_describe(buf->ahb, &desc);
    buf->handle = new_handle;
    buf->width = desc.width;
    buf->height = desc.height;
    buf->format = desc.format;
    buf->stride = desc.stride;
    buf->usage = desc.usage;
    return buf;
}

inline std::shared_ptr<gralloc_buffer>
gralloc_nativewindow::import_buffer(buffer_handle_t handle, int width,
                                    int height, int stride, int format,
                                    uint64_t usage)
{
    int rval = 0;
    auto buf = std::make_shared<gralloc_buffer_nativewindow>();
    buf->adapter = shared_from_this();
    buf->layerCount = 1;

    AHardwareBuffer_Desc desc = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .layers = static_cast<uint32_t>(buf->layerCount),
        .format = static_cast<uint32_t>(format),
        .usage = usage,
        .stride = static_cast<uint32_t>(stride),
    };

    rval = vptr.AHardwareBuffer_createFromHandle(
        &desc, handle, AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE,
        &buf->ahb);

    if (rval != 0)
    {
        logger::log_error() << "import buffer failed, errno: " << rval;
        return nullptr;
    }

    auto new_handle = vptr.AHardwareBuffer_getNativeHandle(buf->ahb);
    logger::log_info() << "get buffer: " << (void*)buf->ahb
                       << ", handle: " << (void*)new_handle
                       << ", width: " << width << ", height: " << height
                       << ", stride: " << stride << std::showbase << std::hex
                       << ", format: " << format << ", usage: " << usage;

    vptr.AHardwareBuffer_describe(buf->ahb, &desc);
    buf->handle = new_handle;
    buf->width = desc.width;
    buf->height = desc.height;
    buf->format = desc.format;
    buf->stride = desc.stride;
    buf->usage = desc.usage;
    return buf;
}

#endif // GRALLOC_ADAPTER_NATIVEWINDOW_H_
