#pragma once

template <typename UserClass>
FInputActionBinding& UInputComponent::BindAction(
	const std::string& _ActionName, EInputEvent _KeyEvent,
	UserClass* _Object, void(UserClass::* _Function)())
{
	FInputActionBinding Binding;
	Binding.__ActionName = _ActionName;
	Binding.__KeyEvent = _KeyEvent;
	Binding.__ActionDelegate.BindRaw(_Object, _Function);
	__ActionBindings.emplace_back(std::move(Binding));
	return __ActionBindings.back();
}

template <typename UserClass>
FInputAxisBinding& UInputComponent::BindAxis(
	const std::string& _AxisName,
	UserClass* _Object, void(UserClass::* _Function)(float))
{
	FInputAxisBinding Binding;
	Binding.__AxisName = _AxisName;
	Binding.__AxisDelegate.BindRaw(_Object, _Function);
	__AxisBindings.emplace_back(std::move(Binding));
	return __AxisBindings.back();
}
