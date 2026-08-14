#include "pch.h"
#include "Pawn.h"
#include "InputComponent.h"
#include "Controller.h"

APawn::APawn()
{
	__InputComponent = CreateDefaultSubobject<UInputComponent>();
}

APawn::~APawn()
{
	if (nullptr != __Controller)
		__Controller->UnPossess();
}

void APawn::Tick(float _DeltaTime)
{
	Super::Tick(_DeltaTime);
}

void APawn::SetupPlayerInputComponent(UInputComponent* _InputComponent)
{
	UNREFERENCED_PARAMETER(_InputComponent);
}

void APawn::PossessedBy(AController* _NewController)
{
	__Controller = _NewController;

	if (nullptr == __InputComponent)
		return;

	if (false == __bInputBindingsInitialized)
	{
		SetupPlayerInputComponent(__InputComponent);
		__bInputBindingsInitialized = true;
	}

}

void APawn::UnPossessed()
{
	__Controller = nullptr;
}

