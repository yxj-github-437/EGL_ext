#ifndef GRALLOC_ADAPTER_LIBHARDWARE_H_
#define GRALLOC_ADAPTER_LIBHARDWARE_H_

#include "gralloc_adapter.h"
#include "GrallocUsageConversion.h"
#include "logger.h"

#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include <cutils/native_handle.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>

class gralloc_libhareware;
class gralloc_buffer_libhardware;

namespace wrap {
native_handle_t* native_handle_clone(const native_handle_t* handle)
{
    native_handle_t* clone =
        native_handle_create(handle->numFds, handle->numInts);
    if (clone == NULL)
        return NULL;

    for (int i = 0; i < handle->numFds; i++)
    {
        clone->data[i] = dup(handle->data[i]);
        if (clone->data[i] == -1)
        {
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
} // namespace wrap

// clang-format off
struct gralloc1_vptr
{
    GRALLOC1_PFN_CREATE_DESCRIPTOR      create_descriptor;
    GRALLOC1_PFN_DESTROY_DESCRIPTOR     destroy_descriptor;
    GRALLOC1_PFN_SET_CONSUMER_USAGE     set_consumer_usage;
    GRALLOC1_PFN_SET_DIMENSIONS         set_dimensions;
    GRALLOC1_PFN_SET_FORMAT             set_format;
    GRALLOC1_PFN_SET_PRODUCER_USAGE     set_producer_usage;
    GRALLOC1_PFN_GET_BACKING_STORE      get_backing_store;
    GRALLOC1_PFN_GET_CONSUMER_USAGE     get_consumer_usage;
    GRALLOC1_PFN_GET_DIMENSIONS         get_dimensions;
    GRALLOC1_PFN_GET_FORMAT             get_format;
    GRALLOC1_PFN_GET_PRODUCER_USAGE     get_producer_usage;
    GRALLOC1_PFN_GET_STRIDE             get_stride;
    GRALLOC1_PFN_ALLOCATE               allocate;
    GRALLOC1_PFN_RETAIN                 retain;
    GRALLOC1_PFN_RELEASE                release;
    GRALLOC1_PFN_GET_NUM_FLEX_PLANES    get_num_flex_planes;
    GRALLOC1_PFN_LOCK                   lock;
    GRALLOC1_PFN_LOCK_FLEX              lock_flex;
    GRALLOC1_PFN_UNLOCK                 unlock;
    GRALLOC1_PFN_SET_LAYER_COUNT        set_layer_count;
    GRALLOC1_PFN_GET_LAYER_COUNT        get_layer_count;
};
// clang-format on

class gralloc_libhareware
    : public gralloc_adapter_t,
      public std::enable_shared_from_this<gralloc_libhareware> {
    const hw_module_t* gralloc_module;
    friend class gralloc_buffer_libhardware;

    bool is_gralloc1{};

    // gralloc1
    gralloc1_device_t* gralloc1_device{};
    struct gralloc1_vptr gralloc1_vptr = {};
    bool gralloc1_support_layered_buffers{};
    bool gralloc1_release_implies_delete{};

    // gralloc0
    gralloc_module_t* gralloc0_module{};
    alloc_device_t* gralloc0_device{};

    void gralloc1_init_vptr()
    {
        uint32_t count = 0;
        gralloc1_device->getCapabilities(gralloc1_device, &count, nullptr);
        logger::log_info() << "gralloc1 count " << count;

        if (count >= 1)
        {
            std::vector<int32_t> gralloc1_capabilities(count);
            gralloc1_device->getCapabilities(gralloc1_device, &count,
                                             gralloc1_capabilities.data());
            gralloc1_capabilities.resize(count);

            // currently the only one that affects us/interests us is release
            // imply delete.
            for (auto capability : gralloc1_capabilities)
            {
                switch (capability)
                {
                case GRALLOC1_CAPABILITY_RELEASE_IMPLY_DELETE:
                    gralloc1_release_implies_delete = true;
                    break;
                case GRALLOC1_CAPABILITY_LAYERED_BUFFERS:
                    gralloc1_support_layered_buffers = true;
                    break;
                }
            }
        }

        gralloc1_vptr.create_descriptor =
            (GRALLOC1_PFN_CREATE_DESCRIPTOR)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_CREATE_DESCRIPTOR);
        gralloc1_vptr.destroy_descriptor =
            (GRALLOC1_PFN_DESTROY_DESCRIPTOR)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR);
        gralloc1_vptr.set_consumer_usage =
            (GRALLOC1_PFN_SET_CONSUMER_USAGE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_SET_CONSUMER_USAGE);
        gralloc1_vptr.set_dimensions =
            (GRALLOC1_PFN_SET_DIMENSIONS)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_SET_DIMENSIONS);
        gralloc1_vptr.set_format =
            (GRALLOC1_PFN_SET_FORMAT)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_SET_FORMAT);
        gralloc1_vptr.set_producer_usage =
            (GRALLOC1_PFN_SET_PRODUCER_USAGE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_SET_PRODUCER_USAGE);
        gralloc1_vptr.get_backing_store =
            (GRALLOC1_PFN_GET_BACKING_STORE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_BACKING_STORE);
        gralloc1_vptr.get_consumer_usage =
            (GRALLOC1_PFN_GET_CONSUMER_USAGE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_CONSUMER_USAGE);
        gralloc1_vptr.get_dimensions =
            (GRALLOC1_PFN_GET_DIMENSIONS)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_DIMENSIONS);
        gralloc1_vptr.get_format =
            (GRALLOC1_PFN_GET_FORMAT)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_FORMAT);
        gralloc1_vptr.get_producer_usage =
            (GRALLOC1_PFN_GET_PRODUCER_USAGE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_PRODUCER_USAGE);
        gralloc1_vptr.get_stride =
            (GRALLOC1_PFN_GET_STRIDE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_STRIDE);
        gralloc1_vptr.allocate =
            (GRALLOC1_PFN_ALLOCATE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_ALLOCATE);
        gralloc1_vptr.retain =
            (GRALLOC1_PFN_RETAIN)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_RETAIN);
        gralloc1_vptr.release =
            (GRALLOC1_PFN_RELEASE)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_RELEASE);
        gralloc1_vptr.get_num_flex_planes =
            (GRALLOC1_PFN_GET_NUM_FLEX_PLANES)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES);
        gralloc1_vptr.lock = (GRALLOC1_PFN_LOCK)gralloc1_device->getFunction(
            gralloc1_device, GRALLOC1_FUNCTION_LOCK);
        gralloc1_vptr.lock_flex =
            (GRALLOC1_PFN_LOCK_FLEX)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_LOCK_FLEX);
        gralloc1_vptr.unlock =
            (GRALLOC1_PFN_UNLOCK)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_UNLOCK);
        gralloc1_vptr.set_layer_count =
            (GRALLOC1_PFN_SET_LAYER_COUNT)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_SET_LAYER_COUNT);
        gralloc1_vptr.get_layer_count =
            (GRALLOC1_PFN_GET_LAYER_COUNT)gralloc1_device->getFunction(
                gralloc1_device, GRALLOC1_FUNCTION_GET_LAYER_COUNT);
    }

  public:
    gralloc_libhareware(const hw_module_t* gralloc_module) :
        gralloc_module(gralloc_module)
    {
        uint8_t majorVersion = (gralloc_module->module_api_version >> 8) & 0xFF;
        uint8_t minorVersion = (gralloc_module->module_api_version) & 0xFF;
        is_gralloc1 = (majorVersion == 1);

        // clang-format off
        logger::log_info() << "id: " << (gralloc_module->id ?: "(null)")
                           << ", name: " << (gralloc_module->name ?: "(null)")
                           << ", author: " << (gralloc_module->author ?: "(null)")
                           << ", version: " << (int)majorVersion << "." << (int)minorVersion;
        // clang-format on

        if (is_gralloc1)
        {
            gralloc1_device = nullptr;
            gralloc1_open(gralloc_module, &gralloc1_device);

            if (!gralloc1_device)
            {
                logger::log_fatal() << "cannot open gralloc1_device";
                return;
            }

            logger::log_info() << "gralloc1_device: " << gralloc1_device;
            gralloc1_init_vptr();
        }
        else
        {
            gralloc0_module = (struct gralloc_module_t*)gralloc_module;
            gralloc0_device = nullptr;
            gralloc_open(gralloc_module, &gralloc0_device);

            if (!gralloc0_device)
            {
                logger::log_fatal() << "cannot open gralloc0_device";
                return;
            }

            logger::log_info() << "gralloc0_device: " << gralloc0_device;
        }
    }
    ~gralloc_libhareware()
    {
        if (is_gralloc1)
        {
            if (gralloc1_device)
                gralloc1_close(gralloc1_device);
            gralloc1_device = nullptr;

            memset(&gralloc1_vptr, 0, sizeof(gralloc1_vptr));
        }
        else
        {
            if (gralloc0_device)
                gralloc_close(gralloc0_device);
            gralloc0_device = nullptr;
            gralloc0_module = nullptr;
        }

        dlclose(gralloc_module->dso);
        gralloc_module = nullptr;
    }

    std::shared_ptr<gralloc_buffer>
    allocate_buffer(int width, int height, int format, uint64_t usage) override;
    std::shared_ptr<gralloc_buffer> import_buffer(buffer_handle_t handle,
                                                  int width, int height,
                                                  int stride, int format,
                                                  uint64_t usage) override;
};

