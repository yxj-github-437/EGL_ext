#define VK_USE_PLATFORM_ANDROID_KHR

#include <vulkan/vulkan.h>

#undef CALL_VK_API
#undef CALL_VK_API_INTERNAL_CALL
#undef CALL_VK_API_INTERNAL_SET_RETURN_VALUE
#undef CALL_VK_API_INTERNAL_DO_RETURN
#undef CALL_VK_API_RETURN

#define CALL_VK_API_INTERNAL_CALL(_api, ...) return (_api)(__VA_ARGS__);

#define CALL_VK_API_INTERNAL_SET_RETURN_VALUE return {};

// This stays blank, since void functions will implicitly return, and
// all of the other functions will return 0 based on the previous macro.
#define CALL_VK_API_INTERNAL_DO_RETURN

#define CALL_VK_API(_api, ...)                                                 \
    CALL_VK_API_INTERNAL_CALL(_api, __VA_ARGS__)                               \
    CALL_VK_API_INTERNAL_DO_RETURN

#define CALL_VK_API_RETURN(_api, ...)                                          \
    CALL_VK_API_INTERNAL_CALL(_api, __VA_ARGS__)                               \
    CALL_VK_API_INTERNAL_SET_RETURN_VALUE                                      \
    CALL_VK_API_INTERNAL_DO_RETURN

extern "C" {
#include "vulkan_api.in"
}

#undef CALL_VK_API
#undef CALL_VK_API_INTERNAL_CALL
#undef CALL_VK_API_INTERNAL_SET_RETURN_VALUE
#undef CALL_VK_API_INTERNAL_DO_RETURN
#undef CALL_VK_API_RETURN
