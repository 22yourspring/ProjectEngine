#pragma once
#include "Object.h"

ENGINE_API UObject* LoadAssetObject(const TCHAR* _ObjectPath, FString* _Error = nullptr);

template<typename T>
T* LoadObject(UObject* _Outer, const TCHAR* _ObjectPath);

#include "UObjectGlobals.inl"
