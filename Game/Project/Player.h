#pragma once

#include "UE/Character.h"
#include "UE/Math/Vector.h"

class UStaticMesh;
class UStaticMeshComponent;
class UBoxComponent;
class UInputComponent;

UCLASS()
class APlayer final : public ACharacter
{
	GENERATED_BODY()

public:
	APlayer();
	virtual ~APlayer() override;
    void Serialize(FArchive& _Archive) override;

protected:
	virtual void Tick(float _DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* _InputComponent) override;

private:
	UFUNCTION()
	void MoveHorizontal(float _Value);

	UFUNCTION()
	void MoveVertical(float _Value);

	UFUNCTION()
	void OpenNextLevel();

private:
	UPROPERTY()
	std::unique_ptr<UStaticMesh>	__PlayerMesh;

	UPROPERTY()
	UBoxComponent*				__CollisionComponent = nullptr;

	UPROPERTY()
	UStaticMeshComponent*			__MeshComponent = nullptr;

	UPROPERTY()
	float							__MoveSpeed = 300.0f;
	float							__HorizontalInput = 0.0f;
	float							__VerticalInput = 0.0f;
};
