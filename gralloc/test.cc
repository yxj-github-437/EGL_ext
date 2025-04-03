#include "logger.h"
#include "gralloc_adapter.h"

#include <android/hardware_buffer.h>
#include <android/rect.h>
#include <stdio.h>

int main()
{
    auto adapter = gralloc_loader::getInstance().get_adapter();
    auto buffer = adapter->allocate_buffer(
        100, 100, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
        AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
            AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER |
            AHARDWAREBUFFER_USAGE_CPU_READ_MASK |
            AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK);

    void* vaddr = nullptr;

    {
        ARect rect = {0, 0, 100, 100};
        buffer->lock(AHARDWAREBUFFER_USAGE_CPU_READ_MASK |
                         AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK,
                     rect, &vaddr);
        if (vaddr)
        {
            ((uint32_t*)vaddr)[0] = 0xaabbccdd;
            logger::log_info() << "get vaddr " << vaddr
                               << ", write vaddr[0]: " << std::showbase
                               << std::hex << ((uint32_t*)vaddr)[0];
        }

        vaddr = nullptr;
    }

    auto buffer_ =
        adapter->import_buffer(buffer->handle, buffer->width, buffer->height,
                               buffer->stride, buffer->format, buffer->usage);

    // buffer = nullptr;

    {
        ARect rect = {0, 0, 100, 100};
        buffer_->lock(AHARDWAREBUFFER_USAGE_CPU_READ_MASK, rect, &vaddr);
        if (vaddr)
        {
            logger::log_info()
                << "get vaddr " << vaddr << ", read vaddr[0]: " << std::showbase
                << std::hex << ((uint32_t*)vaddr)[0];

            char c = getchar();
        }
    }
}
