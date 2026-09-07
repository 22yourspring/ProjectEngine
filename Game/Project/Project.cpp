


#include "pch.h"
#include "Project.h"
#include "Player.h"
#include "ProjectGameInstanceSubsystem.h"
#include "ProjectGameMode.h"
#include "UE/Engine.h"
#include "UE/GameInstance.h"
#include "UE/Level.h"
#include "UE/World.h"
#include "UE/InputEngineSystem.h"
#include "UE/PlayerController.h"
#include "UE/PlayerInput.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/GameplayStatics.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <vector>

namespace
{
	UPlayerInput* ProjectPlayerInput = nullptr;
	std::vector<FInputActionKeyMapping> SavedActionMappings;
	std::vector<FInputAxisKeyMapping> SavedAxisMappings;

	const std::filesystem::path InputSettingsPath =
		std::filesystem::path("Saved") / "Config" / "InputMappings.cfg";
	const std::filesystem::path GameModeSettingsPath =
		std::filesystem::path("Saved") / "Config" / "EditorDefaultPawn.cfg";

	std::string ReadGameModeSelection()
	{
		std::ifstream Input(GameModeSettingsPath);
		std::string Selection;
		if (Input.is_open())
			Input >> Selection;
		return Selection;
	}
	void LoadInputSettings(UPlayerInput* _PlayerInput);
	void StoreActionMapping(const std::string& _MappingName, EKey _Key);
	void StoreAxisMapping(const std::string& _MappingName, EKey _Key, float _Scale);
	void ConfigureMovementMappings(UPlayerInput* _PlayerInput, bool _bProjectGameMode);

	bool ConfigureProjectWorld(UWorld* _World)
	{
		if (nullptr == _World)
			return false;

		AGameModeBase* GameMode = _World->GetAuthGameMode<AGameModeBase>();
		if (nullptr == GameMode)
			return false;

		APawn* Player = GameMode->GetDefaultPawn();
		APlayerController* PlayerController = GameMode->GetPlayerController();
		if (nullptr == Player || nullptr == PlayerController)
			return false;

		UPlayerInput* PlayerInput = PlayerController->GetPlayerInput();
		if (nullptr == PlayerInput)
			return false;

		ResourceEngineSystem* Resources = GEngine->GetEngineSystem<ResourceEngineSystem>();
		if (nullptr == Resources)
			return false;

		const TCHAR* LevelName = _World->IsPersistentLevel(TEXT("Stage2"))
			? TEXT("Stage2") : TEXT("Stage1");
		FLevelAssetData LevelAsset;
		if (Resources->LoadLevelAsset(LevelName, LevelAsset) &&
			LevelAsset.bHasPlayerLocation)
		{
			Player->SetActorLocation(LevelAsset.PlayerLocation);
		}

		ProjectPlayerInput = PlayerInput;
		PlayerController->Possess(Player);
		LoadInputSettings(PlayerInput);
		PlayerInput->SetActionMapping({ "OpenNextLevel", EKey::F8 });
		StoreActionMapping("OpenNextLevel", EKey::F8);

		ConfigureMovementMappings(
			PlayerInput,
			nullptr != dynamic_cast<AProjectGameMode*>(GameMode));

		if (InputEngineSystem* Input = GEngine->GetEngineSystem<InputEngineSystem>())
			Input->SetPlayerController(PlayerController);

		return PlayerController->GetPawn() == Player;
	}

	bool SaveInputSettings()
	{
		std::error_code Error;
		std::filesystem::create_directories(InputSettingsPath.parent_path(), Error);
		if (Error)
			return false;

		std::ofstream Output(InputSettingsPath, std::ios::trunc);
		if (false == Output.is_open())
			return false;

		Output << "InputMappings 1\n";
		Output << std::setprecision(std::numeric_limits<float>::max_digits10);
		for (const FInputActionKeyMapping& Mapping : SavedActionMappings)
		{
			Output << "Action " << std::quoted(Mapping.__ActionName) << ' '
				<< GetKeyName(Mapping.__Key) << '\n';
		}

		for (const FInputAxisKeyMapping& Mapping : SavedAxisMappings)
		{
			Output << "Axis " << std::quoted(Mapping.__AxisName) << ' '
				<< GetKeyName(Mapping.__Key) << ' ' << Mapping.__Scale << '\n';
		}

		return Output.good();
	}

