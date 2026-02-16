#include "schema.h"

namespace memory {

// Field helper struct
struct Field {
    std::string name;
    uint64_t offset;

    Field(const Process& process, uint64_t address) {
        name = process.readStringUncached(process.read<uint64_t>(address));
        offset = static_cast<uint64_t>(process.read<int32_t>(address + 0x10));
    }
};

// Class implementation
Class::Class(const Process& process, uint64_t address) : size_(0) {
    name_ = process.readStringUncached(process.read<uint64_t>(address + 0x08));

    int16_t fieldCount = process.read<int16_t>(address + 0x1C);
    size_ = process.read<int32_t>(address + 0x18);

    if (fieldCount < 0 || fieldCount > 20000) {
        return;
    }

    uint64_t fieldsVec = process.read<uint64_t>(address + 0x28);
    for (int16_t i = 0; i < fieldCount; ++i) {
        Field field(process, fieldsVec + (0x20 * i));
        fields_[field.name] = field.offset;
    }
}

std::optional<uint64_t> Class::get(const std::string& fieldName) const {
    auto it = fields_.find(fieldName);
    if (it != fields_.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ModuleScope implementation
ModuleScope::ModuleScope(const Process& process, uint64_t address) {
    name_ = process.readStringUncached(address + 0x08);

    // Has 1024 buckets
    uint64_t hashVector = address + 0x560 + 0x90;
    for (int i = 0; i < 1024; ++i) {
        // first_uncommitted
        uint64_t currentElement = process.read<uint64_t>(hashVector + (i * 24) + 0x28);

        while (currentElement != 0) {
            uint64_t data = process.read<uint64_t>(currentElement + 0x10);
            if (data != 0) {
                Class cls(process, data);
                classes_.emplace(cls.name(), std::move(cls));
            }
            currentElement = process.read<uint64_t>(currentElement + 0x08);
        }
    }

    // free_list (HashAllocatedBlob)
    uint64_t currentBlob = process.read<uint64_t>(address + 0x560 + 0x20);
    while (currentBlob != 0) {
        uint64_t data = process.read<uint64_t>(currentBlob + 0x10);
        if (data > process.getMin() && data < process.getMax()) {
            Class cls(process, data);
            classes_.emplace(cls.name(), std::move(cls));
        }
        currentBlob = process.read<uint64_t>(currentBlob);
    }
}

std::optional<uint64_t> ModuleScope::get(const std::string& className, const std::string& fieldName) const {
    auto classIt = classes_.find(className);
    if (classIt == classes_.end()) {
        return std::nullopt;
    }
    return classIt->second.get(fieldName);
}

const Class* ModuleScope::getClass(const std::string& className) const {
    auto it = classes_.find(className);
    if (it != classes_.end()) {
        return &it->second;
    }
    return nullptr;
}

// Schema implementation
std::optional<Schema> Schema::create(const Process& process, uint64_t schemaModule) {
    auto schemaSystemAddr = process.scan(
        "48 8D 3D ? ? ? ? E8 ? ? ? ? 48 8B BD ? ? ? ? 31 F6 E8 ? ? ? ? E9",
        schemaModule
    );
    if (!schemaSystemAddr.has_value()) return std::nullopt;

    uint64_t schemaSystem = process.getRelativeAddress(schemaSystemAddr.value(), 3, 7);

    int32_t typeScopesLen = process.read<int32_t>(schemaSystem + 0x1F0);
    uint64_t typeScopesVec = process.read<uint64_t>(schemaSystem + 0x1F8);

    Schema schema;
    for (int32_t i = 0; i < typeScopesLen; ++i) {
        uint64_t typeScopeAddress = process.read<uint64_t>(typeScopesVec + (i * 8));
        ModuleScope scope(process, typeScopeAddress);
        schema.scopes_.emplace(scope.name(), std::move(scope));
    }

    return schema;
}

std::optional<uint64_t> Schema::get(const std::string& library, const std::string& className,
                                    const std::string& fieldName) const {
    auto scopeIt = scopes_.find(library);
    if (scopeIt == scopes_.end()) return std::nullopt;
    return scopeIt->second.get(className, fieldName);
}

const ModuleScope* Schema::getLibrary(const std::string& library) const {
    auto it = scopes_.find(library);
    if (it != scopes_.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace memory
