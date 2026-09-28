#pragma once
#include "UE/Actor.h"

class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class APhysicsBoxActor final : public AActor
{
    GENERATED_BODY()

public:
    APhysicsBoxActor();
    ~APhysicsBoxActor() override;

private:
    UPROPERTY(VisibleAnywhere)
    UBoxComponent* __CollisionComponent = nullptr;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* __MeshComponent = nullptr;

private:
    std::unique_ptr<UStaticMesh> __Mesh;
};
