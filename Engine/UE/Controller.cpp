#include "pch.h"
#include "Controller.h"
#include "Pawn.h"

AController::AController()
	: __ControlledPawn(nullptr)
{
}

AController::~AController()
{
	UnPossess();
}

void AController::Possess(APawn* _NewPawn)
{
	if (__ControlledPawn == _NewPawn)
		return;

	UnPossess();

	if (nullptr == _NewPawn)
		return;

	if (AController* PreviousController = _NewPawn->GetController())
		PreviousController->UnPossess();

	__ControlledPawn = _NewPawn;
	__ControlledPawn->PossessedBy(this);
}

void AController::UnPossess()
{
	if (nullptr == __ControlledPawn)
		return;

	APawn* PreviousPawn = __ControlledPawn;
	__ControlledPawn = nullptr;
	PreviousPawn->UnPossessed();
}

void AController::Tick(float _DeltaTime)
{
	Super::Tick(_DeltaTime);
	if (__ControlledPawn)
	{		
	}
}
