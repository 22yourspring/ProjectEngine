#pragma once
#include "Pawn.h"

UCLASS(Blueprintable, MinimalAPI)
class ACharacter : public APawn
{
	GENERATED_BODY()

public:
	ACharacter();
	virtual ~ACharacter() override;

protected:
	virtual void Tick(float _DeltaTime);
};

