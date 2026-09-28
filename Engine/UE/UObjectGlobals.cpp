#include "pch.h"
#include "UObjectGlobals.h"
#include "ResourceEngineSystem.h"

UObject* LoadAssetObject(const TCHAR* _ObjectPath, FString* _Error)
{
    FString Error;
    UObject* Result = nullptr;
    if (_ObjectPath && GEngine)
    {
        if (auto* Resources = GEngine->GetEngineSystem<ResourceEngineSystem>())
            Result = Resources->GetPackageStore().Load(_ObjectPath, Error);
    }
    if (!Result && Error.IsEmpty()) Error = TEXT("Asset loading requires an initialized engine and an object path.");
    if (_Error) *_Error = Error;
    return Result;
}
