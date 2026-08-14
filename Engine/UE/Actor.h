#pragma once

#include "Object.h"
#include "Tickable.h"
#include "ActorComponent.h"
#include "Math/Vector.h"

class UWorld;
class ULevel;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable, MinimalAPI)
class AActor : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class ULevel;

public:
	AActor() = default;
	virtual ~AActor() = default;

	FActorTickFunction PrimaryActorTick;

	virtual bool IsTickable() const override { return PrimaryActorTick.bCanEverTick; }

	virtual void TickActor(float _DeltaTime);
	UWorld* GetWorld() const;

	ULevel* GetLevel() const { return __Level; }

	void SetLevel(ULevel* _Level) { __Level = _Level; }

	const std::vector<std::unique_ptr<UActorComponent>>& GetComponents() const { return __Components; }

	bool SetRootComponent(USceneComponent* _RootComponent);
	USceneComponent* GetRootComponent() const { return __RootComponent; }
	FVector GetActorLocation() const;
	bool SetActorLocation(const FVector& _Location);

	bool DestroyComponent(UActorComponent* _Component);

	bool IsPendingDestroy() const { return __bPendingDestroy; }

	void MarkPendingDestroy() { __bPendingDestroy = true; }

	template <typename ComponentType, typename... Args>
	ComponentType* CreateDefaultSubobject(Args&&... args);

private:
	void RegisterComponentTickFunction(UActorComponent* _Component);
	void UnregisterComponentTickFunction(UActorComponent* _Component);
	void RemoveComponent(UActorComponent* _Component);

protected:
	virtual void Tick(float _DeltaTime) override;

protected:
	virtual void PreInitializeComponents();
	virtual void PostInitializeComponents();

	virtual void BeginPlay();
	virtual void EndPlay();
	
	virtual void Destroy();
	virtual void Destroyed();

protected:
	ULevel* __Level = nullptr;
	std::vector<std::unique_ptr<UActorComponent>> __Components;
	USceneComponent* __RootComponent = nullptr;
	bool __bPendingDestroy = false;

};

#include "Actor.inl"
