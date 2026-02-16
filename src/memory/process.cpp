#include "process.h"
#include "constants.h"

#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace memory {

Process::Process(int32_t pid) : pid_(pid), fd_(-1), min_(UINT64_MAX), max_(0) {
    if (pid == -1) {
        path_ = "/proc/-1";
        fd_ = ::open("/dev/null", O_RDONLY);
        return;
    }

    path_ = "/proc/" + std::to_string(pid);

    std::string memPath = path_ + "/mem";
    fd_ = ::open(memPath.c_str(), O_RDONLY);
    if (fd_ < 0) {
        fd_ = ::open("/dev/null", O_RDONLY);
    }

    // Calculate min/max address range from loaded libraries
    for (const auto& lib : cs2::LIBS) {
        auto baseAddr = moduleBaseAddress(lib);
        if (baseAddr.has_value()) {
            uint64_t base = baseAddr.value();
            uint64_t size = moduleSize(base);
            uint64_t libMin = base - 1000000;
            uint64_t libMax = base + size + 1000000;
            if (libMin < min_) min_ = libMin;
            if (libMax > max_) max_ = libMax;
        }
    }
}

Process::~Process() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

Process::Process(Process&& other) noexcept 
    : pid_(other.pid_), fd_(other.fd_), path_(std::move(other.path_)), 
      min_(other.min_), max_(other.max_), stringCache_(std::move(other.stringCache_)) {
    other.fd_ = -1;
    other.pid_ = 0;
}

Process& Process::operator=(Process&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }
        pid_ = other.pid_;
        fd_ = other.fd_;
        path_ = std::move(other.path_);
        min_ = other.min_;
        max_ = other.max_;
        stringCache_ = std::move(other.stringCache_);

        other.fd_ = -1;
        other.pid_ = 0;
    }
    return *this;
}

std::optional<int32_t> Process::getPid(const char* processName) {
    DIR* procDir = opendir("/proc");
    if (!procDir) return std::nullopt;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if directory name is numeric (PID)
        bool isNumeric = true;
        for (const char* p = entry->d_name; *p; ++p) {
            if (*p < '0' || *p > '9') {
                isNumeric = false;
                break;
            }
        }
        if (!isNumeric) continue;

        std::string exePath = std::string("/proc/") + entry->d_name + "/exe";
        char linkBuf[PATH_MAX];
        ssize_t len = readlink(exePath.c_str(), linkBuf, sizeof(linkBuf) - 1);
        if (len < 0) continue;
        linkBuf[len] = '\0';

        // Extract executable name
        const char* exeName = strrchr(linkBuf, '/');
        if (exeName) exeName++;
        else exeName = linkBuf;

        if (strcmp(exeName, processName) == 0) {
            closedir(procDir);
            return std::stoi(entry->d_name);
        }
    }

    closedir(procDir);
    return std::nullopt;
}

std::optional<Process> Process::open(const char* processName) {
    auto pid = getPid(processName);
    if (!pid.has_value()) return std::nullopt;

    Process process(pid.value());
    if (!process.isValid()) return std::nullopt;

    return process;
}

bool Process::isValid() const {
    return pid_ > 0 && std::filesystem::exists(path_);
}

std::vector<uint8_t> Process::readVec(uint64_t address, size_t length) const {
    std::vector<uint8_t> buffer(length);

    struct iovec local_iov = {
        .iov_base = buffer.data(),
        .iov_len = length
    };
    struct iovec remote_iov = {
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = length
    };

    process_vm_readv(pid_, &local_iov, 1, &remote_iov, 1, 0);
    return buffer;
}

std::vector<uint8_t> Process::readBytes(uint64_t address, uint64_t count) const {
    std::vector<uint8_t> buffer(count);

    struct iovec local_iov = {
        .iov_base = buffer.data(),
        .iov_len = count
    };
    struct iovec remote_iov = {
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = count
    };

    ssize_t nread = process_vm_readv(pid_, &local_iov, 1, &remote_iov, 1, 0);
    if (nread < 0) {
        // Fallback to pread if process_vm_readv fails
        if (fd_ >= 0) {
            pread(fd_, buffer.data(), count, address);
        }
    }
    return buffer;
}

