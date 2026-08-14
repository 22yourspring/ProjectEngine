#pragma once

#include "UnrealString.h"

#include <cstddef>
#include <functional>
#include <string_view>

/**
 * Lightweight Unreal-style immutable name.
 *
 * This compatibility type keeps the FName API used by the imported math code.
 * Unlike Unreal Engine's global name table, it owns its text through FString.
 */
class FName
{
public:
    FName() = default;
    FName(const FName&) = default;
    FName(FName&&) noexcept = default;
    FName& operator=(const FName&) = default;
    FName& operator=(FName&&) noexcept = default;

    explicit FName(const FString& _Name);
    explicit FName(FString&& _Name) noexcept;
    FName(const TCHAR* _Name);
    FName(std::wstring_view _Name);
    FName(const char* _Utf8Name);
    FName(std::string_view _Utf8Name);

    [[nodiscard]] bool IsNone() const noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] FString ToString() const;
    [[nodiscard]] const FString& GetPlainNameString() const noexcept;

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

extern const FName NAME_None;

namespace std
{
    template <>
    struct hash<FName>
    {
        [[nodiscard]] std::size_t operator()(const FName& _Name) const noexcept
        {
            return std::hash<std::wstring>{}(_Name.GetPlainNameString().ToWide());
        }
    };
}
