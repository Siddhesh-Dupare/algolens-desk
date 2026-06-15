#pragma once

#include <string>
#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#endif

struct IRHeader {
    uint32_t sequence;
    uint32_t length;
};

class SharedMemory {
    public:
        SharedMemory() = default;
        ~SharedMemory();
        SharedMemory(const SharedMemory&) = delete;
        SharedMemory& operator=(const SharedMemory&) = delete;

        bool CreateWriter(const std::string& name, size_t size);
        bool OpenReader(const std::string& name, size_t size);
        void Write(const std::string& json);
        bool Read(std::string& out, uint32_t& seq);

    private:
        #if defined(_WIN32)
            HANDLE hMapFile_ = nullptr;
        #else
            int fd_ = -1;
            std::string posixName_;
        #endif
            void* ptr_ = nullptr;
            size_t size_ = 0;
};