std::string Process::readString(uint64_t address) {
    auto it = stringCache_.find(address);
    if (it != stringCache_.end()) {
        return it->second;
    }

    std::string result = readStringUncached(address);
    stringCache_[address] = result;
    return result;
}

std::string Process::readStringUncached(uint64_t address) const {
    std::string result;
    result.reserve(64);

    uint64_t addr = address;
    while (true) {
        char c = read<char>(addr);
        if (c == '\0') break;
        result.push_back(c);
        addr++;
        if (result.size() > 1024) break;  // Safety limit
    }

    return result;
}

std::optional<uint64_t> Process::moduleBaseAddress(const char* moduleName) const {
    std::string mapsPath = "/proc/" + std::to_string(pid_) + "/maps";
    std::ifstream mapsFile(mapsPath);
    if (!mapsFile.is_open()) return std::nullopt;

    std::string line;
    while (std::getline(mapsFile, line)) {
        if (line.find(moduleName) == std::string::npos) continue;

        // Extract address from start of line
        size_t dashPos = line.find('-');
        if (dashPos == std::string::npos) continue;

        std::string addrStr = line.substr(0, dashPos);
        uint64_t address = std::stoull(addrStr, nullptr, 16);
        return address;
    }

    return std::nullopt;
}

std::vector<uint8_t> Process::dumpModule(uint64_t address) const {
    uint64_t size = moduleSize(address);
    return readBytes(address, size);
}

uint64_t Process::moduleSize(uint64_t address) const {
    uint64_t sectionHeaderOffset = read<uint64_t>(address + elf::SECTION_HEADER_OFFSET);
    uint16_t sectionHeaderEntrySize = read<uint16_t>(address + elf::SECTION_HEADER_ENTRY_SIZE);
    uint16_t sectionHeaderNumEntries = read<uint16_t>(address + elf::SECTION_HEADER_NUM_ENTRIES);

    return sectionHeaderOffset + static_cast<uint64_t>(sectionHeaderEntrySize) * sectionHeaderNumEntries;
}

std::optional<uint64_t> Process::scan(const char* pattern, uint64_t baseAddress) const {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;

    // Parse pattern string
    std::istringstream stream(pattern);
    std::string token;
    while (stream >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0x00);
            mask.push_back(0x00);
        } else if (token.length() == 2) {
            try {
                uint8_t b = static_cast<uint8_t>(std::stoi(token, nullptr, 16));
                bytes.push_back(b);
                mask.push_back(0xFF);
            } catch (...) {
                // Invalid token, skip
            }
        }
    }

    if (bytes.empty()) return std::nullopt;

    std::vector<uint8_t> module = dumpModule(baseAddress);
    if (module.size() < 500) return std::nullopt;

    size_t patternLen = bytes.size();
    size_t stopIndex = module.size() - patternLen;

    for (size_t i = 0; i < stopIndex; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] == 0xFF && module[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return baseAddress + i;
        }
    }

    return std::nullopt;
}

uint64_t Process::getRelativeAddress(uint64_t instruction, uint64_t offset, uint64_t instructionSize) const {
    int32_t ripAddress = read<int32_t>(instruction + offset);
    return instruction + instructionSize + static_cast<uint64_t>(static_cast<int64_t>(ripAddress));
}