	void LoadInputSettings(UPlayerInput* _PlayerInput)
	{
		SavedActionMappings.clear();
		SavedAxisMappings.clear();

		std::ifstream Input(InputSettingsPath);
		if (false == Input.is_open())
			return;

		std::string Header;
		int Version = 0;
		if (false == static_cast<bool>(Input >> Header >> Version) ||
			"InputMappings" != Header || 1 != Version)
			return;

		std::string Type;
		while (Input >> Type)
		{
			std::string MappingName;
			std::string KeyName;
			if (false == static_cast<bool>(Input >> std::quoted(MappingName) >> KeyName))
				break;

			EKey Key = EKey::Invalid;
			if (false == TryParseKey(KeyName, Key) || EKey::Invalid == Key)
			{
				std::string IgnoredLine;
				std::getline(Input, IgnoredLine);
				continue;
			}

			if ("Action" == Type)
			{
				SavedActionMappings.push_back({ MappingName, Key });
                if (_PlayerInput)
                    _PlayerInput->SetActionMapping({ MappingName, Key });
			}
			else if ("Axis" == Type)
			{
				float Scale = 0.0f;
				if (Input >> Scale)
				{
					SavedAxisMappings.push_back({ MappingName, Key, Scale });
                    if (_PlayerInput)
                        _PlayerInput->SetAxisMapping({ MappingName, Key, Scale });
				}
			}
			else
			{
				std::string IgnoredLine;
				std::getline(Input, IgnoredLine);
			}
		}
	}

	void StoreActionMapping(const std::string& _MappingName, EKey _Key)
	{
		for (const FInputActionKeyMapping& Mapping : SavedActionMappings)
		{
			if (Mapping.__ActionName == _MappingName && Mapping.__Key == _Key)
				return;
		}

		SavedActionMappings.push_back({ _MappingName, _Key });
	}

	void StoreAxisMapping(const std::string& _MappingName, EKey _Key, float _Scale)
	{
		for (FInputAxisKeyMapping& Mapping : SavedAxisMappings)
		{
			if (Mapping.__AxisName == _MappingName && Mapping.__Key == _Key)
			{
				Mapping.__Scale = _Scale;
				return;
			}
		}

		SavedAxisMappings.push_back({ _MappingName, _Key, _Scale });
	}

	void ConfigureMovementMappings(UPlayerInput* _PlayerInput, bool _bProjectGameMode)
	{
		if (nullptr == _PlayerInput)
			return;

		for (const FInputAxisKeyMapping& Mapping : SavedAxisMappings)
		{
			if (Mapping.__AxisName == "MoveHorizontal" ||
				Mapping.__AxisName == "MoveVertical")
			{
				_PlayerInput->RemoveAxisMapping(Mapping);
			}
		}

		SavedAxisMappings.erase(
			std::remove_if(
				SavedAxisMappings.begin(),
				SavedAxisMappings.end(),
				[](const FInputAxisKeyMapping& _Mapping)
				{
					return _Mapping.__AxisName == "MoveHorizontal" ||
						_Mapping.__AxisName == "MoveVertical";
				}),
			SavedAxisMappings.end());

		const std::vector<FInputAxisKeyMapping> MovementMappings =
			_bProjectGameMode
			? std::vector<FInputAxisKeyMapping>
			{
				{ "MoveHorizontal", EKey::Left, -1.0f },
				{ "MoveHorizontal", EKey::Right, 1.0f },
				{ "MoveVertical", EKey::Up, -1.0f },
				{ "MoveVertical", EKey::Down, 1.0f }
			}
			: std::vector<FInputAxisKeyMapping>
			{
				{ "MoveHorizontal", EKey::A, -1.0f },
				{ "MoveHorizontal", EKey::D, 1.0f },
				{ "MoveVertical", EKey::W, -1.0f },
				{ "MoveVertical", EKey::S, 1.0f }
			};

		for (const FInputAxisKeyMapping& Mapping : MovementMappings)
		{
			_PlayerInput->SetAxisMapping(Mapping);
			StoreAxisMapping(
				Mapping.__AxisName,
				Mapping.__Key,
				Mapping.__Scale);
		}
	}
}

bool InitializeProject()
{
	return InitializeProjectWithGameMode(
		ReadGameModeSelection() == "EngineGameMode");
}

