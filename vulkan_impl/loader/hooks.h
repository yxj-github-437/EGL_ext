#ifndef VULKAN_HOOKS_H_
#define VULKAN_HOOKS_H_

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

namespace vk_wrapper {

#undef VK_ENTRY
#define VK_ENTRY(_r, _api, ...) _r (*(_api))(__VA_ARGS__);

// clang-format off
struct vulkan_t {
    #include "vulkan_entries.in"
};
// clang-format on

#undef VK_ENTRY

} // namespace vk_wrapper

#endif // VULKAN_HOOKS_H_
