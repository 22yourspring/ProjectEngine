#pragma once

#include "Controller.h"
#include "InputTypes.h"

#include <memory>
#include <vector>

class InputSubsystem;
class UInputComponent;
class UPlayerInput;


UCLASS(MinimalAPI)
class APlayerController : public AController
{
	GENERATED_BODY()

	friend class InputSubsystem;

public:
	APlayerController();
	virtual ~APlayerController() override;

	void PushInputComponent(UInputComponent* _InputComponent);
	bool PopInputComponent(UInputComponent* _InputComponent);

	UPlayerInput* GetPlayerInput() const { return __PlayerInput.get(); }
	UInputComponent* GetInputComponent() const { return __InputComponent; }

protected:
	virtual void SetupInputComponent();
	virtual void BuildInputStack(std::vector<UInputComponent*>& _InputStack);
	virtual void TickPlayerInput(const std::vector<FKeyInputEvent>& _KeyEvents,
		const std::vector<FAnalogInputEvent>& _AnalogEvents,
		float _DeltaTime, bool _bGamePaused);

private:
	std::unique_ptr<UPlayerInput> __PlayerInput;
	UInputComponent* __InputComponent = nullptr;
	std::vector<UInputComponent*> __PushedInputComponents;
};
