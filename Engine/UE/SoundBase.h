#pragma once
#include "Object.h"

UCLASS(Abstract, MinimalAPI)
class ENGINE_API USoundBase : public UObject
{
    GENERATED_BODY()

public:
    virtual float GetDuration() const = 0;
};
