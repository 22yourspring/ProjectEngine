#include "pch.h"
#include "PlayerController.h"

#include "InputComponent.h"
#include "Engine.h"
#include "InputSubsystem.h"
#include "Pawn.h"
#include "PlayerInput.h"

#include <algorithm>

APlayerController::APlayerController()
{
	__PlayerInput = std::make_unique<UPlayerInput>();
	__InputComponent = CreateDefaultSubobject<UInputComponent>();
	SetupInputComponent();
}

APlayerController::~APlayerController()
{
	if (InputSubsystem* Input = GEngine->GetSubsystem<InputSubsystem>())
		Input->ClearPlayerController(this);
}

void APlayerController::PushInputComponent(UInputComponent* _InputComponent)
{
	if (nullptr == _InputComponent)
		return;

	PopInputComponent(_InputComponent);
	__PushedInputComponents.push_back(_InputComponent);
}

bool APlayerController::PopInputComponent(UInputComponent* _InputComponent)
{
	const auto Iter = std::remove(
		__PushedInputComponents.begin(), __PushedInputComponents.end(), _InputComponent);
	const bool bRemoved = __PushedInputComponents.end() != Iter;
	__PushedInputComponents.erase(Iter, __PushedInputComponents.end());
	return bRemoved;
}

void APlayerController::SetupInputComponent()
{
}

void APlayerController::BuildInputStack(std::vector<UInputComponent*>& _InputStack)
{
	if (nullptr != __InputComponent)
		_InputStack.push_back(__InputComponent);

	if (APawn* Pawn = GetPawn())
	{
		if (UInputComponent* PawnInputComponent = Pawn->GetInputComponent())
			_InputStack.push_back(PawnInputComponent);
	}

	for (UInputComponent* InputComponent : __PushedInputComponents)
	{
		if (nullptr != InputComponent)
			_InputStack.push_back(InputComponent);
	}

	std::stable_sort(_InputStack.begin(), _InputStack.end(),
		[](const UInputComponent* _Left, const UInputComponent* _Right)
		{
			return _Left->__Priority < _Right->__Priority;
		});
}

void APlayerController::TickPlayerInput(const std::vector<FKeyInputEvent>& _KeyEvents,
	const std::vector<FAnalogInputEvent>& _AnalogEvents,
	float _DeltaTime, bool _bGamePaused)
{
	if (nullptr == __PlayerInput)
		return;

	std::vector<UInputComponent*> InputStack;
	BuildInputStack(InputStack);
	__PlayerInput->ProcessInputStack(InputStack, _KeyEvents, _AnalogEvents, _DeltaTime, _bGamePaused);
}
