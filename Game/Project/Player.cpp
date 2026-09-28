#include "pch.h"
#include "Player.h"
#include "UE/Archive.h"
#include <cmath>
#include "UE/PrimitiveComponent.h"
#include "UE/StaticMesh.h"
#include "UE/StaticMeshComponent.h"
#include "UE/BoxComponent.h"
#include "UE/InputComponent.h"
#include "UE/GameplayStatics.h"
#include "UE/World.h"
#include "UE/Level.h"

void APlayer::Serialize(FArchive& _Archive)
{
    ACharacter::Serialize(_Archive);
    _Archive.Serialize(&__MoveSpeed, sizeof(__MoveSpeed));
    if (!std::isfinite(__MoveSpeed) || __MoveSpeed < 0) _Archive.SetError();
}


APlayer::APlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	__PlayerMesh = std::make_unique<UStaticMesh>();
	__PlayerMesh->SetSize(100, 100);
	__PlayerMesh->SetColor({ 220, 60, 60, 255 });

	__CollisionComponent = CreateDefaultSubobject<UBoxComponent>();
    SetRootComponent(__CollisionComponent);
    __CollisionComponent->SetBoxExtent(FVector(50, 50, 20));
    __CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    __CollisionComponent->SetCollisionObjectType(ECC_Pawn);
    __CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
    __CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    __CollisionComponent->SetEnableGravity(false);
    __CollisionComponent->BodyInstance.bUseCCD = true;
    __CollisionComponent->BodyInstance.bLockZTranslation = true;
    __CollisionComponent->BodyInstance.bLockXRotation = true;
    __CollisionComponent->BodyInstance.bLockYRotation = true;
    __CollisionComponent->BodyInstance.bLockZRotation = true;
    __CollisionComponent->BodyInstance.SetDOFLock(EDOFMode::SixDOF);
    __CollisionComponent->SetNotifyRigidBodyCollision(true);
    __CollisionComponent->SetSimulatePhysics(true);

	__MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>();
	__MeshComponent->SetStaticMesh(__PlayerMesh.get());
	__MeshComponent->SetupAttachment(__CollisionComponent);
	__MeshComponent->SetRelativeLocation({ -50.0, -50.0, 0.0 });

	SetActorLocation({ 150.0f, 150.0f, 0.0f });

}

APlayer::~APlayer() = default;

void APlayer::Tick(float _DeltaTime)
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

void APlayer::SetupPlayerInputComponent(UInputComponent* _InputComponent)
{
	if (nullptr == _InputComponent)
		return;

	_InputComponent->BindAxis("MoveHorizontal", this, &APlayer::MoveHorizontal);
	_InputComponent->BindAxis("MoveVertical", this, &APlayer::MoveVertical);
	_InputComponent->BindAction("OpenNextLevel", EInputEvent::Pressed,
		this, &APlayer::OpenNextLevel);
}

void APlayer::OpenNextLevel()
{
	UWorld* World = GetWorld();
	if (nullptr == World || nullptr == World->GetPersistentLevel())
		return;

	UGameplayStatics::OpenLevel(this,
		World->IsPersistentLevel(TEXT("Stage1")) ? TEXT("Stage2") : TEXT("Stage1"));
}

void APlayer::MoveHorizontal(float _Value)
{
	__HorizontalInput = _Value;
}

void APlayer::MoveVertical(float _Value)
{
	__VerticalInput = _Value;
}