std::optional<uint64_t> Process::getInterfaceOffset(uint64_t baseAddress, const char* interfaceName) const {
    auto createInterface = getModuleExport(baseAddress, "CreateInterface");
    if (!createInterface.has_value()) return std::nullopt;

    uint64_t exportAddress = createInterface.value() + 0x10;
    uint64_t interfaceEntry = read<uint64_t>(exportAddress + 0x07 + read<uint32_t>(exportAddress + 0x03));

    while (interfaceEntry != 0) {
        uint64_t entryNameAddress = read<uint64_t>(interfaceEntry + 8);
        std::string entryName = readStringUncached(entryNameAddress);

        if (entryName.find(interfaceName) == 0) {
            uint64_t vfuncAddress = read<uint64_t>(interfaceEntry);
            return static_cast<uint64_t>(read<uint32_t>(vfuncAddress + 0x03)) + vfuncAddress + 0x07;
        }

        interfaceEntry = read<uint64_t>(interfaceEntry + 0x10);
    }

    return std::nullopt;
}

std::optional<uint64_t> Process::getModuleExport(uint64_t baseAddress, const char* exportName) const {
    constexpr uint64_t add = 0x18;

    auto stringTable = getAddressFromDynamicSection(baseAddress, 0x05);
    if (!stringTable.has_value()) return std::nullopt;

    auto symbolTableOpt = getAddressFromDynamicSection(baseAddress, 0x06);
    if (!symbolTableOpt.has_value()) return std::nullopt;

    uint64_t symbolTable = symbolTableOpt.value() + add;

    while (read<uint32_t>(symbolTable) != 0) {
        uint32_t stName = read<uint32_t>(symbolTable);
        std::string name = readStringUncached(stringTable.value() + stName);
        if (name == exportName) {
            return read<uint64_t>(symbolTable + 0x08) + baseAddress;
        }
        symbolTable += add;
    }

    return std::nullopt;
}

std::optional<uint64_t> Process::getAddressFromDynamicSection(uint64_t baseAddress, uint64_t tag) const {
    auto dynamicSection = getSegmentFromPht(baseAddress, elf::DYNAMIC_SECTION_PHT_TYPE);
    if (!dynamicSection.has_value()) return std::nullopt;

    constexpr uint64_t registerSize = 8;
    uint64_t address = read<uint64_t>(dynamicSection.value() + 2 * registerSize) + baseAddress;

    while (true) {
        uint64_t tagValue = read<uint64_t>(address);
        if (tagValue == 0) break;
        if (tagValue == tag) {
            return read<uint64_t>(address + registerSize);
        }
        address += registerSize * 2;
    }

    return std::nullopt;
}

std::optional<uint64_t> Process::getSegmentFromPht(uint64_t baseAddress, uint64_t tag) const {
    uint64_t firstEntry = read<uint64_t>(baseAddress + elf::PROGRAM_HEADER_OFFSET) + baseAddress;
    uint16_t entrySize = read<uint16_t>(baseAddress + elf::PROGRAM_HEADER_ENTRY_SIZE);
    uint16_t numEntries = read<uint16_t>(baseAddress + elf::PROGRAM_HEADER_NUM_ENTRIES);

    for (uint16_t i = 0; i < numEntries; ++i) {
        uint64_t entry = firstEntry + static_cast<uint64_t>(i) * entrySize;
        if (read<uint32_t>(entry) == static_cast<uint32_t>(tag)) {
            return entry;
        }
    }

    return std::nullopt;
}

std::optional<uint64_t> Process::getConvar(uint64_t convarInterface, const char* convarName) const {
    if (convarInterface == 0) return std::nullopt;

    uint64_t objects = read<uint64_t>(convarInterface + 0x48);
    uint32_t count = read<uint32_t>(convarInterface + 160);

    for (uint32_t i = 0; i < count; ++i) {
        uint64_t object = read<uint64_t>(objects + i * 16);
        if (object == 0) break;

        uint64_t nameAddress = read<uint64_t>(object);
        std::string name = readStringUncached(nameAddress);
        if (name == convarName) {
            return object;
        }
    }

    return std::nullopt;
}

uint64_t Process::getInterfaceFunction(uint64_t interfaceAddress, uint64_t index) const {
    return read<uint64_t>(read<uint64_t>(interfaceAddress) + (index * 8));
}




}