class gralloc_buffer_libhardware : public gralloc_buffer {
    std::shared_ptr<gralloc_libhareware> adapter;
    friend class gralloc_libhareware;

    bool locked{};
    bool was_allocated{};

  public:
    int lock(uint64_t usage, const ARect& rect, void** vaddr) override
    {
        int rval = -ENOSYS;
        if (adapter->is_gralloc1)
        {
            uint64_t producer_usage;
            uint64_t consumer_usage;
            gralloc1_rect_t access_region;

            access_region.left = rect.left;
            access_region.top = rect.top;
            access_region.width = rect.right - rect.left;
            access_region.height = rect.bottom - rect.top;

            android_convertGralloc0To1Usage(usage, &producer_usage,
                                            &consumer_usage);

            rval = adapter->gralloc1_vptr.lock(adapter->gralloc1_device, handle,
                                               producer_usage, consumer_usage,
                                               &access_region, vaddr, -1);
        }
        else
        {
            int32_t left = rect.left;
            int32_t top = rect.top;
            int32_t width = rect.right - rect.left;
            int32_t height = rect.bottom - rect.top;
            rval = adapter->gralloc0_module->lock(adapter->gralloc0_module,
                                                  handle, usage, left, top,
                                                  width, height, vaddr);
        }

        if (rval == 0)
            locked = true;
        return rval;
    }

