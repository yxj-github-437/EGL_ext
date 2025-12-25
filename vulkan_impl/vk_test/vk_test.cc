/*
 * test_vulkan: Test Vulkan implementation
 * Copyright (c) 2022 Jolla Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <vector>

#include <wayland-client.h>
#include <xdg-shell-client-protocol-core.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

#define GET_EXTENSION_FUNCTION(_id)                                            \
    ({                                                                         \
        PFN_##_id fn = (PFN_##_id)(vkGetInstanceProcAddr(instance, #_id));     \
        if (!fn)                                                               \
        {                                                                      \
            fprintf(stderr, "cannot find function %s\n", #_id);                \
            exit(-1);                                                          \
        }                                                                      \
        fn;                                                                    \
    })

VkDevice device;
VkPhysicalDevice usedPhysicalDevice;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
std::vector<VkImage> swapchainImages;
uint32_t imageCount;
VkFormat swapchainImageFormat;
VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
std::vector<VkFence> fences;
uint32_t graphicsFamily = 0;
uint32_t presentFamily = 0;
VkSurfaceKHR surface = VK_NULL_HANDLE;

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct wl_data
{
    struct wl_display* wldisplay;
    struct wl_registry* wlregistry;
    struct wl_compositor* wlcompositor;
    struct wl_surface* wlsurface;
    struct wl_region* wlregion;
    struct xdg_wm_base* xdgshell;
    struct xdg_surface* xdgshellSurface;
    struct xdg_toplevel* xdgtoplevel;
    struct wl_output* wloutput;
    uint32_t width;
    uint32_t height;
    bool quit;
} wl_data;

static void handleShellPing(void* data, struct xdg_wm_base* shell,
                            uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener shellListener = {.ping =
                                                              handleShellPing};

static void handleShellSurfaceConfigure(void* data,
                                        struct xdg_surface* shellSurface,
                                        uint32_t serial)
{
    xdg_surface_ack_configure(shellSurface, serial);
}

static const struct xdg_surface_listener shellSurfaceListener = {
    .configure = handleShellSurfaceConfigure};

static void handleToplevelConfigure(void* data, struct xdg_toplevel* toplevel,
                                    int32_t width, int32_t height,
                                    struct wl_array* states)
{
    struct wl_data* driverdata = (struct wl_data*)data;
    if (width != 0 && height != 0)
    {
        driverdata->width = width;
        driverdata->height = height;
    }
}

static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel)
{
    struct wl_data* driverdata = (struct wl_data*)data;
    driverdata->quit = 1;
}

static const struct xdg_toplevel_listener toplevelListener = {
    .configure = handleToplevelConfigure, .close = handleToplevelClose};

static void display_handle_geometry(void* data, struct wl_output* output, int x,
                                    int y, int physical_width,
                                    int physical_height, int subpixel,
                                    const char* make, const char* model,
                                    int transform)
{
}

static void display_handle_mode(void* data, struct wl_output* output,
                                uint32_t flags, int width, int height,
                                int refresh)
{
    struct wl_data* driverdata = (struct wl_data*)data;
    driverdata->width = width;
    driverdata->height = height;
}

static void display_handle_done(void* data, struct wl_output* output)
{
}

static void display_handle_scale(void* data, struct wl_output* output,
                                 int32_t factor)
{
}

static const struct wl_output_listener output_listener = {
    display_handle_geometry, display_handle_mode, display_handle_done,
    display_handle_scale};

static void global_registry_handler(void* data, struct wl_registry* registry,
                                    uint32_t id, const char* interface,
                                    uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0)
    {
        wl_data.wlcompositor = (wl_compositor*)wl_registry_bind(
            registry, id, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        wl_data.xdgshell = (xdg_wm_base*)wl_registry_bind(
            registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wl_data.xdgshell, &shellListener, &wl_data);
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        wl_data.wloutput =
            (wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
        wl_output_add_listener(wl_data.wloutput, &output_listener, &wl_data);
    }
}

static void global_registry_remover(void* data, struct wl_registry* registry,
                                    uint32_t id)
{
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler, global_registry_remover};

static void get_server_references(void)
{
    wl_data.wldisplay = wl_display_connect(nullptr);
    if (wl_data.wldisplay == nullptr)
    {
        fprintf(stderr, "Failed to connect to display\n");
        exit(1);
    }

    wl_data.wlregistry = wl_display_get_registry(wl_data.wldisplay);
    wl_registry_add_listener(wl_data.wlregistry, &registry_listener, nullptr);

    wl_display_dispatch(wl_data.wldisplay);
    wl_display_roundtrip(wl_data.wldisplay);

    if (wl_data.wlcompositor == nullptr || wl_data.xdgshell == nullptr)
    {
        fprintf(stderr, "Failed to find compositor or shell\n");
        exit(1);
    }
}

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphore) != VK_SUCCESS)
    {
        printf("Failed to create synchronization objects for a frame\n");
        exit(1);
    }

    fences.resize(imageCount);

    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkResult result;

        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        result = vkCreateFence(device, &createInfo, nullptr, &fences[i]);
        if (result != VK_SUCCESS)
        {
            for (; i > 0; i--)
            {
                vkDestroyFence(device, fences[i - 1], nullptr);
            }
            fences.clear();
            printf("Failed to create fences\n");
            exit(2);
        }
    }
}

VkResult vkGetBestGraphicsQueue(VkPhysicalDevice physicalDevice)
{
    VkResult ret = VK_ERROR_INITIALIZATION_FAILED;
    uint32_t queueFamilyPropertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyPropertiesCount, 0);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);

    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());

    graphicsFamily = queueFamilyPropertiesCount;
    presentFamily = queueFamilyPropertiesCount;

    for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++)
    {
        if (VK_QUEUE_GRAPHICS_BIT & queueFamilyProperties[i].queueFlags)
        {
            graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                             &presentSupport);

        if (presentSupport)
        {
            presentFamily = i;
        }

        if (graphicsFamily != queueFamilyPropertiesCount &&
            presentFamily != queueFamilyPropertiesCount)
        {
            ret = VK_SUCCESS;
            break;
        }
    }

    return ret;
}

#define check_result(result)                                                   \
    if (VK_SUCCESS != (result))                                                \
    {                                                                          \
        fprintf(stderr, "Failure at %i %s with result %i\n", __LINE__,         \
                __FILE__, result);                                             \
        exit(-1);                                                              \
    }

static void recordPipelineImageBarrier(VkCommandBuffer commandBuffer,
                                       VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask,
                                       VkImageLayout srcLayout,
                                       VkImageLayout dstLayout, VkImage image)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = srcLayout;
    barrier.newLayout = dstLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
}

static void rerecordCommandBuffer(uint32_t frameIndex,
                                  const VkClearColorValue* clearColor)
{
    VkCommandBuffer commandBuffer = commandBuffers[frameIndex];
    VkImage image = swapchainImages[frameIndex];
    VkCommandBufferBeginInfo beginInfo = {};
    VkImageSubresourceRange clearRange = {};

    check_result(vkResetCommandBuffer(commandBuffer, 0));

    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    check_result(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    recordPipelineImageBarrier(commandBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image);

    clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clearRange.baseMipLevel = 0;
    clearRange.levelCount = 1;
    clearRange.baseArrayLayer = 0;
    clearRange.layerCount = 1;
    vkCmdClearColorImage(commandBuffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clearColor, 1,
                         &clearRange);

    recordPipelineImageBarrier(commandBuffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_ACCESS_MEMORY_READ_BIT,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, image);
    check_result(vkEndCommandBuffer(commandBuffer));
}

bool createSwapchain()
{
    /*
     * Select mode and format
     */
    SwapchainSupportDetails swapchainSupport =
        querySwapchainSupport(usedPhysicalDevice);
    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapchainSupport.presentModes);
    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapchainSupport.formats);

    imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = (VkExtent2D){wl_data.width, wl_data.height};
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};

    if (graphicsFamily != presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapchain;

    if (vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain) != VK_SUCCESS)
    {
        return false;
    }

    /*
     * Get swapchain images
     */
    check_result(
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));

    swapchainImages.resize(imageCount);
    check_result(vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
                                         swapchainImages.data()));
    if (createInfo.oldSwapchain)
    {
        vkDestroySwapchainKHR(device, createInfo.oldSwapchain, nullptr);
    }

    swapchainImageFormat = surfaceFormat.format;

    /*
     * Create command pool
     */
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                           VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = graphicsFamily;
    check_result(
        vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool));

    /*
     * Create command buffers
     */
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = imageCount;
    commandBuffers.resize(imageCount);
    check_result(vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()));

    return true;
}

