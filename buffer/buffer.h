#pragma once

#include <cstring>
#include <iostream>
#include <unistd.h>     // write
#include <sys/uio.h>    // read
#include <vector>
#include <atomic>
#include <assert.h>

class Buffer {
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    // 返回缓冲区中可写字节数、可读字节数和可预置字节数
    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;          // 返回缓冲区中可读数据的起始地址
    void EnsureWriteable(size_t len);  // 确保缓冲区中有足够的可写空间
    void HasWritten(size_t len);

    void Retrieve(size_t len);            // 读走len长度的数据
    void RetrieveUntil(const char* end);  // 读走直到end的数据
    void RetrieveAll();                   // 读走所有数据
    std::string RetrieveAllToStr();       // 读走所有数据并返回字符串

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);


private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    /*
        已读取的数据：从缓冲区的起始位置到 readPos_ 位置的数据，这些数据已经被读取过，可以被重新利用或预置新的数据。
        可读的数据：从 readPos_ 位置到 writePos_ 位置的数据，这些数据是当前缓冲区中尚未被读取的数据。
        可写的数据：从 writePos_ 位置到缓冲区末尾的数据，这些数据是当前缓冲区中尚未被写入的数据。
    */
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};