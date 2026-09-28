#pragma once

#include "define.h"
#include "ModuleInterface.h"
#include <filesystem>

class CORE_API FModuleManager final
{
public:
    FModuleManager();
    ~FModuleManager();
    FModuleManager(const FModuleManager&) = delete;
    FModuleManager& operator=(const FModuleManager&) = delete;

    bool LoadGameModule(const std::filesystem::path& _ModuleFile);
    static IModuleInterface* GetGameModule();
    const std::filesystem::path& GetLoadedModuleFile() const;

private:
    void* __Library = nullptr;
    IModuleInterface* __GameModule = nullptr;
    std::filesystem::path __LoadedModuleFile;
};
