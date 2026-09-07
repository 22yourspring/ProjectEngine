#include "pch.h"
#include "GameModeBase.h"
#include "MannequinPawn.h"
#include "Pawn.h"
#include "PlayerController.h"
#include "World.h"

AGameModeBase::AGameModeBase()
{
	__DefaultPawnFactory = [](UWorld* _World)
	{
		return nullptr != _World
			? _World->SpawnActor<AMannequinPawn>()
			: nullptr;
	};

	__PlayerControllerFactory = [](UWorld* _World)
	{
		return nullptr != _World
			? _World->SpawnActor<APlayerController>()
			: nullptr;
	};
}

AGameModeBase::~AGameModeBase() = default;

void AGameModeBase::SetDefaultPawn(PawnFactory _Factory)
{
	if (_Factory)
		__DefaultPawnFactory = std::move(_Factory);
}

void AGameModeBase::SetPlayerControllerFactory(PlayerControllerFactory _Factory)
{
	if (_Factory)
		__PlayerControllerFactory = std::move(_Factory);
}

APawn* AGameModeBase::SpawnDefaultPawn(UWorld* _World)
{
	return __DefaultPawnFactory ? __DefaultPawnFactory(_World) : nullptr;
}

APlayerController* AGameModeBase::SpawnPlayerController(UWorld* _World)
{
	return __PlayerControllerFactory
		? __PlayerControllerFactory(_World)
		: nullptr;
}

bool AGameModeBase::StartPlay(UWorld* _World)
{
	if (nullptr == _World)
		return false;

	__PlayerController = SpawnPlayerController(_World);
	__DefaultPawn = SpawnDefaultPawn(_World);

	if (nullptr == __PlayerController || nullptr == __DefaultPawn)
		return false;

	__PlayerController->Possess(__DefaultPawn);
	return __PlayerController->GetPawn() == __DefaultPawn;
}