    int unlock() override
    {
        int rval = -ENOSYS;

        if (adapter->is_gralloc1)
        {
            int releaseFence = 0;
            rval = adapter->gralloc1_vptr.unlock(adapter->gralloc1_device,
                                                 handle, &releaseFence);
            close(releaseFence);
        }
        else
        {
            rval = adapter->gralloc0_module->unlock(adapter->gralloc0_module,
                                                    handle);
        }

        if (rval == 0)
            locked = false;

        return rval;
    }

    virtual ~gralloc_buffer_libhardware()
    {
        if (!handle)
            return;

        if (locked)
            unlock();

        logger::log_info() << "delete buffer, handle: " << handle;

        if (adapter->is_gralloc1)
        {
            auto gralloc1_device = adapter->gralloc1_device;
            adapter->gralloc1_vptr.release(gralloc1_device, handle);

            if (!adapter->gralloc1_release_implies_delete)
            {
                native_handle_close(handle);
                native_handle_delete((native_handle_t*)handle);
            }
        }
        else
        {
            auto gralloc0_module = adapter->gralloc0_module;
            auto gralloc0_device = adapter->gralloc0_device;
            if (was_allocated)
            {
                gralloc0_device->free(gralloc0_device, handle);
            }
            else
            {
                gralloc0_module->unregisterBuffer(gralloc0_module, handle);

                // this needs to happen if the last reference is gone, this
                // function is only called in such cases.
                native_handle_close(handle);
                native_handle_delete((native_handle_t*)handle);
            }
        }
    }
};

