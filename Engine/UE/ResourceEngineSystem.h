#pragma once

#include "EngineSystem.h"
#include "Math/Vector.h"

#include <filesystem>
#include <mutex>
#include <vector>

USTRUCT()
struct ENGINE_API FLevelAssetData
{
	GENERATED_BODY()

	FVector PlayerLocation = {};
	bool bHasPlayerLocation = false;
};

UCLASS(MinimalAPI)
class ENGINE_API ResourceEngineSystem final : public IEngineSystem
{
	GENERATED_BODY()

public:
	HRESULT Initialize() override;
	void Deinitialize() override;
	void Tick(float _DeltaTime) override { UNREFERENCED_PARAMETER(_DeltaTime); }
	bool IsTickable() const override { return false; }

	void AddContentRoot(const TCHAR* _ContentRoot);
	bool LoadLevelAsset(const TCHAR* _LevelName, FLevelAssetData& _OutData) const;

private:
	mutable std::mutex __ContentRootMutex;
	std::vector<std::filesystem::path> __ContentRoots;
};
