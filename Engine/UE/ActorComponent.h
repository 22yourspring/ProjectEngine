#pragma once
#include "Object.h"
#include "Tickable.h"

class UWorld;
class AActor;

UCLASS(BlueprintType, Abstract, MinimalAPI)
class ENGINE_API UActorComponent : public UObject
{
	GENERATED_BODY()

public:
	explicit UActorComponent() = default;
	virtual ~UActorComponent() = default;

public:
	FComponentTickFunction PrimaryComponentTick;
	virtual void TickComponent(float _DeltaTime);
	virtual void BeginPlay();
	virtual void EndPlay();	
	virtual void InitializeComponent();
	virtual void DestroyComponent();
	virtual void OnRegister();
	virtual void OnUnregister();
	void RegisterComponentWithWorld(UWorld* _World);
	void UnregisterComponent();
	UWorld* GetWorld() const override { return __World; }

	bool IsPendingDestroy() const { return __bPendingDestroy; }

	void MarkPendingDestroy() { __bPendingDestroy = true; }

	FORCEINLINE AActor* GetOwner() const { return __Owner; }

private:
	friend class AActor;

	FORCEINLINE void SetOwner(AActor* _NewOwner) { __Owner = _NewOwner; }

	AActor* __Owner = nullptr;
	UWorld* __World = nullptr;
	bool	__bPendingDestroy = false;
};

