#ifndef GRALLOC_APAPTER_H_
#define GRALLOC_APAPTER_H_

#include <cutils/native_handle.h>
#include <hardware/hardware.h>

#include <android/hardware_buffer.h>
#include <memory>

enum class backend_type : uint8_t {
    gralloc_libhardware,
    gralloc_nativewindow,
    gralloc_none
};

class gralloc_adapter_t {
  public:
    class loader {
      public:
        ~loader();
        static loader& getInstance();

        backend_type get_backend() const { return backend; };

        loader(const loader&) = delete;
        loader& operator=(const loader&) = delete;

        std::shared_ptr<gralloc_adapter_t> get_adapter() const;

      protected:
        loader();

        backend_type backend;
        void* nativewindow_handle;
        const hw_module_t* gralloc_module;
        std::shared_ptr<gralloc_adapter_t> adapter;
    };

    class cutils_loader {
        void* handle = nullptr;

      public:
        struct cutils_vptr
        {
            int (*native_handle_close)(const native_handle_t* h);
            native_handle_t* (*native_handle_init)(char* storage, int numFds,
                                                   int numInts);
            native_handle_t* (*native_handle_create)(int numFds, int numInts);
            native_handle_t* (*native_handle_clone)(const native_handle_t* handle);
            int (*native_handle_delete)(native_handle_t* h);
        } vptr = {};

        cutils_loader() noexcept;
        ~cutils_loader() noexcept;
    };

    class buffer {
      public:
        int width{};
        int height{};
        int stride{};
        int format{};
        uintptr_t layerCount{};
        buffer_handle_t handle{};
        uint64_t usage{};

        buffer() = default;
        virtual ~buffer() = 0;

        buffer(const buffer&) = delete;
        buffer& operator=(const buffer&) = delete;

        virtual int lock(uint64_t usage, const ARect& rect, void** vaddr) = 0;
        virtual int unlock() = 0;
    };

  public:
    virtual ~gralloc_adapter_t() = default;
    virtual std::shared_ptr<gralloc_adapter_t::buffer>
    allocate_buffer(int width, int height, int format, uint64_t usage) = 0;
    virtual std::shared_ptr<gralloc_adapter_t::buffer>
    import_buffer(buffer_handle_t handle, int width, int height, int stride,
                  int format, uint64_t usage) = 0;

    cutils_loader cutils{};
};

using gralloc_loader = gralloc_adapter_t::loader;
using gralloc_buffer = gralloc_adapter_t::buffer;

#endif
