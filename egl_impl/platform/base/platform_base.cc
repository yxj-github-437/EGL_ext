#include "platform_base.h"
#include "logger.h"

#include <sync/sync.h>

#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        const __typeof(((type*)nullptr)->member)* _ptr = (ptr);                \
        (type*)((char*)_ptr) - offsetof(type, member);                         \
    })

BaseNativeWindowBuffer::BaseNativeWindowBuffer()
{
    common.incRef = +[](struct android_native_base_t* base) {
        ANativeWindowBuffer* self =
            container_of(base, ANativeWindowBuffer, common);
        auto buffer = static_cast<BaseNativeWindowBuffer*>(self);

        buffer->refcount++;
    };
    common.decRef = +[](struct android_native_base_t* base) {
        ANativeWindowBuffer* self =
            container_of(base, ANativeWindowBuffer, common);
        auto buffer = static_cast<BaseNativeWindowBuffer*>(self);

        if (buffer->refcount-- == 1)
        {
            delete buffer;
        }
    };

    ANativeWindowBuffer::width = 0;
    ANativeWindowBuffer::height = 0;
    ANativeWindowBuffer::stride = 0;
    ANativeWindowBuffer::format = 0;
    ANativeWindowBuffer::usage = ANativeWindowBuffer::usage_deprecated = 0;
    ANativeWindowBuffer::layerCount = 0;
    ANativeWindowBuffer::handle = nullptr;

    refcount = 0;
}

BaseNativeWindowBuffer::~BaseNativeWindowBuffer()
{
    common.incRef = nullptr;
    common.decRef = nullptr;

    refcount = 0;
}

ANativeWindowBuffer* BaseNativeWindowBuffer::getNativeBuffer() const
{
    return const_cast<BaseNativeWindowBuffer*>(this);
};

BaseNativeWindow::BaseNativeWindow()
{
    common.incRef = +[](struct android_native_base_t* base) {
        ANativeWindow* self = container_of(base, ANativeWindow, common);
        auto window = static_cast<BaseNativeWindow*>(self);

        window->refcount++;
    };
    common.decRef = +[](struct android_native_base_t* base) {
        ANativeWindow* self = container_of(base, ANativeWindow, common);
        auto window = static_cast<BaseNativeWindow*>(self);

        if (window->refcount-- == 1)
        {
            delete window;
        }
    };

    const_cast<uint32_t&>(ANativeWindow::flags) = 0;
    const_cast<float&>(ANativeWindow::xdpi) = 0;
    const_cast<float&>(ANativeWindow::ydpi) = 0;
    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 0;

    // clang-format off
    ANativeWindow::setSwapInterval = +[](struct ANativeWindow* window,
                                         int interval) {
        auto self = static_cast<BaseNativeWindow*>(window);
        return self->setSwapInterval(interval);
    };
    ANativeWindow::dequeueBuffer_DEPRECATED = +[](ANativeWindow* window,
                                                  ANativeWindowBuffer** buffer) {
        auto self = static_cast<BaseNativeWindow*>(window);
        BaseNativeWindowBuffer* new_buffer = nullptr;
        int fenceFd = -1;
        int rval = self->dequeueBuffer(&new_buffer, &fenceFd);

        *buffer = new_buffer;
        if (fenceFd != -1) {
            sync_wait(fenceFd, -1);
            close(fenceFd);
        }

        return rval;
    };
    ANativeWindow::dequeueBuffer = +[](ANativeWindow* window,
                                       ANativeWindowBuffer** buffer, int* fenceFd) {
        auto self = static_cast<BaseNativeWindow*>(window);
        BaseNativeWindowBuffer* new_buffer = nullptr;

        int rval = self->dequeueBuffer(&new_buffer, fenceFd);
        *buffer = new_buffer;
        return rval;
    };
    ANativeWindow::queueBuffer_DEPRECATED = +[](struct ANativeWindow* window,
                                                ANativeWindowBuffer* buffer) {
        auto self = static_cast<BaseNativeWindow*>(window);
        auto nativeBuffer = static_cast<BaseNativeWindowBuffer*>(buffer);
        return self->queueBuffer(nativeBuffer, -1);
    };
    ANativeWindow::queueBuffer = +[](struct ANativeWindow *window,
                                     ANativeWindowBuffer *buffer, int fenceFd) {
        auto self = static_cast<BaseNativeWindow*>(window);
        auto nativeBuffer = static_cast<BaseNativeWindowBuffer*>(buffer);
        return self->queueBuffer(nativeBuffer, fenceFd);
    };
    ANativeWindow::cancelBuffer_DEPRECATED = +[](struct ANativeWindow* window,
                                                 ANativeWindowBuffer* buffer) {
        auto self = static_cast<BaseNativeWindow*>(window);
        auto nativeBuffer = static_cast<BaseNativeWindowBuffer*>(buffer);
        return self->cancelBuffer(nativeBuffer, -1);
    };
    ANativeWindow::cancelBuffer = +[](struct ANativeWindow *window,
                                      ANativeWindowBuffer *buffer, int fenceFd) {
        auto self = static_cast<BaseNativeWindow*>(window);
        auto nativeBuffer = static_cast<BaseNativeWindowBuffer*>(buffer);
        return self->cancelBuffer(nativeBuffer, fenceFd);
    };
    ANativeWindow::lockBuffer_DEPRECATED = +[](struct ANativeWindow* window,
                                               ANativeWindowBuffer* buffer) {
        auto self = static_cast<BaseNativeWindow*>(window);
        return self->lockBuffer(static_cast<BaseNativeWindowBuffer*>(buffer));
    };
    ANativeWindow::query = _query;
    ANativeWindow::perform = _perform;
    // clang-format on

    refcount = 0;
}