const char* physicalDeviceTypeString(VkPhysicalDeviceType type)
{
    const char* typeString;
    switch (type)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        typeString = "Other";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        typeString = "Integrated GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        typeString = "Discrete GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        typeString = "Virtual GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        typeString = "CPU";
        break;
    default:
        typeString = "unknown";
    }
    return typeString;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    /*
     * Wayland Setup
     */
    get_server_references();

    wl_data.wlsurface = wl_compositor_create_surface(wl_data.wlcompositor);
    if (wl_data.wlsurface == nullptr)
    {
        fprintf(stderr, "Failed to create surface\n");
        exit(1);
    }

    wl_data.xdgshellSurface =
        xdg_wm_base_get_xdg_surface(wl_data.xdgshell, wl_data.wlsurface);
    xdg_surface_add_listener(wl_data.xdgshellSurface, &shellSurfaceListener,
                             &wl_data);

    wl_data.xdgtoplevel = xdg_surface_get_toplevel(wl_data.xdgshellSurface);
    xdg_toplevel_add_listener(wl_data.xdgtoplevel, &toplevelListener, &wl_data);

    xdg_toplevel_set_title(wl_data.xdgtoplevel, "test_vulkan");
    xdg_toplevel_set_app_id(wl_data.xdgtoplevel, "test_vulkan");

    wl_surface_commit(wl_data.wlsurface);
    wl_display_roundtrip(wl_data.wldisplay);

    wl_data.wlregion = wl_compositor_create_region(wl_data.wlcompositor);
    wl_region_add(wl_data.wlregion, 0, 0, wl_data.width, wl_data.height);
    wl_surface_set_opaque_region(wl_data.wlsurface, wl_data.wlregion);

    /*
     * Create instance
     */
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "test_vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    /*
     * Enumerate the instance extensions
     */
    struct vkExtensionName_t {
        char* name;
        vkExtensionName_t() { name = new char[VK_MAX_EXTENSION_NAME_SIZE]; }
        vkExtensionName_t(const vkExtensionName_t& other) = delete;
        vkExtensionName_t(vkExtensionName_t&& other) noexcept {
            name = other.name;
            other.name = nullptr;
        };
        vkExtensionName_t& operator=(const vkExtensionName_t&) = delete;
        vkExtensionName_t& operator=(vkExtensionName_t&& other) noexcept {
            std::swap(this->name, other.name);
            return *this;
        }
        ~vkExtensionName_t() { if (name) delete[] name; }
    };
    std::vector<vkExtensionName_t> enabledExtensions;
    {
        uint32_t instExtCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &instExtCount, nullptr);

        std::vector<VkExtensionProperties> inst_exts(instExtCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &instExtCount, inst_exts.data());

        enabledExtensions.resize(instExtCount);
        for (uint32_t i = 0; i < instExtCount; i++)
        {
            strncpy(enabledExtensions[i].name, inst_exts[i].extensionName, VK_MAX_EXTENSION_NAME_SIZE);
        }

        for(size_t i = 0; i < enabledExtensions.size(); i++) {
            const char* name = *(((const char* const*)enabledExtensions.data()) + i);
            printf("Found extension %s\n", name);
        }
        printf("Found %u instance extensions\n", instExtCount);
    }

    /*
     * Create instance
     */
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = (const char* const*)enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.pNext = nullptr;

    VkInstance instance;
    check_result(vkCreateInstance(&instanceCreateInfo, 0, &instance));

    /*
     * Find physical device
     */
    uint32_t physicalDeviceCount = 0;
    check_result(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));

    printf("Found %u physical device(s)\n", physicalDeviceCount);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

    check_result(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                            physicalDevices.data()));

    if (!physicalDevices.empty())
    {
        /*
         * Prinf device information
         */
        usedPhysicalDevice = physicalDevices[0];

        VkPhysicalDeviceProperties deviceProperties = {};

        vkGetPhysicalDeviceProperties(usedPhysicalDevice, &deviceProperties);
        printf("  Device name: %s\n", deviceProperties.deviceName);
        printf("  Device type: %s\n",
               physicalDeviceTypeString(deviceProperties.deviceType));
        printf("  API version: %u.%u.%u\n",
               VK_VERSION_MAJOR(deviceProperties.apiVersion),
               VK_VERSION_MINOR(deviceProperties.apiVersion),
               VK_VERSION_PATCH(deviceProperties.apiVersion));
        printf("  Driver version: %u.%u.%u\n",
               VK_VERSION_MAJOR(deviceProperties.driverVersion),
               VK_VERSION_MINOR(deviceProperties.driverVersion),
               VK_VERSION_PATCH(deviceProperties.driverVersion));
        printf("  Device ID 0x%x\n", deviceProperties.deviceID);
        printf("  Vendor ID 0x%x\n", deviceProperties.vendorID);

        uint32_t deviceExtensionCount = 0;
        check_result(vkEnumerateDeviceExtensionProperties(usedPhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
        {
            std::vector<VkExtensionProperties> dev_exts(deviceExtensionCount);
            check_result(vkEnumerateDeviceExtensionProperties(usedPhysicalDevice, nullptr, &deviceExtensionCount, dev_exts.data()));
            for (auto& e : dev_exts) {
                printf("Found device extension %s\n", e.extensionName);
            }
            printf("  Found %u device extensions\n", deviceExtensionCount);
        }

        /*
         * Create surface
         */
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType =
            VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.pNext = nullptr;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.display = wl_data.wldisplay;
        surfaceCreateInfo.surface = wl_data.wlsurface;
        check_result(GET_EXTENSION_FUNCTION(vkCreateWaylandSurfaceKHR)(
            instance, &surfaceCreateInfo, nullptr, &surface));

        /*
         * Create device
         */
        uint32_t queueFamilyIndex = 0;
        check_result(vkGetBestGraphicsQueue(usedPhysicalDevice));

        static const char* const deviceExtensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
        deviceQueueCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;

        check_result(vkCreateDevice(usedPhysicalDevice, &deviceCreateInfo, 0, &device));

        /*
         * Get queues
         */
        vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
        if (graphicsFamily != presentFamily)
        {
            vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
        }
        else
        {
            presentQueue = graphicsQueue;
        }

        /*
         * Create swapchain
         */
        if (!createSwapchain())
        {
            printf("Failed to create swapchain\n");
            exit(1);
        }

        /*
         * Create sync objects
         */
        createSyncObjects();

        /*
         * Render loop
         */
        printf("Start render loop\n");
        auto start = std::chrono::system_clock::now();
        uint32_t last_frame = 0;
        for (uint32_t currentFrame = 0; currentFrame < 3600 && !wl_data.quit; currentFrame++)
        {
            VkClearColorValue clearColor = {{0}};
            float time = currentFrame / 120.0;
            uint32_t imageIndex;
            check_result(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                               imageAvailableSemaphore,
                                               VK_NULL_HANDLE, &imageIndex));

            check_result(vkWaitForFences(device, 1, &fences[imageIndex],
                                         VK_FALSE, UINT64_MAX));
            check_result(vkResetFences(device, 1, &fences[imageIndex]));

            clearColor.float32[0] = (float)(0.5 + 0.5 * sin(time));
            clearColor.float32[1] =
                (float)(0.5 + 0.5 * sin(time + M_PI * 2 / 3));
            clearColor.float32[2] =
                (float)(0.5 + 0.5 * sin(time + M_PI * 4 / 3));
            clearColor.float32[3] = 1;
            rerecordCommandBuffer(imageIndex, &clearColor);

            VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_TRANSFER_BIT};

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

            check_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                                       fences[imageIndex]));

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pImageIndices = &imageIndex;

            check_result(vkQueuePresentKHR(presentQueue, &presentInfo));

            if (currentFrame % 360 == 359 || currentFrame == (3600 - 1))
            {
                auto end = std::chrono::system_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                printf("FPS %u\n", (uint32_t)(double(currentFrame - last_frame) / duration.count() * 1000));

                last_frame = currentFrame;
                start = end;
            }

            wl_display_dispatch_pending(wl_data.wldisplay);
            wl_display_flush(wl_data.wldisplay);
        }

        /*
         * Cleanup vulkan setup
         */
        for (uint32_t i = 0; i < imageCount; i++)
        {
            vkDestroyFence(device, fences[i], nullptr);
        }
        fences.clear();

        vkFreeCommandBuffers(device, commandPool, imageCount, commandBuffers.data());
        commandBuffers.clear();

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchainImages.clear();

        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    vkDestroyInstance(instance, nullptr);

    xdg_toplevel_destroy(wl_data.xdgtoplevel);
    xdg_surface_destroy(wl_data.xdgshellSurface);
    wl_surface_destroy(wl_data.wlsurface);
    xdg_wm_base_destroy(wl_data.xdgshell);
    wl_compositor_destroy(wl_data.wlcompositor);
    wl_registry_destroy(wl_data.wlregistry);
    wl_display_disconnect(wl_data.wldisplay);

    return 0;
}
