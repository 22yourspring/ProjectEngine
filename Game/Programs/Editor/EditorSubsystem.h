#pragma once
#include "UE/Object.h"

class UEditorSubsystem : public UObject
{
public:
    virtual void Initialize() {}
    virtual void Deinitialize() {}
};
