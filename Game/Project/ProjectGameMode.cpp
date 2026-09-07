#include "pch.h"
#include "ProjectGameMode.h"
#include "Player.h"
#include "UE/PlayerController.h"
#include "UE/World.h"

AProjectGameMode::AProjectGameMode()
{
	SetDefaultPawn([](UWorld* _World)
	{
		return nullptr != _World
			? _World->SpawnActor<APlayer>()
			: nullptr;
	});

	SetPlayerControllerFactory([](UWorld* _World)
	{
		return nullptr != _World
			? _World->SpawnActor<APlayerController>()
			: nullptr;
	});
}
