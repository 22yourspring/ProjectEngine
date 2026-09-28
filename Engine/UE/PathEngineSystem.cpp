#include "pch.h"
#include "CollisionProfile.h"
#include "PathEngineSystem.h"
#include "UnrealString.h"
#include <array>
#include <fstream>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>
#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")

namespace
{
	std::filesystem::path NormalizePath(const std::filesystem::path& _Path)
	{
		if (_Path.empty())
			return {};

		return std::filesystem::absolute(_Path).lexically_normal();
	}

	FString ReadDescriptorValue(const FString& _Text, const FString& _Key)
	{
		const FString Token = FString(TEXT("\"")) + _Key + TEXT("\"");
		const int32 KeyPosition = _Text.Find(Token, ESearchCase::CaseSensitive);
		if (KeyPosition == INDEX_NONE)
			return {};

		const int32 ColonPosition = _Text.Find(TEXT(":"), ESearchCase::CaseSensitive,
			ESearchDir::FromStart, KeyPosition + Token.Len());
		if (ColonPosition == INDEX_NONE)
			return {};
		const int32 QuoteBegin = _Text.Find(TEXT("\""), ESearchCase::CaseSensitive,
			ESearchDir::FromStart, ColonPosition + 1);
		if (QuoteBegin == INDEX_NONE)
			return {};

		const int32 QuoteEnd = _Text.Find(TEXT("\""), ESearchCase::CaseSensitive,
			ESearchDir::FromStart, QuoteBegin + 1);
		if (QuoteEnd == INDEX_NONE)
			return {};

		return _Text.Mid(QuoteBegin + 1, QuoteEnd - QuoteBegin - 1);
	}
}



HRESULT PathEngineSystem::Initialize()
{
	if (!__ProjectDirectory.empty())
		return S_OK;

	try
	{
		int ArgumentCount = 0;
		LPWSTR* Arguments = CommandLineToArgvW(GetCommandLineW(), &ArgumentCount);
		if (nullptr == Arguments)
			return E_FAIL;
		const std::unique_ptr<wchar_t*, decltype(&LocalFree)> ArgumentOwner(Arguments, &LocalFree);
		std::filesystem::path ProjectFile;
		for (int Index = 1; Index < ArgumentCount; ++Index)
		{
			const std::filesystem::path Argument(Arguments[Index]);
			if (0 == _wcsicmp(Argument.extension().c_str(), TEXT(".uproject")))
			{
				ProjectFile = Argument;
				break;
			}
		}

		if (ProjectFile.empty())
		{
			std::array<TCHAR, 32768> ExecutableBuffer = {};
			const DWORD Length = GetModuleFileNameW(nullptr, ExecutableBuffer.data(),
				static_cast<DWORD>(ExecutableBuffer.size()));
			if (0 == Length || Length >= ExecutableBuffer.size())
				return E_FAIL;
			const FString Executable(ExecutableBuffer.data());
			ProjectFile = FindProjectFile(std::filesystem::path(*Executable).parent_path());
		}

		if (ProjectFile.empty())
			return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		return ConfigureFromProjectFile(ProjectFile);
	}
	catch (const std::filesystem::filesystem_error&)
	{
		return E_INVALIDARG;
	}
}

void PathEngineSystem::Deinitialize()
{
	__RootDirectory.clear();
	__EngineDirectory.clear();
	__ProjectDirectory.clear();
	__ProjectModuleName.clear();
	__ProjectContentDirectoryName = TEXT("Content");
}

HRESULT PathEngineSystem::Configure(const std::filesystem::path& _RootDirectory,
	const std::filesystem::path& _EngineDirectory,
	const std::filesystem::path& _ProjectDirectory)
{
	return ApplyConfiguration(
		_RootDirectory,
		_EngineDirectory,
		_ProjectDirectory,
		TEXT("Content"));
}

HRESULT PathEngineSystem::ApplyConfiguration(const std::filesystem::path& _RootDirectory,
	const std::filesystem::path& _EngineDirectory,
	const std::filesystem::path& _ProjectDirectory,
	const std::filesystem::path& _ProjectContentDirectoryName)
{
	if (_RootDirectory.empty() || _EngineDirectory.empty() || _ProjectDirectory.empty())
		return E_INVALIDARG;
	if (_ProjectContentDirectoryName.empty())
		return E_INVALIDARG;

	std::error_code Error;
	auto Root = std::filesystem::absolute(_RootDirectory, Error).lexically_normal();
	if (Error) return E_INVALIDARG;
	auto Engine = std::filesystem::absolute(_EngineDirectory, Error).lexically_normal();
	if (Error) return E_INVALIDARG;
	auto Project = std::filesystem::absolute(_ProjectDirectory, Error).lexically_normal();
	if (Error) return E_INVALIDARG;

	__RootDirectory = std::move(Root);
	__EngineDirectory = std::move(Engine);
	__ProjectDirectory = std::move(Project);
	__ProjectModuleName = __ProjectDirectory.filename();
	__ProjectContentDirectoryName = _ProjectContentDirectoryName;
	const auto CollisionConfig = __ProjectDirectory / TEXT("Config") / TEXT("CollisionProfiles.cfg");
	if (std::filesystem::exists(CollisionConfig) && !UCollisionProfile::Get()->LoadConfig(CollisionConfig)) return E_FAIL;
    if (!std::filesystem::exists(CollisionConfig)) UCollisionProfile::Get()->ResetToDefaults();
	return S_OK;
}

