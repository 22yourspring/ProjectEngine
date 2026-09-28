


#include "pch.h"
#include "UE/GameMapsSettings.h"
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
#include "UE/PlayerStart.h"
#include "UE/PlayerInput.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/GameplayStatics.h"
#include "UE/PathEngineSystem.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <cmath>
#include <vector>

namespace
{
	UPlayerInput* ProjectPlayerInput = nullptr;
	std::vector<FInputActionKeyMapping> SavedActionMappings;
	std::vector<FInputAxisKeyMapping> SavedAxisMappings;

	const std::filesystem::path ProjectSavedDirectory =
		GEngine->GetEngineSystem<PathEngineSystem>()->GetProjectSavedDirectory();
	const std::filesystem::path InputSettingsPath =
        GEngine->GetEngineSystem<PathEngineSystem>()->GetProjectDirectory() / TEXT("Config/DefaultInput.ini");
	const std::filesystem::path GameModeSettingsPath =
		ProjectSavedDirectory / TEXT("Config") / TEXT("EditorDefaultPawn.cfg");

	FString ReadGameModeSelection()
	{
		std::ifstream Input(GameModeSettingsPath);
		std::string Selection;
		if (Input.is_open())
			Input >> Selection;
		return Selection;
	}
	void LoadInputSettings(UPlayerInput* _PlayerInput);
	void StoreActionMapping(const FString& _MappingName, EKey _Key);
	void StoreAxisMapping(const FString& _MappingName, EKey _Key, float _Scale);
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

        const FString LevelName = _World->GetPersistentLevel()->GetMapName();
        const auto& Actors = _World->GetPersistentLevel()->GetActors();
        const bool HasStart = std::any_of(Actors.begin(), Actors.end(), [](const auto& _Actor)
        { return dynamic_cast<APlayerStart*>(_Actor.get()) && !_Actor->IsPendingDestroy(); });
		FLevelAssetData LevelAsset;
		if (!HasStart && Resources->LoadLevelAsset(*LevelName, LevelAsset) &&
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

        std::string Preserved, InputOther, Line; bool InputSection = false;
        std::ifstream Existing(InputSettingsPath);
        while (std::getline(Existing, Line))
        {
            if (!Line.empty() && Line.back() == '\r') Line.pop_back();
            if (!Line.empty() && Line.front() == '[') InputSection = Line == "[/Script/Engine.InputSettings]";
            if (Line == "[/Script/Engine.InputSettings]") continue;
            if (InputSection && (Line.rfind("+ActionMappings=", 0) == 0 || Line.rfind("+AxisMappings=", 0) == 0)) continue;
            if (InputSection) { InputOther += Line + '\n'; continue; }
            Preserved += Line + '\n';
        }
        Existing.close();
        const auto Temporary = std::filesystem::path(InputSettingsPath.wstring() + L".tmp");
		std::ofstream Output(Temporary, std::ios::trunc);
		if (false == Output.is_open())
			return false;

        Output << Preserved << "\n[/Script/Engine.InputSettings]\n" << InputOther;
		Output << std::setprecision(std::numeric_limits<float>::max_digits10);
		for (const FInputActionKeyMapping& Mapping : SavedActionMappings)
		{
            Output << "+ActionMappings=(ActionName=" << std::quoted(Mapping.__ActionName.ToUtf8()) << ",Key=" << GetKeyName(Mapping.__Key).ToUtf8() << ")\n";
		}

		for (const FInputAxisKeyMapping& Mapping : SavedAxisMappings)
		{
            Output << "+AxisMappings=(AxisName=" << std::quoted(Mapping.__AxisName.ToUtf8()) << ",Key=" << GetKeyName(Mapping.__Key).ToUtf8() << ",Scale=" << Mapping.__Scale << ")\n";
		}

        Output.flush(); const bool Valid = Output.good(); Output.close();
        if (Valid && MoveFileExW(Temporary.c_str(), InputSettingsPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
        std::filesystem::remove(Temporary, Error); return false;
	}

	void LoadInputSettings(UPlayerInput* _PlayerInput)
	{
		SavedActionMappings.clear();
		SavedAxisMappings.clear();

		std::ifstream Input(InputSettingsPath);
        if (Input.is_open())
        {
            const std::regex Action(R"re(^\+ActionMappings=\(ActionName=("(?:\\.|[^"])*"),Key=([A-Za-z0-9_]+)\)$)re");
            const std::regex Axis(R"re(^\+AxisMappings=\(AxisName=("(?:\\.|[^"])*"),Key=([A-Za-z0-9_]+),Scale=([-+0-9.eE]+)\)$)re");
            std::string Line; bool InputSection = false;
            while (std::getline(Input, Line))
            {
                if (!Line.empty() && Line.back() == '\r') Line.pop_back();
                if (!Line.empty() && Line.front() == '[') InputSection = Line == "[/Script/Engine.InputSettings]";
                if (!InputSection) continue;
                std::smatch Match; const bool IsAction = std::regex_match(Line, Match, Action);
                if (!IsAction && !std::regex_match(Line, Match, Axis)) continue;
                std::string Name; std::istringstream NameStream(Match[1].str()); NameStream >> std::quoted(Name);
                EKey Key; if (!TryParseKey(Match[2].str(), Key)) continue;
                if (IsAction) { SavedActionMappings.push_back({Name, Key}); if (_PlayerInput) _PlayerInput->SetActionMapping({Name, Key}); }
                else
                {
                    float Scale = 0; std::istringstream ScaleStream(Match[3].str());
                    if (!(ScaleStream >> Scale) || !std::isfinite(Scale)) continue;
                    SavedAxisMappings.push_back({Name, Key, Scale}); if (_PlayerInput) _PlayerInput->SetAxisMapping({Name, Key, Scale});
                }
            }
            return;
        }
        Input.open(ProjectSavedDirectory / TEXT("Config/InputMappings.cfg"));
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

	void StoreActionMapping(const FString& _MappingName, EKey _Key)
	{
		for (const FInputActionKeyMapping& Mapping : SavedActionMappings)
		{
			if (Mapping.__ActionName == _MappingName && Mapping.__Key == _Key)
				return;
		}

		SavedActionMappings.push_back({ _MappingName, _Key });
	}

	void StoreAxisMapping(const FString& _MappingName, EKey _Key, float _Scale)
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
    UGameMapsSettings Settings;
    Settings.Load(GEngine->GetEngineSystem<PathEngineSystem>()->GetProjectDirectory());
    return InitializeProjectWithGameMode(Settings.__UseEngineGameMode);
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

	if (nullptr == World->GetAuthGameMode<AGameModeBase>())
	{
		ProjectState->MarkProjectInitialized();
        UGameMapsSettings Settings;
        Settings.Load(GEngine->GetEngineSystem<PathEngineSystem>()->GetProjectDirectory());
        return UGameplayStatics::OpenLevel(nullptr, Settings.__GameDefaultMap.IsEmpty() ? TEXT("Stage1") : *Settings.__GameDefaultMap);
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

bool SetProjectActionMapping(const FString& _MappingName, EKey _Key)
{
    if (_MappingName.IsEmpty())
        return false;

	if (ProjectPlayerInput)
        ProjectPlayerInput->SetActionMapping({ _MappingName, _Key });
	StoreActionMapping(_MappingName, _Key);
	return SaveInputSettings();
}

bool SetProjectAxisMapping(const FString& _MappingName, EKey _Key, float _Scale)
{
    if (_MappingName.IsEmpty())
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
