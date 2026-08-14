#pragma once

#include "CoreTypes.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ESearchCase
{
    enum Type
    {
        CaseSensitive,
        IgnoreCase
    };
}

namespace ESearchDir
{
    enum Type
    {
        FromStart,
        FromEnd
    };
}

/**
 * Lightweight Unreal-style owning string.
 *
 * FString keeps Unreal's common API surface while using std::wstring as its
 * storage. UTF-8 conversion is explicit at external-system boundaries.
 */
class FString
{
public:
    using CharType = TCHAR;
    using SizeType = std::wstring::size_type;

    FString() = default;
    FString(const FString&) = default;
    FString(FString&&) noexcept = default;
    FString& operator=(const FString&) = default;
    FString& operator=(FString&&) noexcept = default;

    FString(const TCHAR* _Text);
    FString(std::wstring _Text);
    FString(std::wstring_view _Text);

    // Compatibility constructors for UTF-8 sources used by this project.
    FString(const char* _Utf8Text);
    FString(const std::string& _Utf8Text);
    FString(std::string_view _Utf8Text);

    [[nodiscard]] const TCHAR* c_str() const noexcept;
    [[nodiscard]] const TCHAR* operator*() const noexcept;

    [[nodiscard]] int32 Len() const noexcept;
    [[nodiscard]] bool IsEmpty() const noexcept;

    void Empty(int32 _Slack = 0);
    void Reset(int32 _NewReservedSize = 0);
    void Reserve(int32 _CharacterCount);

    FString& Append(const FString& _Other);
    FString& Append(const TCHAR* _Text);
    FString& Append(const TCHAR* _Text, int32 _Count);

    [[nodiscard]] bool Contains(
        const FString& _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    [[nodiscard]] bool Contains(
        const TCHAR* _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;

    [[nodiscard]] bool StartsWith(
        const FString& _Prefix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    [[nodiscard]] bool StartsWith(
        const TCHAR* _Prefix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;

    [[nodiscard]] bool EndsWith(
        const FString& _Suffix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    [[nodiscard]] bool EndsWith(
        const TCHAR* _Suffix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;

    [[nodiscard]] int32 Find(
        const FString& _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase,
        ESearchDir::Type _SearchDir = ESearchDir::FromStart,
        int32 _StartPosition = INDEX_NONE) const;
    [[nodiscard]] int32 Find(
        const TCHAR* _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase,
        ESearchDir::Type _SearchDir = ESearchDir::FromStart,
        int32 _StartPosition = INDEX_NONE) const;

    [[nodiscard]] FString Replace(
        const TCHAR* _From,
        const TCHAR* _To,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    int32 ReplaceInline(
        const TCHAR* _SearchText,
        const TCHAR* _ReplacementText,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase);

    [[nodiscard]] bool Equals(
        const FString& _Other,
        ESearchCase::Type _SearchCase = ESearchCase::CaseSensitive) const;

    [[nodiscard]] FString ToUpper() const;
    [[nodiscard]] FString ToLower() const;
    [[nodiscard]] FString TrimStartAndEnd() const;

    int32 ParseIntoArray(
        std::vector<FString>& _OutArray,
        const TCHAR* _Delimiter,
        bool _CullEmpty = true) const;

    [[nodiscard]] std::vector<TCHAR> GetCharArray() const;
    [[nodiscard]] FString Left(int32 _Count) const;
    [[nodiscard]] FString Right(int32 _Count) const;
    [[nodiscard]] FString Mid(int32 _Start, int32 _Count = INT32_MAX) const;

    [[nodiscard]] const std::wstring& ToWide() const noexcept;
    [[nodiscard]] std::string ToUtf8() const;

    FString& operator+=(const FString& _Other);
    FString& operator+=(const TCHAR* _Other);

    friend FString operator+(FString _Left, const FString& _Right)
    {
        _Left += _Right;
        return _Left;
    }

    friend bool operator==(const FString& _Left, const FString& _Right) noexcept
    {
        return _Left.__Data == _Right.__Data;
    }

    friend bool operator!=(const FString& _Left, const FString& _Right) noexcept
    {
        return !(_Left == _Right);
    }

    [[nodiscard]] static FString FromUtf8(std::string_view _Utf8Text);

    template <typename... Args>
    [[nodiscard]] static FString Printf(const TCHAR* _Format, Args... _Values);

private:
    static std::wstring Utf8ToWide(std::string_view _Utf8Text);
    static std::string WideToUtf8(std::wstring_view _WideText);

    std::wstring __Data;
};

#include "UnrealString.inl"
