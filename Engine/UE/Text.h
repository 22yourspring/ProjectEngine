#pragma once

#include "UnrealString.h"

#include <string_view>







class FText
{
public:
    FText() = default;
    FText(const FText&) = default;
    FText(FText&&) noexcept = default;
    FText& operator=(const FText&) = default;
    FText& operator=(FText&&) noexcept = default;

    [[nodiscard]] static FText FromString(const FString& _String);
    [[nodiscard]] static FText FromString(FString&& _String);
    [[nodiscard]] static FText FromString(const TCHAR* _String);
    [[nodiscard]] static FText FromString(std::wstring_view _String);
    [[nodiscard]] static FText FromString(std::string_view _Utf8String);

    [[nodiscard]] static FText AsCultureInvariant(const FString& _String);
    [[nodiscard]] static const FText& GetEmpty();

    [[nodiscard]] bool IsEmpty() const noexcept;
    [[nodiscard]] FString ToString() const;

    friend bool operator==(const FText& _Left, const FText& _Right) noexcept
    {
        return _Left.__Text == _Right.__Text;
    }

    friend bool operator!=(const FText& _Left, const FText& _Right) noexcept
    {
        return !(_Left == _Right);
    }

private:
    explicit FText(FString _String);

    FString __Text;
};
