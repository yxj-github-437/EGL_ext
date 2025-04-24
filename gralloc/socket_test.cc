#include "logger.h"
#include "gralloc_adapter.h"

#include <android/hardware_buffer.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <iostream>
#include <vector>

void send_handle(int socket_fd, std::shared_ptr<gralloc_buffer> buffer);
std::shared_ptr<gralloc_buffer> recv_buffer(int socket_fd);
auto adapter = gralloc_adapter_t::loader::getInstance().get_adapter();

int main()
{
    int fd_pair[2] = {};
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fd_pair) < 0)
    {
        logger::log_error() << "create socket pair failed";
        return 1;
    }

    pid_t fork_pid = fork();
    if (fork_pid < 0)
    {
        logger::log_error() << "fork failed";
        return 1;
    }

    if (fork_pid > 0)
    { // parent process
        close(fd_pair[0]);
        logger::log_info() << "parent pid:" << getpid();

        auto buffer = adapter->allocate_buffer(
            100, 100, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER |
                AHARDWAREBUFFER_USAGE_CPU_READ_MASK |
                AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK);
        if (buffer)
        {

            void* vaddr = nullptr;
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
            buffer->unlock();
            send_handle(fd_pair[1], buffer);
        }
        else
        {
            kill(fork_pid, -9);
        }

        int status;
        if (waitpid(fork_pid, &status, 0) < 0)
        {
            logger::log_info() << "waitpid failed";
            return 1;
        }

        logger::log_info() << "chile proc exit with " << status;
        close(fd_pair[1]);
    }
    else
    { // child process
        close(fd_pair[1]);
        logger::log_info() << "child pid:" << getpid();

        auto buffer = recv_buffer(fd_pair[0]);
        if (buffer)
        {
            void* vaddr = nullptr;
            ARect rect = {0, 0, 100, 100};
            buffer->lock(AHARDWAREBUFFER_USAGE_CPU_READ_MASK |
                             AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK,
                         rect, &vaddr);
            if (vaddr)
            {
                logger::log_info() << "get vaddr " << vaddr
                                   << ", read vaddr[0]: " << std::showbase
                                   << std::hex << ((uint32_t*)vaddr)[0];

                char c = getchar();
            }
        }
        else
        {
            logger::log_info() << "recv buffer failed";
        }

        close(fd_pair[0]);
    }
}

constexpr int kFdBufferSize = 128 * sizeof(int);

void send_handle(int socket_fd, std::shared_ptr<gralloc_buffer> buffer)
{
    std::vector<uint8_t> bufs{};

    AHardwareBuffer_Desc desc = {};
    desc.width = buffer->width;
    desc.height = buffer->height;
    desc.stride = buffer->stride;
    desc.format = buffer->format;
    desc.layers = 1;
    desc.usage = buffer->usage;

    bufs.resize(sizeof(desc) + buffer->handle->numInts * sizeof(int));
    memcpy(bufs.data(), &desc, sizeof(desc));
    memcpy(bufs.data() + sizeof(desc),
           &buffer->handle->data[buffer->handle->numFds],
           buffer->handle->numInts * sizeof(int));

    logger::log_info() << "numFds: " << buffer->handle->numFds
                       << ", numInts: " << buffer->handle->numInts;

    struct iovec iov[1];
    iov[0].iov_base = bufs.data();
    iov[0].iov_len = bufs.size();

    char buf[CMSG_SPACE(kFdBufferSize)];
    struct msghdr msg = {
        .msg_iov = &iov[0],
        .msg_iovlen = 1,
        .msg_control = buf,
        .msg_controllen = sizeof(buf),
    };

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int) * buffer->handle->numFds);
    int* fdData = reinterpret_cast<int*>(CMSG_DATA(cmsg));
    memcpy(fdData, buffer->handle->data, sizeof(int) * buffer->handle->numFds);
    msg.msg_controllen = cmsg->cmsg_len;

    int result;
    do
    {
        result = sendmsg(socket_fd, &msg, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
    {
        result = errno;
        logger::log_error()
            << "Error writing buffer to socket: error: " << strerror(result);
    }
}

std::shared_ptr<gralloc_buffer> recv_buffer(int socket_fd)
{
    static constexpr int kMessageBufferSize = 4096 * sizeof(int);

    std::vector<uint8_t> dataBuf{};
    dataBuf.resize(kMessageBufferSize);

    char fdBuf[CMSG_SPACE(kFdBufferSize)]{};
    struct iovec iov[1];
    iov[0].iov_base = dataBuf.data();
    iov[0].iov_len = dataBuf.size();

    struct msghdr msg = {
        .msg_iov = &iov[0],
        .msg_iovlen = 1,
        .msg_control = fdBuf,
        .msg_controllen = sizeof(fdBuf),
    };

    int result;
    do
    {
        result = recvmsg(socket_fd, &msg, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
    {
        result = errno;
        logger::log_error()
            << "Error reading buffer from socket: error: " << strerror(result);
        return nullptr;
    }

    if (msg.msg_iovlen != 1)
    {
        logger::log_error()
            << "Error reading buffer from socket: bad data length";
        return nullptr;
    }

    if (msg.msg_controllen % sizeof(int) != 0)
    {
        logger::log_error()
            << "Error reading buffer from socket: bad fd length";
        return nullptr;
    }

    size_t dataLen = result;
    if (dataLen % sizeof(int) != 0)
    {
        logger::log_error()
            << "Error reading buffer from socket: bad data length";
        return nullptr;
    }

    const void* data = static_cast<const void*>(msg.msg_iov[0].iov_base);
    if (!data)
    {
        logger::log_error()
            << "Error reading buffer from socket: no buffer data";
        return nullptr;
    }

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg)
    {
        logger::log_error() << "Error reading buffer from socket: no fd header";
        return nullptr;
    }

    size_t fdCount = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);

    const int* fdData = reinterpret_cast<const int*>(CMSG_DATA(cmsg));
    if (!fdData)
    {
        logger::log_error() << "Error reading buffer from socket: no fd data";
        return nullptr;
    }

    auto desc = (AHardwareBuffer_Desc*)data;
    int numInts = (dataLen - sizeof(AHardwareBuffer_Desc)) / sizeof(int);
    int* ints_array = (int*)((uint8_t*)data + sizeof(AHardwareBuffer_Desc));
    auto handle = native_handle_create(fdCount, numInts);

    logger::log_info() << "numFds: " << handle->numFds
                       << ", numInts: " << handle->numInts;

    memcpy((int*)handle->data, fdData, fdCount * sizeof(int));
    memcpy((int*)handle->data + handle->numFds, ints_array,
           numInts * sizeof(int));

    auto buffer =
        adapter->import_buffer(handle, desc->width, desc->height, desc->stride,
                               desc->format, desc->usage);

    native_handle_close(handle);
    native_handle_delete((native_handle_t*)handle);
    return buffer;
}
