#include "pch.h"
#include "Archive.h"
#include <cmath>
#include "PrimitiveComponent.h"
#include "MannequinPawn.h"
#include "SceneComponent.h"
#include "StaticMesh.h"
#include "StaticMeshComponent.h"
#include "InputComponent.h"
#include "GameplayStatics.h"
#include "World.h"
#include "Level.h"

void AMannequinPawn::Serialize(FArchive& _Archive)
{
    ACharacter::Serialize(_Archive);
    _Archive.Serialize(&__MoveSpeed, sizeof(__MoveSpeed));
    if (!std::isfinite(__MoveSpeed) || __MoveSpeed < 0) _Archive.SetError();
}

AMannequinPawn::AMannequinPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	__Mesh = std::make_unique<UStaticMesh>();
	__Mesh->SetSize(80, 120);
	__Mesh->SetColor({ 70, 140, 230, 255 });

	__RootSceneComponent = CreateDefaultSubobject<USceneComponent>();
	SetRootComponent(__RootSceneComponent);

	__MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>();
	__MeshComponent->SetStaticMesh(__Mesh.get());
	__MeshComponent->SetupAttachment(__RootSceneComponent);
	SetActorLocation({ 100.0f, 100.0f, 0.0f });
}

AMannequinPawn::~AMannequinPawn() = default;

void AMannequinPawn::Tick(float _DeltaTime)
{
	Super::Tick(_DeltaTime);
    if (auto* Body = dynamic_cast<UPrimitiveComponent*>(GetRootComponent()); Body && Body->IsSimulatingPhysics())
    {
        FVector Input(__HorizontalInput, __VerticalInput, 0);
        if (Input.SizeSquared() > 1) Input.Normalize();
        Body->SetPhysicsLinearVelocity(Input * __MoveSpeed);
        return;
    }

	const float MoveDistance = __MoveSpeed * _DeltaTime;
	FVector NewLocation = GetActorLocation();
	NewLocation.X += __HorizontalInput * MoveDistance;
	NewLocation.Y += __VerticalInput * MoveDistance;
	SetActorLocation(NewLocation);
}

void AMannequinPawn::SetupPlayerInputComponent(UInputComponent* _InputComponent)
{
	if (nullptr == _InputComponent)
		return;

	_InputComponent->BindAxis("MoveHorizontal", this,
		&AMannequinPawn::MoveHorizontal);
	_InputComponent->BindAxis("MoveVertical", this,
		&AMannequinPawn::MoveVertical);
	_InputComponent->BindAction("OpenNextLevel", EInputEvent::Pressed,
		this, &AMannequinPawn::OpenNextLevel);
}

void AMannequinPawn::OpenNextLevel()
{
	UWorld* World = GetWorld();
	if (nullptr == World || nullptr == World->GetPersistentLevel())
		return;

	UGameplayStatics::OpenLevel(this,
		World->IsPersistentLevel(TEXT("Stage1")) ? TEXT("Stage2") : TEXT("Stage1"));
}

void AMannequinPawn::MoveHorizontal(float _Value)
{
	__HorizontalInput = _Value;
}

void AMannequinPawn::MoveVertical(float _Value)
{
	__VerticalInput = _Value;
}
