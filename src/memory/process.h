#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <fstream>
#include <sys/uio.h>

namespace memory {

// Forward declare process_vm_* functions
extern "C" ssize_t process_vm_readv(pid_t pid, const struct iovec *local_iov,
                                    unsigned long liovcnt, const struct iovec *remote_iov,
                                    unsigned long riovcnt, unsigned long flags);
extern "C" ssize_t process_vm_writev(pid_t pid, const struct iovec *local_iov,
                                     unsigned long liovcnt, const struct iovec *remote_iov,
                                     unsigned long riovcnt, unsigned long flags);

class Process {
public:

    Process(int32_t pid);
    ~Process();

    // Move-only class to manage file descriptor resources safely
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&& other) noexcept;
    Process& operator=(Process&& other) noexcept;

    // Factory method to find and open a process by name
    static std::optional<Process> open(const char* processName);

    // Check if process is still valid
    bool isValid() const;

    // Get process ID
    int32_t getPid() const { return pid_; }
    uint64_t getMin() const { return min_; }
    uint64_t getMax() const { return max_; }


    // Memory reading
    template<typename T>
    T read(uint64_t address) const;

    template<typename T>
    T readOrZeroed(uint64_t address) const;

    std::vector<uint8_t> readVec(uint64_t address, size_t length) const;

    template<typename T>
    std::vector<T> readTypedVec(uint64_t address, size_t stride, size_t count) const;

    std::vector<uint8_t> readBytes(uint64_t address, uint64_t count) const;

    std::string readString(uint64_t address);
    std::string readStringUncached(uint64_t address) const;

    // Memory writing
    template<typename T>
    void write(uint64_t address, const T& value) const;

    // Module operations
    std::optional<uint64_t> moduleBaseAddress(const char* moduleName) const;
    std::vector<uint8_t> dumpModule(uint64_t address) const;
    uint64_t moduleSize(uint64_t address) const;

    // Pattern scanning
    std::optional<uint64_t> scan(const char* pattern, uint64_t baseAddress) const;

    // Address resolution
    uint64_t getRelativeAddress(uint64_t instruction, uint64_t offset, uint64_t instructionSize) const;

    // Interface and export operations
    std::optional<uint64_t> getInterfaceOffset(uint64_t baseAddress, const char* interfaceName) const;
    std::optional<uint64_t> getModuleExport(uint64_t baseAddress, const char* exportName) const;
    std::optional<uint64_t> getConvar(uint64_t convarInterface, const char* convarName) const;

    // ELF parsing helpers
    std::optional<uint64_t> getAddressFromDynamicSection(uint64_t baseAddress, uint64_t tag) const;
    std::optional<uint64_t> getSegmentFromPht(uint64_t baseAddress, uint64_t tag) const;

    // Interface function
    uint64_t getInterfaceFunction(uint64_t interfaceAddress, uint64_t index) const;

private:
    static std::optional<int32_t> getPid(const char* processName);

    int32_t pid_;
    int fd_;  // File descriptor for /proc/[pid]/mem
    std::string path_;
    uint64_t min_;
    uint64_t max_;

    // String cache
    mutable std::unordered_map<uint64_t, std::string> stringCache_;
};

// Template implementations
template<typename T>
T Process::read(uint64_t address) const {
    T value{};
    struct iovec local_iov = {
        .iov_base = &value,
        .iov_len = sizeof(T)
    };
    struct iovec remote_iov = {
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = sizeof(T)
    };

    process_vm_readv(pid_, &local_iov, 1, &remote_iov, 1, 0);
    return value;
}

template<typename T>
T Process::readOrZeroed(uint64_t address) const {
    T value{};
    std::memset(&value, 0, sizeof(T));
    struct iovec local_iov = {
        .iov_base = &value,
        .iov_len = sizeof(T)
    };
    struct iovec remote_iov = {
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = sizeof(T)
    };

    process_vm_readv(pid_, &local_iov, 1, &remote_iov, 1, 0);
    return value;
}

template<typename T>
std::vector<T> Process::readTypedVec(uint64_t address, size_t stride, size_t count) const {
    size_t size = sizeof(T);
    if (stride < size) stride = size;

    std::vector<uint8_t> buffer(stride * count);
    struct iovec local_iov = {
        .iov_base = buffer.data(),
        .iov_len = buffer.size()
    };
    struct iovec remote_iov = {
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = buffer.size()
    };

    process_vm_readv(pid_, &local_iov, 1, &remote_iov, 1, 0);

    std::vector<T> result(count);
    for (size_t i = 0; i < count; ++i) {
        std::memcpy(&result[i], buffer.data() + i * stride, size);
    }
    return result;
}

template<typename T>
void Process::write(uint64_t address, const T& value) const {
#ifndef READ_ONLY
    struct iovec local_iov = {
        .iov_base = const_cast<T*>(&value),
        .iov_len = sizeof(T)
    };
    struct iovec remote_iov = {
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = sizeof(T)
    };

    process_vm_writev(pid_, &local_iov, 1, &remote_iov, 1, 0);
#endif
}

} // namespace memory
