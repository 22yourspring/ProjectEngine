#pragma once

#include "ActorComponent.h"
#include "DelegatesMacros.h"
#include "InputTypes.h"

#include <string>
#include <vector>

DECLARE_DELEGATE(FInputActionHandlerSignature);
DECLARE_DELEGATE_OneParam(FInputAxisHandlerSignature, float);


USTRUCT()
struct FInputActionBinding
{
	GENERATED_BODY()

	UPROPERTY()
	std::string __ActionName;
	UPROPERTY()
	EInputEvent __KeyEvent = EInputEvent::Pressed;
	FInputActionHandlerSignature __ActionDelegate;
	bool __bConsumeInput = true;
};


USTRUCT()
struct FInputAxisBinding
{
	GENERATED_BODY()

	UPROPERTY()
	std::string __AxisName;
	FInputAxisHandlerSignature __AxisDelegate;
	bool __bConsumeInput = false;
};


UCLASS(Transient, MinimalAPI)
class UInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInputComponent();
	virtual ~UInputComponent() override;

	template <typename UserClass>
	FInputActionBinding& BindAction(const std::string& _ActionName, EInputEvent _KeyEvent,
		UserClass* _Object, void(UserClass::* _Function)());

	template <typename UserClass>
	FInputAxisBinding& BindAxis(const std::string& _AxisName,
		UserClass* _Object, void(UserClass::* _Function)(float));

	bool ProcessAction(const std::string& _ActionName, EInputEvent _KeyEvent);
	bool ProcessAxis(const std::string& _AxisName, float _Value);

	int32_t __Priority = 0;
	bool __bBlockInput = false;

private:
	std::vector<FInputActionBinding> __ActionBindings;
	std::vector<FInputAxisBinding> __AxisBindings;
};

#include "InputComponent.inl"