BaseNativeWindow::~BaseNativeWindow()
{
    common.incRef = nullptr;
    common.decRef = nullptr;

    refcount = 0;
}

namespace {
const char* native_window_operation(int what)
{
    switch (what)
    {
    case NATIVE_WINDOW_SET_USAGE:
        return "NATIVE_WINDOW_SET_USAGE";
    case NATIVE_WINDOW_CONNECT:
        return "NATIVE_WINDOW_CONNECT";
    case NATIVE_WINDOW_DISCONNECT:
        return "NATIVE_WINDOW_DISCONNECT";
    case NATIVE_WINDOW_SET_CROP:
        return "NATIVE_WINDOW_SET_CROP";
    case NATIVE_WINDOW_SET_BUFFER_COUNT:
        return "NATIVE_WINDOW_SET_BUFFER_COUNT";
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        return "NATIVE_WINDOW_SET_BUFFERS_GEOMETRY";
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
        return "NATIVE_WINDOW_SET_BUFFERS_TRANSFORM";
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
        return "NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP";
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
        return "NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS";
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
        return "NATIVE_WINDOW_SET_BUFFERS_FORMAT";
    case NATIVE_WINDOW_SET_SCALING_MODE:
        return "NATIVE_WINDOW_SET_SCALING_MODE";
    case NATIVE_WINDOW_LOCK:
        return "NATIVE_WINDOW_LOCK";
    case NATIVE_WINDOW_UNLOCK_AND_POST:
        return "NATIVE_WINDOW_UNLOCK_AND_POST";
    case NATIVE_WINDOW_API_CONNECT:
        return "NATIVE_WINDOW_API_CONNECT";
    case NATIVE_WINDOW_API_DISCONNECT:
        return "NATIVE_WINDOW_API_DISCONNECT";
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS:
        return "NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS";
    case NATIVE_WINDOW_SET_POST_TRANSFORM_CROP:
        return "NATIVE_WINDOW_SET_POST_TRANSFORM_CROP";
    case NATIVE_WINDOW_SET_USAGE64:
        return "NATIVE_WINDOW_SET_USAGE64";
    case NATIVE_WINDOW_GET_CONSUMER_USAGE64:
        return "NATIVE_WINDOW_GET_CONSUMER_USAGE64";
    default:
        return "NATIVE_UNKNOWN_OPERATION";
    }
}

const char* native_query_operation(int what)
{
    switch (what)
    {
    case NATIVE_WINDOW_WIDTH:
        return "NATIVE_WINDOW_WIDTH";
    case NATIVE_WINDOW_HEIGHT:
        return "NATIVE_WINDOW_HEIGHT";
    case NATIVE_WINDOW_FORMAT:
        return "NATIVE_WINDOW_FORMAT";
    case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
        return "NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS";
    case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
        return "NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER";
    case NATIVE_WINDOW_CONCRETE_TYPE:
        return "NATIVE_WINDOW_CONCRETE_TYPE";
    case NATIVE_WINDOW_DEFAULT_WIDTH:
        return "NATIVE_WINDOW_DEFAULT_WIDTH";
    case NATIVE_WINDOW_DEFAULT_HEIGHT:
        return "NATIVE_WINDOW_DEFAULT_HEIGHT";
    case NATIVE_WINDOW_TRANSFORM_HINT:
        return "NATIVE_WINDOW_TRANSFORM_HINT";
    case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
        return "NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND";
    case NATIVE_WINDOW_DEFAULT_DATASPACE:
        return "NATIVE_WINDOW_DEFAULT_DATASPACE";
    case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
        return "NATIVE_WINDOW_CONSUMER_USAGE_BITS";
    case NATIVE_WINDOW_IS_VALID:
        return "NATIVE_WINDOW_IS_VALID";
    case NATIVE_WINDOW_BUFFER_AGE:
        return "NATIVE_WINDOW_BUFFER_AGE";
    case NATIVE_WINDOW_MAX_BUFFER_COUNT:
        return "NATIVE_WINDOW_MAX_BUFFER_COUNT";
    default:
        return "NATIVE_UNKNOWN_QUERY";
    }
}
} // namespace

