#pragma once

#include "NameTypes.h"
#include "Object.h"

class UWorld;


UCLASS(Abstract, MinimalAPI)
class ENGINE_API UGameplayStatics : public UObject
{
	GENERATED_BODY()

public:
	static bool OpenLevel(UObject* _WorldContextObject, const FName& _LevelName);
	static bool OpenLevel(UObject* _WorldContextObject, const TCHAR* _LevelName);
};