HRESULT PathEngineSystem::ConfigureFromProjectFile(const std::filesystem::path& _ProjectFile)
{
	std::ifstream Descriptor(_ProjectFile, std::ios::binary);
	if (!Descriptor)
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

	const std::vector<char> Utf8Bytes(
		(std::istreambuf_iterator<char>(Descriptor)),
		std::istreambuf_iterator<char>());
	if (Descriptor.bad() || Utf8Bytes.empty())
		return E_INVALIDARG;
	const FString Text = FString::FromUtf8(std::string_view(Utf8Bytes.data(), Utf8Bytes.size()));
	if (Text.IsEmpty())
		return E_INVALIDARG;

	std::error_code Error;
	const std::filesystem::path ProjectFile = std::filesystem::absolute(_ProjectFile, Error).lexically_normal();
	if (Error)
		return E_INVALIDARG;
	const std::filesystem::path ProjectDirectory = ProjectFile.parent_path();
	const FString EngineDirectoryValue = ReadDescriptorValue(Text, TEXT("EngineDirectory"));
	const FString RootDirectoryValue = ReadDescriptorValue(Text, TEXT("RootDirectory"));
	const FString ContentDirectoryValue = ReadDescriptorValue(Text, TEXT("ContentDirectory"));
	if (EngineDirectoryValue.IsEmpty())
		return E_INVALIDARG;

	const std::filesystem::path RootDirectory = RootDirectoryValue.IsEmpty()
		? ProjectDirectory.parent_path().parent_path()
		: ProjectDirectory / std::filesystem::path(*RootDirectoryValue);
	const std::filesystem::path ProjectContentDirectoryName = ContentDirectoryValue.IsEmpty()
		? std::filesystem::path(TEXT("Content"))
		: std::filesystem::path(*ContentDirectoryValue);
	const FString ModuleName = ReadDescriptorValue(Text, TEXT("ProjectName"));
	const std::filesystem::path ModulePath = ModuleName.IsEmpty()
		? ProjectFile.stem() : std::filesystem::path(*ModuleName);
	if (ModulePath.empty() || ModulePath.has_parent_path() || ModulePath.has_extension() ||
		ModulePath == TEXT(".") || ModulePath == TEXT(".."))
		return E_INVALIDARG;
	const HRESULT Result = ApplyConfiguration(
		RootDirectory,
		ProjectDirectory / std::filesystem::path(*EngineDirectoryValue),
		ProjectDirectory,
		ProjectContentDirectoryName);
	if (SUCCEEDED(Result))
	{
		__ProjectFile = ProjectFile;
		__ProjectModuleName = ModulePath;
	}
	return Result;
}

std::filesystem::path PathEngineSystem::FindProjectFile(const std::filesystem::path& _StartDirectory)
{
	std::filesystem::path Directory = NormalizePath(_StartDirectory);
	for (int Depth = 0; !Directory.empty() && Depth < 6; ++Depth)
	{
		std::error_code ErrorCode;
		for (const auto& Entry : std::filesystem::recursive_directory_iterator(
			Directory,
			std::filesystem::directory_options::skip_permission_denied,
			ErrorCode))
		{
			if (!ErrorCode && Entry.is_regular_file(ErrorCode) && Entry.path().extension() == TEXT(".uproject"))
				return Entry.path();
		}

		const std::filesystem::path Parent = Directory.parent_path();
		if (Parent == Directory)
			break;
		Directory = Parent;
	}
	return {};
}

std::filesystem::path PathEngineSystem::GetEngineContentDirectory() const
{
	return __EngineDirectory / TEXT("Content");
}

std::filesystem::path PathEngineSystem::GetProjectContentDirectory() const
{
	return __ProjectDirectory / __ProjectContentDirectoryName;
}

std::filesystem::path PathEngineSystem::GetProjectSavedDirectory() const
{
	return __ProjectDirectory / TEXT("Saved");
}

std::filesystem::path PathEngineSystem::GetProjectModuleFile(const TCHAR* _TargetName) const
{
#ifdef _DEBUG
	const TCHAR* Configuration = TEXT("Debug");
#else
	const TCHAR* Configuration = TEXT("Release");
#endif
	if (!_TargetName || (FString(_TargetName) != TEXT("Editor") &&
		FString(_TargetName) != TEXT("Client")))
		return {};
	std::filesystem::path ModuleName(_TargetName);
	ModuleName += TEXT("-");
	ModuleName += __ProjectModuleName;
	ModuleName += TEXT(".dll");
	return __ProjectDirectory / TEXT("Binaries") / TEXT("Win64") / Configuration / ModuleName;
}
