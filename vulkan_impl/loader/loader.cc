#include "loader.h"
#include "hooks.h"


namespace {
#undef VK_ENTRY
#define VK_ENTRY(_r, _api, ...) #_api,

// clang-format off
char const* const vk_names[] = {
    #include "vulkan_entries.in"
    nullptr
};
// clang-format on

#undef VK_ENTRY
} // namespace

