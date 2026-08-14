#pragma once
#include "Actor.h"

UCLASS(Abstract, MinimalAPI)
class AController : public AActor
{
	GENERATED_BODY()

public:
	AController();
	virtual ~AController() override;

	virtual void Possess(class APawn* _NewPawn);
	virtual void UnPossess();
	virtual void Tick(float _DeltaTime);

	class APawn* GetPawn() const { return __ControlledPawn; }

protected:
	class APawn* __ControlledPawn;
};

