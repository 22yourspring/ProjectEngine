#include "pch.h"
#include "PlayerInput.h"

#include "InputComponent.h"

#include <algorithm>

void UPlayerInput::AddActionMapping(const FInputActionKeyMapping& _KeyMapping)
{
	std::lock_guard<std::mutex> Lock(__MappingMutex);
	__ActionMappings.push_back(_KeyMapping);
}

void UPlayerInput::AddAxisMapping(const FInputAxisKeyMapping& _KeyMapping)
{
	std::lock_guard<std::mutex> Lock(__MappingMutex);
	__AxisMappings.push_back(_KeyMapping);
}

void UPlayerInput::SetActionMapping(const FInputActionKeyMapping& _KeyMapping)
{
	std::lock_guard<std::mutex> Lock(__MappingMutex);
	for (FInputActionKeyMapping& Mapping : __ActionMappings)
	{
		if (Mapping.__ActionName == _KeyMapping.__ActionName && Mapping.__Key == _KeyMapping.__Key)
			return;
	}

	__ActionMappings.push_back(_KeyMapping);
}

void UPlayerInput::SetAxisMapping(const FInputAxisKeyMapping& _KeyMapping)
{
	std::lock_guard<std::mutex> Lock(__MappingMutex);
	for (FInputAxisKeyMapping& Mapping : __AxisMappings)
	{
		if (Mapping.__AxisName == _KeyMapping.__AxisName && Mapping.__Key == _KeyMapping.__Key)
		{
			Mapping.__Scale = _KeyMapping.__Scale;
			return;
		}
	}

	__AxisMappings.push_back(_KeyMapping);
}

bool UPlayerInput::RemoveActionMapping(const FInputActionKeyMapping& _KeyMapping)
{
	std::lock_guard<std::mutex> Lock(__MappingMutex);
	const auto Iter = std::find_if(__ActionMappings.begin(), __ActionMappings.end(),
		[&_KeyMapping](const FInputActionKeyMapping& _Mapping)
		{
			return _Mapping.__ActionName == _KeyMapping.__ActionName &&
				_Mapping.__Key == _KeyMapping.__Key;
		});
	if (__ActionMappings.end() == Iter)
		return false;

	__ActionMappings.erase(Iter);
	return true;
}

bool UPlayerInput::RemoveAxisMapping(const FInputAxisKeyMapping& _KeyMapping)
{
	std::lock_guard<std::mutex> Lock(__MappingMutex);
	const auto Iter = std::find_if(__AxisMappings.begin(), __AxisMappings.end(),
		[&_KeyMapping](const FInputAxisKeyMapping& _Mapping)
		{
			return _Mapping.__AxisName == _KeyMapping.__AxisName &&
				_Mapping.__Key == _KeyMapping.__Key &&
				_Mapping.__Scale == _KeyMapping.__Scale;
		});
	if (__AxisMappings.end() == Iter)
		return false;

	__AxisMappings.erase(Iter);
	return true;
}

void UPlayerInput::FlushPressedKeys()
{
	for (auto& KeyState : __KeyStateMap)
		KeyState.second = false;
}

void UPlayerInput::ProcessInputStack(const std::vector<UInputComponent*>& _InputComponentStack,
	const std::vector<FKeyInputEvent>& _KeyEvents,
	const std::vector<FAnalogInputEvent>& _AnalogEvents, float _DeltaTime, bool _bGamePaused)
{
	EvaluateInputDelegates(_InputComponentStack, _KeyEvents, _AnalogEvents, _DeltaTime, _bGamePaused);
}

void UPlayerInput::EvaluateInputDelegates(const std::vector<UInputComponent*>& _InputComponentStack,
	const std::vector<FKeyInputEvent>& _KeyEvents,
	const std::vector<FAnalogInputEvent>& _AnalogEvents, float _DeltaTime, bool _bGamePaused)
{
	UNREFERENCED_PARAMETER(_DeltaTime);
	UNREFERENCED_PARAMETER(_bGamePaused);

	std::vector<FInputActionKeyMapping> ActionMappings;
	std::vector<FInputAxisKeyMapping> AxisMappings;
	{
		std::lock_guard<std::mutex> Lock(__MappingMutex);
		ActionMappings = __ActionMappings;
		AxisMappings = __AxisMappings;
	}

	for (const FKeyInputEvent& KeyEvent : _KeyEvents)
	{
		__KeyStateMap[KeyEvent.__Key] = EInputEvent::Released != KeyEvent.__Event;

		for (const FInputActionKeyMapping& Mapping : ActionMappings)
		{
			if (Mapping.__Key != KeyEvent.__Key)
				continue;

			for (auto Iter = _InputComponentStack.rbegin(); Iter != _InputComponentStack.rend(); ++Iter)
			{
				UInputComponent* InputComponent = *Iter;
				if (nullptr == InputComponent)
					continue;

				if (InputComponent->ProcessAction(Mapping.__ActionName, KeyEvent.__Event))
					break;
				if (InputComponent->__bBlockInput)
					break;
			}
		}
	}

	std::unordered_map<std::string, float> AxisValues;
	for (const FInputAxisKeyMapping& Mapping : AxisMappings)
	{
		if (__KeyStateMap[Mapping.__Key])
			AxisValues[Mapping.__AxisName] += Mapping.__Scale;
		else
			AxisValues.try_emplace(Mapping.__AxisName, 0.0f);
	}

	for (const FAnalogInputEvent& AnalogEvent : _AnalogEvents)
	{
		for (const FInputAxisKeyMapping& Mapping : AxisMappings)
		{
			if (Mapping.__Key == AnalogEvent.__Key)
				AxisValues[Mapping.__AxisName] += AnalogEvent.__Value * Mapping.__Scale;
		}
	}

	for (const auto& AxisValue : AxisValues)
	{
		for (auto Iter = _InputComponentStack.rbegin(); Iter != _InputComponentStack.rend(); ++Iter)
		{
			UInputComponent* InputComponent = *Iter;
			if (nullptr == InputComponent)
				continue;

			if (InputComponent->ProcessAxis(AxisValue.first, AxisValue.second))
				break;
			if (InputComponent->__bBlockInput)
				break;
		}
	}
}