inline std::shared_ptr<gralloc_buffer>
gralloc_libhareware::allocate_buffer(int width, int height, int format,
                                     uint64_t usage)
{
    auto buf = std::make_shared<gralloc_buffer_libhardware>();
    buf->adapter = shared_from_this();
    buf->was_allocated = true;
    buf->layerCount = 1;

    buffer_handle_t handle{};
    uint32_t stride{};

    int rval = -ENOSYS;
    if (is_gralloc1)
    {
        gralloc1_buffer_descriptor_t desc;
        uint64_t producer_usage{};
        uint64_t consumer_usage{};

        android_convertGralloc0To1Usage(usage, &producer_usage,
                                        &consumer_usage);

        // create temporary description (descriptor) of buffer to allocate
        rval = gralloc1_vptr.create_descriptor(gralloc1_device, &desc);
        if (rval == GRALLOC1_ERROR_NONE)
        {
            rval = gralloc1_vptr.set_dimensions(gralloc1_device, desc, width,
                                                height);
        }
        if (rval == GRALLOC1_ERROR_NONE)
        {
            rval = gralloc1_vptr.set_consumer_usage(gralloc1_device, desc,
                                                    consumer_usage);
        }
        if (rval == GRALLOC1_ERROR_NONE)
        {
            rval = gralloc1_vptr.set_producer_usage(gralloc1_device, desc,
                                                    producer_usage);
        }
        if (rval == GRALLOC1_ERROR_NONE)
        {
            rval = gralloc1_vptr.set_format(gralloc1_device, desc, format);
        }
        if (rval == GRALLOC1_ERROR_NONE)
        {
            if (gralloc1_support_layered_buffers)
            {
                rval = gralloc1_vptr.set_layer_count(gralloc1_device, desc,
                                                     buf->layerCount);
            }
            else if (buf->layerCount > 1)
            {
                rval = GRALLOC1_ERROR_UNSUPPORTED;
            }
        }

        if (rval == GRALLOC1_ERROR_NONE)
        {
            // actual allocation
            rval = gralloc1_vptr.allocate(gralloc1_device, 1, &desc, &handle);
            if (rval == GRALLOC1_ERROR_NONE)
            {
                buf->handle = handle;
            }
        }

        if (rval == GRALLOC1_ERROR_NONE)
        {
            // get stride and release temporary descriptor
            rval = gralloc1_vptr.get_stride(gralloc1_device, handle, &stride);
        }
        gralloc1_vptr.destroy_descriptor(gralloc1_device, desc);
    }
    else
    {
        rval = gralloc0_device->alloc(gralloc0_device, width, height, format,
                                      usage, &handle, (int*)&stride);
        if (rval == 0)
        {
            buf->handle = handle;
        }
    }

    logger::log_info() << "create handle width: " << width
                       << ", height: " << height << std::showbase << std::hex
                       << ", format: " << format << ", usage:" << usage;

    if (rval != 0 || stride == 0)
    {
        logger::log_fatal() << "allocate buffer failed, errno: " << rval;
        return nullptr;
    }

    buf->width = width;
    buf->height = height;
    buf->format = format;
    buf->stride = stride;
    buf->usage = usage;

    logger::log_info() << "get buffer, handle: " << buf->handle;

    return buf;
}

inline std::shared_ptr<gralloc_buffer>
gralloc_libhareware::import_buffer(buffer_handle_t handle, int width,
                                   int height, int stride, int format,
                                   uint64_t usage)
{
    auto buf = std::make_shared<gralloc_buffer_libhardware>();
    buf->adapter = shared_from_this();
    buf->was_allocated = false;
    buf->layerCount = 1;

    buffer_handle_t out_handle = wrap::native_handle_clone(handle);
    if (!out_handle)
        return nullptr;

    int rval = -ENOSYS;
    if (is_gralloc1)
    {
        rval = gralloc1_vptr.retain(gralloc1_device, out_handle);
    }
    else
    {
        rval = gralloc0_module->registerBuffer(gralloc0_module, out_handle);
    }

    if (rval != 0)
    {
        native_handle_close(out_handle);
        native_handle_delete((native_handle_t*)out_handle);

        logger::log_error() << "retain buffer failed, errno: " << rval;
        return nullptr;
    }

    buf->handle = out_handle;
    buf->width = width;
    buf->height = height;
    buf->format = format;
    buf->stride = stride;
    buf->usage = usage;

    logger::log_info() << "get buffer, handle: " << buf->handle
                       << ", width: " << width << ", height: " << height
                       << ", stride: " << stride << std::showbase << std::hex
                       << ", format: " << format << ", usage:" << usage;

    return buf;
}

#endif // GRALLOC_ADAPTER_LIBHARDWARE_H_
