#pragma once
#include "Object.h"
#include "Tickable.h"

class UWorld;

UCLASS(BlueprintType, Abstract, MinimalAPI)
class UActorComponent : public UObject
{
	GENERATED_BODY()

public:
	explicit UActorComponent() : __Owner(nullptr) {};
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
	UWorld* GetWorld() const { return __World; }

	bool IsPendingDestroy() const { return __bPendingDestroy; }

	void MarkPendingDestroy() { __bPendingDestroy = true; }

	FORCEINLINE UObject* GetOwner() { return __Owner; }

	FORCEINLINE void SetOwner(UObject* NewOwner) { __Owner = NewOwner; }

private:
	UObject* __Owner;
	UWorld* __World = nullptr;
	bool	__bPendingDestroy = false;
};

