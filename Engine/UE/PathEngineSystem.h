#pragma once

#include "EngineSystem.h"
#include <filesystem>

class ENGINE_API PathEngineSystem final : public IEngineSystem
{
public:
	virtual HRESULT Initialize() override;
	virtual void Deinitialize() override;
	virtual void Tick(float _DeltaTime) override { UNREFERENCED_PARAMETER(_DeltaTime); }
	virtual bool IsTickable() const override { return false; }

	HRESULT Configure(
		const std::filesystem::path& _RootDirectory,
		const std::filesystem::path& _EngineDirectory,
		const std::filesystem::path& _ProjectDirectory);
	HRESULT ConfigureFromProjectFile(const std::filesystem::path& _ProjectFile);
	static std::filesystem::path FindProjectFile(const std::filesystem::path& _StartDirectory);

	const std::filesystem::path& GetRootDirectory() const { return __RootDirectory; }
	const std::filesystem::path& GetEngineDirectory() const { return __EngineDirectory; }
	const std::filesystem::path& GetProjectDirectory() const { return __ProjectDirectory; }
    const std::filesystem::path& GetProjectFile() const { return __ProjectFile; }

	std::filesystem::path GetEngineContentDirectory() const;
	std::filesystem::path GetProjectContentDirectory() const;
	std::filesystem::path GetProjectSavedDirectory() const;
	std::filesystem::path GetProjectModuleFile(const TCHAR* _TargetName) const;

private:
	HRESULT ApplyConfiguration(const std::filesystem::path& _RootDirectory,
		const std::filesystem::path& _EngineDirectory,
		const std::filesystem::path& _ProjectDirectory,
		const std::filesystem::path& _ProjectContentDirectoryName);

	std::filesystem::path __RootDirectory;
    std::filesystem::path __ProjectFile;
	std::filesystem::path __EngineDirectory;
	std::filesystem::path __ProjectDirectory;
	std::filesystem::path __ProjectModuleName;
	std::filesystem::path __ProjectContentDirectoryName = TEXT("Content");
};
