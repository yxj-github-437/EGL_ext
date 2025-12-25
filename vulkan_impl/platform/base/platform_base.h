#ifndef VULKAN_PLATFORM_BASE_H_
#define VULKAN_PLATFORM_BASE_H_

#include <vulkan/vulkan.h>

#include "platform_common/base/platform_base.h"

class VulkanBaseNativeWindow : public BaseNativeWindow {
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

    virtual ANativeWindow* create_window(void* native_window) = 0;
    virtual void destroy_window(ANativeWindow* win) = 0;
};

#endif // EGL_PLATFORM_BASE_H_
