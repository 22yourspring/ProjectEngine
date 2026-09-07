#pragma once

#include "Object.h"

class UGameInstance;

UCLASS(Abstract, MinimalAPI)
class ENGINE_API UGameInstanceSubsystem : public UObject
{
	GENERATED_BODY()

	friend class UGameInstance;

public:
	virtual ~UGameInstanceSubsystem() override = default;

	virtual HRESULT Initialize();
	virtual void Deinitialize();

	UGameInstance* GetGameInstance() const { return __GameInstance; }

private:
	UGameInstance* __GameInstance = nullptr;
};
