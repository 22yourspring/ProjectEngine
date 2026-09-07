#include "pch.h"
#include "ResourceEngineSystem.h"

#include <fstream>
#include <string>

HRESULT ResourceEngineSystem::Initialize()
{
	return S_OK;
}

void ResourceEngineSystem::Deinitialize()
{
	std::lock_guard<std::mutex> Lock(__ContentRootMutex);
	__ContentRoots.clear();
}

void ResourceEngineSystem::AddContentRoot(const TCHAR* _ContentRoot)
{
	if (nullptr == _ContentRoot || L'\0' == _ContentRoot[0])
		return;

	const std::filesystem::path Root(_ContentRoot);
	std::lock_guard<std::mutex> Lock(__ContentRootMutex);
	for (const std::filesystem::path& ExistingRoot : __ContentRoots)
	{
		if (ExistingRoot == Root)
			return;
	}
	__ContentRoots.push_back(Root);
}

bool ResourceEngineSystem::LoadLevelAsset(
	const TCHAR* _LevelName, FLevelAssetData& _OutData) const
{
	if (nullptr == _LevelName || L'\0' == _LevelName[0])
		return false;

	std::vector<std::filesystem::path> ContentRoots;
	{
		std::lock_guard<std::mutex> Lock(__ContentRootMutex);
		ContentRoots = __ContentRoots;
	}

	std::filesystem::path MapPath;
	for (const std::filesystem::path& Root : ContentRoots)
	{
		const std::filesystem::path Candidate =
			Root / "Maps" / (std::wstring(_LevelName) + L".umap");
		if (std::filesystem::exists(Candidate))
		{
			MapPath = Candidate;
			break;
		}
	}

	if (MapPath.empty())
		return false;

	std::ifstream Input(MapPath);
	if (false == Input.is_open())
		return false;

	_OutData = {};
	std::string Command;
	while (Input >> Command)
	{
		if ("PlayerLocation" == Command)
		{
			if (Input >> _OutData.PlayerLocation.X >>
				_OutData.PlayerLocation.Y >> _OutData.PlayerLocation.Z)
			{
				_OutData.bHasPlayerLocation = true;
			}
		}
		else
		{
			std::string IgnoredLine;
			std::getline(Input, IgnoredLine);
		}
	}

	return true;
}
