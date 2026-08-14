#pragma once

#include "InputTypes.h"
#include "Object.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class UInputComponent;


UCLASS(Transient, MinimalAPI)
class UPlayerInput : public UObject
{
	GENERATED_BODY()

public:
	void AddActionMapping(const FInputActionKeyMapping& _KeyMapping);
	void AddAxisMapping(const FInputAxisKeyMapping& _KeyMapping);
	void SetActionMapping(const FInputActionKeyMapping& _KeyMapping);
	void SetAxisMapping(const FInputAxisKeyMapping& _KeyMapping);
	bool RemoveActionMapping(const FInputActionKeyMapping& _KeyMapping);
	bool RemoveAxisMapping(const FInputAxisKeyMapping& _KeyMapping);
	void FlushPressedKeys();

	void ProcessInputStack(const std::vector<UInputComponent*>& _InputComponentStack,
		const std::vector<FKeyInputEvent>& _KeyEvents,
		const std::vector<FAnalogInputEvent>& _AnalogEvents, float _DeltaTime, bool _bGamePaused);

protected:
	virtual void EvaluateInputDelegates(const std::vector<UInputComponent*>& _InputComponentStack,
		const std::vector<FKeyInputEvent>& _KeyEvents,
		const std::vector<FAnalogInputEvent>& _AnalogEvents, float _DeltaTime, bool _bGamePaused);

private:
	std::mutex __MappingMutex;
	std::vector<FInputActionKeyMapping> __ActionMappings;
	std::vector<FInputAxisKeyMapping> __AxisMappings;
	std::unordered_map<EKey, bool> __KeyStateMap;
};
