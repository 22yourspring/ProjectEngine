#include "pch.h"
#include "Project.h"
#include "UE/ModuleInterface.h"
#include "ProjectGameMode.h"

class FProjectModule final : public IModuleInterface
{
public:
    std::unique_ptr<AGameModeBase> CreateGameMode(bool _UseEngineGameMode) override
    {
        if (_UseEngineGameMode) return std::make_unique<AGameModeBase>();
        return std::make_unique<AProjectGameMode>();
    }
    virtual bool InitializeProject() override { return ::InitializeProject(); }
    virtual bool InitializeProjectWithGameMode(bool _UseEngineGameMode) override { return ::InitializeProjectWithGameMode(_UseEngineGameMode); }
    virtual bool SetProjectPaused(bool _bPaused) override { return ::SetProjectPaused(_bPaused); }
    virtual bool StopProject() override { return ::StopProject(); }
    virtual void LoadProjectInputMappings() override { return ::LoadProjectInputMappings(); }
    virtual bool SetProjectActionMapping(const FString& _MappingName, EKey _Key) override { return ::SetProjectActionMapping(_MappingName, _Key); }
    virtual bool SetProjectAxisMapping(const FString& _MappingName, EKey _Key, float _Scale) override { return ::SetProjectAxisMapping(_MappingName, _Key, _Scale); }
    virtual bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping) override { return ::RemoveProjectActionMapping(_Mapping); }
    virtual bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping) override { return ::RemoveProjectAxisMapping(_Mapping); }
    virtual std::vector<FInputActionKeyMapping> GetProjectActionMappings() override { return ::GetProjectActionMappings(); }
    virtual std::vector<FInputAxisKeyMapping> GetProjectAxisMappings() override { return ::GetProjectAxisMappings(); }
};

extern "C" __declspec(dllexport) unsigned int GetModuleApiVersion()
{
    return GameModuleApiVersion;
}

extern "C" __declspec(dllexport) IModuleInterface* InitializeModule()
{
    return new FProjectModule();
}
