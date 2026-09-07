#pragma once

#include "NameTypes.h"
#include "Object.h"

#include <memory>

class ULevel;



UCLASS(Abstract, MinimalAPI)
class ENGINE_API ULevelStreaming : public UObject
{
	GENERATED_BODY()

public:
	explicit ULevelStreaming(const FName& _PackageName = NAME_None);
	explicit ULevelStreaming(const TCHAR* _PackageName);
	virtual ~ULevelStreaming() override;

	virtual bool ShouldBeLoaded() const { return __bShouldBeLoaded; }
	virtual bool ShouldBeVisible() const { return __bShouldBeVisible; }

	void SetShouldBeLoaded(bool _bShouldBeLoaded) { __bShouldBeLoaded = _bShouldBeLoaded; }
	void SetShouldBeVisible(bool _bShouldBeVisible) { __bShouldBeVisible = _bShouldBeVisible; }

	const FName& GetWorldAssetPackageFName() const { return __PackageName; }
	ULevel* GetLoadedLevel() const { return __LoadedLevel.get(); }
	bool HasLoadedLevel() const { return nullptr != __LoadedLevel; }

	
	void SetLoadedLevel(std::unique_ptr<ULevel> _Level);
	void UnloadLevel();

private:
	FName __PackageName;
	std::unique_ptr<ULevel> __LoadedLevel;
	bool __bShouldBeLoaded = false;
	bool __bShouldBeVisible = false;
};

UCLASS(MinimalAPI)
class ENGINE_API ULevelStreamingDynamic : public ULevelStreaming
{
	GENERATED_BODY()

public:
	using ULevelStreaming::ULevelStreaming;
};
