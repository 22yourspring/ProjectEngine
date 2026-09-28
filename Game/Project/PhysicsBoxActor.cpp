#include "pch.h"
#include "PhysicsBoxActor.h"
#include "UE/BoxComponent.h"
#include "UE/StaticMeshComponent.h"
#include "UE/StaticMesh.h"

APhysicsBoxActor::APhysicsBoxActor()
{
    __CollisionComponent = CreateDefaultSubobject<UBoxComponent>();
    SetRootComponent(__CollisionComponent);
    __CollisionComponent->SetBoxExtent(FVector(30, 30, 20));
    __CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    __CollisionComponent->SetCollisionObjectType(ECC_PhysicsBody);
    __CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
    __CollisionComponent->SetEnableGravity(false);
    __CollisionComponent->BodyInstance.bUseCCD = true;
    __CollisionComponent->BodyInstance.SetDOFLock(EDOFMode::XYPlane);
    __CollisionComponent->SetLinearDamping(1.2f);
    __CollisionComponent->SetAngularDamping(1.5f);
    __CollisionComponent->SetNotifyRigidBodyCollision(true);
    __CollisionComponent->SetSimulatePhysics(true);

    __MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>();
    __MeshComponent->SetupAttachment(__CollisionComponent);
    __MeshComponent->SetRelativeLocation(FVector(-30, -30, 0));
    __Mesh = std::make_unique<UStaticMesh>();
    __Mesh->SetSize(60, 60);
    __Mesh->SetColor(FColor(255, 200, 50));
    __MeshComponent->SetStaticMesh(__Mesh.get());
    __MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

APhysicsBoxActor::~APhysicsBoxActor() = default;
