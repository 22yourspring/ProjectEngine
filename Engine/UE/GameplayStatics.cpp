#include "pch.h"
#include "GameplayStatics.h"
#include "Engine.h"

bool UGameplayStatics::OpenLevel(
	UObject* _WorldContextObject,
	const FName& _LevelName)
{
	UNREFERENCED_PARAMETER(_WorldContextObject);

	if (nullptr == GEngine)
		return false;

	return GEngine->LoadMap(_LevelName);
}

bool UGameplayStatics::OpenLevel(
	UObject* _WorldContextObject,
	const TCHAR* _LevelName)
{
	if (nullptr == _LevelName)
		return false;

	return OpenLevel(_WorldContextObject, FName(_LevelName));
}
