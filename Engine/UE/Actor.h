#pragma once

#include "Object.h"
#include "Tickable.h"
#include "ActorComponent.h"
#include "Math/Vector.h"

#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

class UWorld;
class ULevel;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable, MinimalAPI)
class ENGINE_API AActor : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class ULevel;

public:
	AActor() = default;
	virtual ~AActor() = default;
	AActor(const AActor&) = delete;
	AActor& operator=(const AActor&) = delete;

	FActorTickFunction PrimaryActorTick;

	virtual bool IsTickable() const override { return PrimaryActorTick.bCanEverTick; }

	virtual void TickActor(float _DeltaTime);
	UWorld* GetWorld() const;

	ULevel* GetLevel() const { return __Level; }

	void SetLevel(ULevel* _Level) { __Level = _Level; }

	const std::unordered_set<UActorComponent*>& GetComponents() const { return __OwnedComponents; }
	const std::vector<UActorComponent*>& GetInstanceComponents() const { return __InstanceComponents; }
	bool OwnsComponent(UActorComponent* _Component) const;

	bool SetRootComponent(USceneComponent* _RootComponent);
	USceneComponent* GetRootComponent() const { return __RootComponent; }
	FVector GetActorLocation() const;
	bool SetActorLocation(const FVector& _Location);

	bool DestroyComponent(UActorComponent* _Component);

	bool IsPendingDestroy() const { return __bPendingDestroy; }

	void MarkPendingDestroy() { __bPendingDestroy = true; }

	template <typename ComponentType, typename... Args>
	ComponentType* CreateDefaultSubobject(Args&&... _Args);

	template <typename ComponentType, typename... Args>
	ComponentType* CreateInstanceComponent(Args&&... _Args);

private:
	template <typename ComponentType, typename... Args>
	ComponentType* CreateOwnedComponent(bool _bInstanceComponent, Args&&... _Args);

	bool AddOwnedComponent(UActorComponent* _Component);
	bool AddInstanceComponent(UActorComponent* _Component);
	void RemoveOwnedComponent(UActorComponent* _Component);
	void RemoveInstanceComponent(UActorComponent* _Component);
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
	std::vector<std::unique_ptr<UActorComponent>> __ComponentStorage;
	std::unordered_set<UActorComponent*> __OwnedComponents;
	std::vector<UActorComponent*> __InstanceComponents;
	USceneComponent* __RootComponent = nullptr;
	bool __bPendingDestroy = false;

};

#include "Actor.inl"
