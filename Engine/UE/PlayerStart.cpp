#include "pch.h"
#include "PlayerStart.h"
#include "SceneComponent.h"

APlayerStart::APlayerStart()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>());
}
