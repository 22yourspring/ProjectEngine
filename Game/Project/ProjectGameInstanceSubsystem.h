#pragma once

#include "UE/GameInstanceSubsystem.h"

UCLASS(MinimalAPI)
class UProjectGameInstanceSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	HRESULT Initialize() override;
	void Deinitialize() override;

	void MarkProjectInitialized() noexcept { __bProjectInitialized = true; }
	bool IsProjectInitialized() const noexcept { return __bProjectInitialized; }

private:
	bool __bProjectInitialized = false;
};
