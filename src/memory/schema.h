#pragma once

#include "process.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>

namespace memory {

class Class {
public:
    Class(const Process& process, uint64_t address);

    std::optional<uint64_t> get(const std::string& fieldName) const;
    int32_t size() const { return size_; }
    const std::string& name() const { return name_; }

private:
    std::string name_;
    std::unordered_map<std::string, uint64_t> fields_;
    int32_t size_;
};

class ModuleScope {
public:
    ModuleScope(const Process& process, uint64_t address);

    std::optional<uint64_t> get(const std::string& className, const std::string& fieldName) const;
    const Class* getClass(const std::string& className) const;
    const std::string& name() const { return name_; }

private:
    std::string name_;
    std::unordered_map<std::string, Class> classes_;
};

class Schema {
public:
    static std::optional<Schema> create(const Process& process, uint64_t schemaModule);

    std::optional<uint64_t> get(const std::string& library, const std::string& className,
                                const std::string& fieldName) const;
    const ModuleScope* getLibrary(const std::string& library) const;

private:
    Schema() = default;
    std::unordered_map<std::string, ModuleScope> scopes_;
};

} // namespace memory
