#pragma once

#include "Object.h"
#include "Actor.h"
#include "NameTypes.h"
#include <type_traits>

class UWorld;

UCLASS(MinimalAPI)
class ENGINE_API ULevel : public UObject
{
	GENERATED_BODY()

public:
	explicit ULevel(UWorld* _OwningWorld, const FName& _LevelName = NAME_None);
	ULevel(const ULevel&) = delete;
	ULevel& operator=(const ULevel&) = delete;

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... _Args);

	bool DestroyActor(AActor* _Actor);
	void Tick(float _DeltaTime);
	void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime);
	void Render();

	void SetVisible(bool _bVisible) { __bVisible = _bVisible; }
	bool IsVisible() const { return __bVisible; }

	UWorld* GetWorld() const { return __OwningWorld; }
	const FName& GetLevelName() const { return __LevelName; }
    FString GetMapName() const;
    void SetLevelName(const FName& _Name) { __LevelName = _Name; }
    const std::vector<std::unique_ptr<AActor>>& GetActors() const { return __Actors; }

private:
	UWorld*	__OwningWorld = nullptr;
	FName	__LevelName;
	std::vector<std::unique_ptr<AActor>>	__Actors;
	bool __bVisible = true;
};

#include "Level.inl"
