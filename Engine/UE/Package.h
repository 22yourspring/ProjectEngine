#pragma once
#include "Object.h"

UCLASS(MinimalAPI)
class COREUOBJECT_API UPackage final : public UObject
{
    GENERATED_BODY()

public:
    bool IsDirty() const { return __Dirty; }
    void SetDirtyFlag(bool _Dirty) { __Dirty = _Dirty; }
    const FString& GetImportSource() const { return __ImportSource; }

private:
    friend class FPackageStore;
    FString __ImportSource;
    bool __Dirty = false;
};
