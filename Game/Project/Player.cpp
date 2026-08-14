#include "pch.h"
#include "Player.h"

#include "UE/StaticMesh.h"
#include "UE/StaticMeshComponent.h"
#include "UE/SceneComponent.h"
#include "UE/InputComponent.h"

APlayer::APlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	__PlayerMesh = std::make_unique<UStaticMesh>();
	__PlayerMesh->SetSize(100, 100);
	__PlayerMesh->SetColor({ 220, 60, 60, 255 });

	__RootSceneComponent = CreateDefaultSubobject<USceneComponent>();
	SetRootComponent(__RootSceneComponent);

	__MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>();
	__MeshComponent->SetStaticMesh(__PlayerMesh.get());
	__MeshComponent->SetupAttachment(__RootSceneComponent);
	__MeshComponent->SetRelativeLocation({ 0.0, 0.0, 0.0 });

	SetActorLocation({ 100.0f, 100.0f, 0.0f });

}

APlayer::~APlayer() = default;

void APlayer::Tick(float _DeltaTime)
{
	Super::Tick(_DeltaTime);

	const float MoveDistance = __MoveSpeed * _DeltaTime;
	FVector NewLocation = GetActorLocation();

	NewLocation.X += __HorizontalInput * MoveDistance;
	NewLocation.Y += __VerticalInput * MoveDistance;

	SetActorLocation(NewLocation);
}

void APlayer::SetupPlayerInputComponent(UInputComponent* _InputComponent)
{
	if (nullptr == _InputComponent)
		return;

	_InputComponent->BindAxis("MoveHorizontal", this, &APlayer::MoveHorizontal);
	_InputComponent->BindAxis("MoveVertical", this, &APlayer::MoveVertical);
}

void APlayer::MoveHorizontal(float _Value)
{
	__HorizontalInput = _Value;
}

void APlayer::MoveVertical(float _Value)
{
	__VerticalInput = _Value;
}
