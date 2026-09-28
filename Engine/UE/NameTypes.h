#pragma once

#include "UnrealString.h"
#include "define.h"

#include <cstddef>
#include <functional>
#include <string_view>







class FName
{
public:
    FName() = default;
    FName(const FName&) = default;
    FName(FName&&) noexcept = default;
    FName& operator=(const FName&) = default;
    FName& operator=(FName&&) noexcept = default;

    CORE_API explicit FName(const FString& _Name);
    CORE_API explicit FName(FString&& _Name) noexcept;
    CORE_API FName(const TCHAR* _Name);
    CORE_API FName(std::wstring_view _Name);
    CORE_API FName(const char* _Utf8Name);
    CORE_API FName(std::string_view _Utf8Name);

    [[nodiscard]] CORE_API bool IsNone() const noexcept;
    [[nodiscard]] CORE_API bool IsValid() const noexcept;
    [[nodiscard]] CORE_API FString ToString() const;
    [[nodiscard]] CORE_API const FString& GetPlainNameString() const noexcept;

    friend bool operator==(const FName& _Left, const FName& _Right) noexcept
    {
        return _Left.__Name == _Right.__Name;
    }

    friend bool operator!=(const FName& _Left, const FName& _Right) noexcept
    {
        return !(_Left == _Right);
    }

private:
    FString __Name;
};

extern CORE_API const FName NAME_None;

namespace std
{
    template <>
    struct hash<FName>
    {
        [[nodiscard]] std::size_t operator()(const FName& _Name) const noexcept
        {
            return std::hash<FString>{}(_Name.GetPlainNameString());
        }
    };
}
