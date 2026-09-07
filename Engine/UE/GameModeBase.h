#pragma once

#include "Info.h"

#include <functional>

class APawn;
class APlayerController;
class UWorld;

UCLASS(MinimalAPI)
class ENGINE_API AGameModeBase : public AInfo
{
	GENERATED_BODY()

public:
	AGameModeBase();
	~AGameModeBase() override;

	using PawnFactory = std::function<APawn*(UWorld*)>;
	using PlayerControllerFactory = std::function<APlayerController*(UWorld*)>;

	void SetDefaultPawn(PawnFactory _Factory);
	void SetPlayerControllerFactory(PlayerControllerFactory _Factory);

	bool StartPlay(UWorld* _World);
	APawn* GetDefaultPawn() const { return __DefaultPawn; }
	APlayerController* GetPlayerController() const { return __PlayerController; }

protected:
	virtual APawn* SpawnDefaultPawn(UWorld* _World);
	virtual APlayerController* SpawnPlayerController(UWorld* _World);

private:
	PawnFactory __DefaultPawnFactory;
	PlayerControllerFactory __PlayerControllerFactory;
	APawn* __DefaultPawn = nullptr;
	APlayerController* __PlayerController = nullptr;
};
