// Project.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "Project.h"
#include "Player.h"
#include "UE/Engine.h"
#include "UE/World.h"
#include "UE/InputSubsystem.h"
#include "UE/PlayerController.h"
#include "UE/PlayerInput.h"

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
				_PlayerInput->SetActionMapping({ MappingName, Key });
			}
			else if ("Axis" == Type)
			{
				float Scale = 0.0f;
				if (Input >> Scale)
				{
					SavedAxisMappings.push_back({ MappingName, Key, Scale });
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
}

bool InitializeProject()
{
	UWorld* World = GEngine->GetWorld();
	if (nullptr == World)
		return false;

	InputSubsystem* Input = GEngine->GetSubsystem<InputSubsystem>();
	if (nullptr == Input)
		return false;

	APlayer* Player = World->SpawnActor<APlayer>();
	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	if (nullptr == Player || nullptr == PlayerController)
		return false;

	UPlayerInput* PlayerInput = PlayerController->GetPlayerInput();
	if (nullptr == PlayerInput)
		return false;
	ProjectPlayerInput = PlayerInput;






	PlayerController->Possess(Player);
	LoadInputSettings(PlayerInput);
	Input->SetPlayerController(PlayerController);
	return PlayerController->GetPawn() == Player;
}

bool SetProjectActionMapping(const char* _MappingName, EKey _Key)
{
	if (nullptr == ProjectPlayerInput || nullptr == _MappingName || '\0' == _MappingName[0])
		return false;

	ProjectPlayerInput->SetActionMapping({ _MappingName, _Key });
	StoreActionMapping(_MappingName, _Key);
	return SaveInputSettings();
}

bool SetProjectAxisMapping(const char* _MappingName, EKey _Key, float _Scale)
{
	if (nullptr == ProjectPlayerInput || nullptr == _MappingName || '\0' == _MappingName[0])
		return false;

	ProjectPlayerInput->SetAxisMapping({ _MappingName, _Key, _Scale });
	StoreAxisMapping(_MappingName, _Key, _Scale);
	return SaveInputSettings();
}

bool RemoveProjectActionMapping(const FInputActionKeyMapping& _Mapping)
{
	if (nullptr == ProjectPlayerInput)
		return false;

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
	if (nullptr == ProjectPlayerInput)
		return false;

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
