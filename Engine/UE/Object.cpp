#include "pch.h"
#include "Object.h"
#include "Archive.h"
#include <algorithm>

void UObject::AddTag(const FString& _Tag)
{
    __Tag.push_back(_Tag);
}

UObject::UObject()
{
}

UObject::~UObject()
{
}

UObject::UObject(UObject&&) noexcept = default;
UObject& UObject::operator=(UObject&&) noexcept = default;

FString UObject::GetPathName() const
{
    return __Outer ? __Outer->GetPathName() + TEXT(".") + __ObjectName : __ObjectName;
}

void UObject::Serialize(FArchive& _Archive)
{
    _Archive.String(__ObjectName);
    uint32 Count = static_cast<uint32>(__Tag.size());
    _Archive.UInt32(Count);
    if (Count > 1024) { _Archive.SetError(); return; }
    if (_Archive.IsLoading()) __Tag.resize(Count);
    for (auto& Tag : __Tag)
    {
        _Archive.String(Tag);
    }
}

void UObject::RemoveTag(const FString& _Tag)
{
    auto iter = std::remove(__Tag.begin(), __Tag.end(), _Tag);
    if (iter != __Tag.end())
        __Tag.erase(iter, __Tag.end());
}

bool UObject::HasTag(const FString& _Tag) const
{
    return std::find(__Tag.begin(), __Tag.end(), _Tag) != __Tag.end();
}

