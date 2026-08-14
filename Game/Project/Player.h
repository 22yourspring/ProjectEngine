#pragma once

#include "UE/Character.h"
#include "UE/Math/Vector.h"

class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
class UInputComponent;

UCLASS()
class PROJECT_API APlayer final : public ACharacter
{
	GENERATED_BODY()

public:
	APlayer();
	virtual ~APlayer() override;

protected:
	virtual void Tick(float _DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* _InputComponent) override;

private:
	UFUNCTION()
	void MoveHorizontal(float _Value);
	UFUNCTION()
	void MoveVertical(float _Value);

private:
	UPROPERTY()
	std::unique_ptr<UStaticMesh>	__PlayerMesh;
	UPROPERTY()
	USceneComponent*				__RootSceneComponent = nullptr;
	UPROPERTY()
	UStaticMeshComponent*			__MeshComponent = nullptr;
	UPROPERTY()
	float							__MoveSpeed = 300.0f;
	float							__HorizontalInput = 0.0f;
	float							__VerticalInput = 0.0f;
};