bool InitializeProjectWithGameMode(bool _UseEngineGameMode)
{
	UWorld* World = GEngine->GetWorld();
	if (nullptr == World)
		return false;

	UGameInstance* GameInstance = GEngine->GetGameInstance();
	if (nullptr == GameInstance)
		return false;

	UProjectGameInstanceSubsystem* ProjectState =
		GameInstance->CreateSubsystem<UProjectGameInstanceSubsystem>();
	if (nullptr == ProjectState)
		return false;

	GEngine->SetDefaultGameModeFactory([_UseEngineGameMode]
	{
		if (_UseEngineGameMode)
			return std::unique_ptr<AGameModeBase>(std::make_unique<AGameModeBase>());

		return std::unique_ptr<AGameModeBase>(
			std::make_unique<AProjectGameMode>());
	});
	GEngine->SetWorldInitializer(ConfigureProjectWorld);
	if (ResourceEngineSystem* Resources = GEngine->GetEngineSystem<ResourceEngineSystem>())
	{
		Resources->AddContentRoot(TEXT("Content"));
		Resources->AddContentRoot(TEXT("Game/Project/Content"));
		Resources->AddContentRoot(TEXT("../../Project/Content"));
	}

	if (nullptr == World->GetAuthGameMode<AGameModeBase>())
	{
		ProjectState->MarkProjectInitialized();
		return UGameplayStatics::OpenLevel(nullptr, TEXT("Stage1"));
	}

	InputEngineSystem* Input = GEngine->GetEngineSystem<InputEngineSystem>();
	if (nullptr == Input)
		return false;

	AGameModeBase* GameMode =
		World->GetAuthGameMode<AGameModeBase>();
	if (nullptr == GameMode)
		return false;
	APawn* Player = GameMode->GetDefaultPawn();
	APlayerController* PlayerController = GameMode->GetPlayerController();

	UPlayerInput* PlayerInput = PlayerController->GetPlayerInput();
	if (nullptr == PlayerInput)
		return false;
	ProjectPlayerInput = PlayerInput;






	PlayerController->Possess(Player);
	LoadInputSettings(PlayerInput);
	PlayerInput->SetActionMapping({ "OpenNextLevel", EKey::F8 });
	StoreActionMapping("OpenNextLevel", EKey::F8);
	ConfigureMovementMappings(
		PlayerInput,
		nullptr != dynamic_cast<AProjectGameMode*>(GameMode));
	Input->SetPlayerController(PlayerController);
	ProjectState->MarkProjectInitialized();
	return PlayerController->GetPawn() == Player;
}

bool StopProject()
{
	ProjectPlayerInput = nullptr;
	return nullptr != GEngine && GEngine->StopPlay();
}

bool SetProjectPaused(bool _bPaused)
{
	if (nullptr == GEngine || nullptr == GEngine->GetWorld())
		return false;

	GEngine->GetWorld()->SetDebugPauseExecution(_bPaused);
	return true;
}

void LoadProjectInputMappings()
{
	LoadInputSettings(nullptr);
}

bool SetProjectActionMapping(const char* _MappingName, EKey _Key)
{
    if (nullptr == _MappingName || '\0' == _MappingName[0])
        return false;

	if (ProjectPlayerInput)
        ProjectPlayerInput->SetActionMapping({ _MappingName, _Key });
	StoreActionMapping(_MappingName, _Key);
	return SaveInputSettings();
}

bool SetProjectAxisMapping(const char* _MappingName, EKey _Key, float _Scale)
{
    if (nullptr == _MappingName || '\0' == _MappingName[0])
        return false;

	if (ProjectPlayerInput)
        ProjectPlayerInput->SetAxisMapping({ _MappingName, _Key, _Scale });
	StoreAxisMapping(_MappingName, _Key, _Scale);
	return SaveInputSettings();
}

bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping)
{
	if (ProjectPlayerInput)
        ProjectPlayerInput->RemoveActionMapping(_Mapping);
	const auto Iter = std::find_if(SavedActionMappings.begin(), SavedActionMappings.end(),
		[&_Mapping](const FInputActionKeyMapping& _Saved)
		{
			return _Saved.__ActionName == _Mapping.__ActionName &&
				_Saved.__Key == _Mapping.__Key;
		});
	if (SavedActionMappings.end() == Iter)
		return false;

	SavedActionMappings.erase(Iter);
	return SaveInputSettings();
}

bool RemoveProjectAxisMapping(const FInputAxisKeyMapping& _Mapping)
{
	if (ProjectPlayerInput)
        ProjectPlayerInput->RemoveAxisMapping(_Mapping);
	const auto Iter = std::find_if(SavedAxisMappings.begin(), SavedAxisMappings.end(),
		[&_Mapping](const FInputAxisKeyMapping& _Saved)
		{
			return _Saved.__AxisName == _Mapping.__AxisName &&
				_Saved.__Key == _Mapping.__Key && _Saved.__Scale == _Mapping.__Scale;
		});
	if (SavedAxisMappings.end() == Iter)
		return false;

	SavedAxisMappings.erase(Iter);
	return SaveInputSettings();
}

std::vector<FInputActionKeyMapping> GetProjectActionMappings()
{
	return SavedActionMappings;
}

std::vector<FInputAxisKeyMapping> GetProjectAxisMappings()
{
	return SavedAxisMappings;
}
