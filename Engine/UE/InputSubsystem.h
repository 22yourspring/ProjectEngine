#pragma once

#include "InputTypes.h"
#include "Subsystem.h"

#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

class APlayerController;
class FWindowsInputApplication;


class InputSubsystem final : public ISubsystem
{
public:
	InputSubsystem();
	~InputSubsystem() override;

	virtual HRESULT Initialize() override;
	virtual void Deinitialize() override;
	virtual void Tick(float _DeltaTime) override;
	virtual bool IsTickable() const override { return false; }

	bool InitializePlatformApplication(HWND _WindowHandle);
	void TickInput(float _DeltaTime);

	void SetPlayerController(APlayerController* _PlayerController);
	void ClearPlayerController(APlayerController* _PlayerController);
	static EKey GetKeyFromVirtualKey(uint32_t _VirtualKey);

private:
	std::mutex __PlayerControllerMutex;
	APlayerController* __PlayerController = nullptr;
	std::unordered_map<EKey, bool> __KeyStates;
	std::unique_ptr<FWindowsInputApplication> __PlatformApplication;
};
