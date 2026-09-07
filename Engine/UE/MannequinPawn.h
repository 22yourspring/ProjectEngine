#pragma once

#include "Character.h"

class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
class UInputComponent;


UCLASS(BlueprintType, Blueprintable, MinimalAPI)
class ENGINE_API AMannequinPawn final : public ACharacter
{
	GENERATED_BODY()

public:
	AMannequinPawn();
	~AMannequinPawn() override;

protected:
	void Tick(float _DeltaTime) override;
	void SetupPlayerInputComponent(UInputComponent* _InputComponent) override;

private:
	void MoveHorizontal(float _Value);
	void MoveVertical(float _Value);
	void OpenNextLevel();

private:
	std::unique_ptr<UStaticMesh> __Mesh;
	USceneComponent* __RootSceneComponent = nullptr;
	UStaticMeshComponent* __MeshComponent = nullptr;
	float __MoveSpeed = 300.0f;
	float __HorizontalInput = 0.0f;
	float __VerticalInput = 0.0f;
};