int BaseNativeWindow::_query(const struct ANativeWindow* window, int what,
                             int* value)
{
    logger::log_verbose() << "window:" << window
                          << " what:" << native_query_operation(what);
    const BaseNativeWindow* self = static_cast<const BaseNativeWindow*>(window);
    switch (what)
    {
    case NATIVE_WINDOW_WIDTH:
        *value = self->width();
        return NO_ERROR;
    case NATIVE_WINDOW_HEIGHT:
        *value = self->height();
        return NO_ERROR;
    case NATIVE_WINDOW_FORMAT:
        *value = self->format();
        return NO_ERROR;
    case NATIVE_WINDOW_CONCRETE_TYPE:
        *value = self->type();
        return NO_ERROR;
    case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
        *value = self->queueLength();
        return NO_ERROR;
    case NATIVE_WINDOW_DEFAULT_WIDTH:
        *value = self->defaultWidth();
        return NO_ERROR;
    case NATIVE_WINDOW_DEFAULT_HEIGHT:
        *value = self->defaultHeight();
        return NO_ERROR;
    case NATIVE_WINDOW_TRANSFORM_HINT:
        *value = self->transformHint();
        return NO_ERROR;
    case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
        *value = 1;
        return NO_ERROR;
    case NATIVE_WINDOW_DEFAULT_DATASPACE:
        *value = HAL_DATASPACE_UNKNOWN;
        return NO_ERROR;
    case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
        *value = self->getUsage();
        return NO_ERROR;
    case NATIVE_WINDOW_IS_VALID:
        // sure :)
        *value = 1;
        return NO_ERROR;
    case NATIVE_WINDOW_BUFFER_AGE:
        // sure :)
        *value = 2;
        return NO_ERROR;
    case NATIVE_WINDOW_MAX_BUFFER_COUNT:
        // The default maximum count of BufferQueue items.
        // See android::BufferQueueDefs::NUM_BUFFER_SLOTS.
        *value = 64;
        return NO_ERROR;
    }
    logger::log_error() << "NativeWindow error: unknown window attribute! "
                        << what;
    *value = 0;
    return BAD_VALUE;
}

int BaseNativeWindow::_perform(struct ANativeWindow* window, int operation, ...)
{
    BaseNativeWindow* self = static_cast<BaseNativeWindow*>(window);
    va_list args;
    va_start(args, operation);

    logger::log_verbose() << "operation: "
                          << native_window_operation(operation);
    switch (operation)
    {
    case NATIVE_WINDOW_SET_USAGE: //  0,
    {
        uint64_t usage = va_arg(args, uint32_t);
        va_end(args);
        return self->setUsage(usage);
    }
    case NATIVE_WINDOW_CONNECT: //  1,   /* deprecated */
        logger::log_verbose() << "connect";
        break;
    case NATIVE_WINDOW_DISCONNECT: //  2,   /* deprecated */
        logger::log_verbose() << "disconnect";
        break;
    case NATIVE_WINDOW_SET_CROP: //  3,   /* private */
        logger::log_verbose() << "set crop";
        break;
    case NATIVE_WINDOW_SET_BUFFER_COUNT: //  4,
    {
        int cnt = va_arg(args, int);
        logger::log_verbose() << "set buffer count " << cnt;
        va_end(args);
        return self->setBufferCount(cnt);
    }
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY: //  5,   /* deprecated */
        logger::log_verbose() << "set buffers geometry";
        break;
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM: //  6,
        logger::log_verbose() << "set buffers transform";
        break;
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP: //  7,
        logger::log_verbose() << "set buffers timestamp";
        break;
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS: //  8,
    {
        int width = va_arg(args, int);
        int height = va_arg(args, int);
        va_end(args);
        return self->setBuffersDimensions(width, height);
    }
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT: //  9,
    {
        int format = va_arg(args, int);
        va_end(args);
        return self->setBuffersFormat(format);
    }
    case NATIVE_WINDOW_SET_SCALING_MODE: // 10,   /* private */
        logger::log_verbose() << "set scaling mode";
        break;
    case NATIVE_WINDOW_LOCK: // 11,   /* private */
        logger::log_verbose() << "window lock";
        break;
    case NATIVE_WINDOW_UNLOCK_AND_POST: // 12,   /* private */
        logger::log_verbose() << "unlock and post";
        break;
    case NATIVE_WINDOW_API_CONNECT: // 13,   /* private */
        logger::log_verbose() << "api connect";
        break;
    case NATIVE_WINDOW_API_DISCONNECT: // 14,   /* private */
        logger::log_verbose() << "api disconnect";
        break;
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS: // 15, /* private */
        logger::log_verbose() << "set buffers user dimensions";
        break;
    case NATIVE_WINDOW_SET_POST_TRANSFORM_CROP: // 16,
        logger::log_verbose() << "set post transform crop";
        break;
    case NATIVE_WINDOW_SET_USAGE64: // 30,
    {
        logger::log_verbose() << "set usage 64";
        uint64_t usage = va_arg(args, uint64_t);
        va_end(args);
        return self->setUsage(usage);
    }
    case NATIVE_WINDOW_GET_CONSUMER_USAGE64: // 31,
    {
        logger::log_verbose() << "get consumer usage 64";
        uint64_t* usage = va_arg(args, uint64_t*);
        *usage = self->getUsage();
        break;
    }
    }
    va_end(args);
    return NO_ERROR;
}
