#include "pch.h"
#include "Object.h"
#include <algorithm>

template<typename T>
inline void UObject::AddTag(T&& _Tag)
{
	__Tag.emplace_back(std::forward<T>(_Tag));
}

UObject::UObject()
{
}

UObject::~UObject()
{
}

void UObject::RemoveTag(const std::string& _Tag)
{
    auto iter = std::remove(__Tag.begin(), __Tag.end(), _Tag);
    if (iter != __Tag.end())
        __Tag.erase(iter, __Tag.end());
}

bool UObject::HasTag(const std::string& _Tag) const
{
    return std::find(__Tag.begin(), __Tag.end(), _Tag) != __Tag.end();
}

template void UObject::AddTag<std::string>(std::string&&);
