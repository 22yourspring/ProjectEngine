#pragma once
#include "Actor.h"

class UInputComponent;
class AController;

UCLASS(BlueprintType, Blueprintable, MinimalAPI)
class APawn : public AActor
{
	GENERATED_BODY()

	friend class AController;

public:
	APawn();
	virtual ~APawn() override;

	UInputComponent* GetInputComponent() const { return __InputComponent; }
	AController* GetController() const { return __Controller; }

protected:
	virtual void Tick(float _DeltaTime);
	virtual void SetupPlayerInputComponent(UInputComponent* _InputComponent);
	virtual void PossessedBy(AController* _NewController);
	virtual void UnPossessed();

private:
	UInputComponent* __InputComponent = nullptr;
	AController* __Controller = nullptr;
	bool __bInputBindingsInitialized = false;
};

