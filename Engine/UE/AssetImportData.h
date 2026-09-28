#pragma once
#include "Object.h"

UCLASS(MinimalAPI)
class UAssetImportData : public UObject
{
    GENERATED_BODY()

public:
    const FString& GetFirstFilename() const { return __SourceFilename; }
    void UpdateFilenameOnly(FString _Filename) { __SourceFilename = std::move(_Filename); }

private:
    FString __SourceFilename;
};
