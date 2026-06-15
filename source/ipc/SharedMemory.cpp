#include "SharedMemory.h"

#include <cstring>

SharedMemory::~SharedMemory() {
    #if defined(_WIN32)
        if (ptr_) UnmapViewOfFile(ptr_);
        if (hMapFile_) CloseHandle(hMapFile_);
    #endif
        ptr_ = nullptr;
        hMapFile_ = nullptr;
}

bool SharedMemory::CreateWriter(const std::string& name, size_t size) {
    size_ = size;
    #if defined(_WIN32)
        hMapFile_ = CreateFileMappingA(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name.c_str());

        if (!hMapFile_) return false;

        ptr_ = MapViewOfFile(hMapFile_, FILE_MAP_ALL_ACCESS, 0, 0, size);

        if (!ptr_) {
            CloseHandle(hMapFile_);
            hMapFile_ = nullptr;
            return false;
        }
    #endif

    return true;
}

bool SharedMemory::OpenReader(const std::string& name, size_t size) {
    size_ = size;
    #if defined(_WIN32)
        hMapFile_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());

        if (!hMapFile_) return false;

        ptr_ = MapViewOfFile(hMapFile_, FILE_MAP_ALL_ACCESS, 0, 0, size);

        if (!ptr_) {
            CloseHandle(hMapFile_);
            hMapFile_ = nullptr;
            return false;
        }
    #endif

    return true;
}

void SharedMemory::Write(const std::string& json) {
    if (!ptr_) return;

    if (sizeof(IRHeader) + json.size() > size_) return;

    auto* header = reinterpret_cast<IRHeader*>(ptr_);
    char* data = reinterpret_cast<char*>(ptr_) + sizeof(IRHeader);

    memcpy(data, json.data(), json.size());
    header->length = static_cast<uint32_t>(json.size());

    header->sequence = header->sequence + 1;
}

bool SharedMemory::Read(std::string& out, uint32_t& seq) {
    if (!ptr_) return false;

    auto* header = reinterpret_cast<IRHeader*>(ptr_);
    const char* data = reinterpret_cast<char*>(ptr_) + sizeof(IRHeader);

    uint32_t seqBefore = header->sequence;
    uint32_t length = header->length;

    if (length == 0) return false;

    if (length > size_ - sizeof(IRHeader)) return false;

    out.assign(data, length);

    if (header->sequence != seqBefore) return false;

    seq = seqBefore;
    return true;
}
