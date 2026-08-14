#include "pch.h"
#include "InputComponent.h"

UInputComponent::UInputComponent()
{
}

UInputComponent::~UInputComponent() = default;

bool UInputComponent::ProcessAction(const std::string& _ActionName, EInputEvent _KeyEvent)
{
	bool bConsumed = false;

	for (FInputActionBinding& Binding : __ActionBindings)
	{
		if (Binding.__ActionName != _ActionName || Binding.__KeyEvent != _KeyEvent)
			continue;

		Binding.__ActionDelegate.ExecuteIfBound();
		bConsumed = bConsumed || Binding.__bConsumeInput;
	}

	return bConsumed;
}

bool UInputComponent::ProcessAxis(const std::string& _AxisName, float _Value)
{
	bool bConsumed = false;

	for (FInputAxisBinding& Binding : __AxisBindings)
	{
		if (Binding.__AxisName == _AxisName)
		{
			Binding.__AxisDelegate.ExecuteIfBound(_Value);
			bConsumed = bConsumed || Binding.__bConsumeInput;
		}
	}

	return bConsumed;
}
