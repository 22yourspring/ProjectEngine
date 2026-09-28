#pragma once

#include "CoreTypes.h"

#include <functional>
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

    CORE_API FString(const TCHAR* _Text);
    CORE_API FString(std::wstring _Text);
    CORE_API FString(std::wstring_view _Text);

    
    CORE_API FString(const char* _Utf8Text);
    CORE_API FString(const std::string& _Utf8Text);
    CORE_API FString(std::string_view _Utf8Text);

    [[nodiscard]] CORE_API const TCHAR* c_str() const noexcept;
    [[nodiscard]] CORE_API const TCHAR* operator*() const noexcept;

    [[nodiscard]] CORE_API int32 Len() const noexcept;
    [[nodiscard]] CORE_API bool IsEmpty() const noexcept;

    CORE_API void Empty(int32 _Slack = 0);
    CORE_API void Reset(int32 _NewReservedSize = 0);
    CORE_API void Reserve(int32 _CharacterCount);

    CORE_API FString& Append(const FString& _Other);
    CORE_API FString& Append(const TCHAR* _Text);
    CORE_API FString& Append(const TCHAR* _Text, int32 _Count);

    [[nodiscard]] CORE_API bool Contains(
        const FString& _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    [[nodiscard]] CORE_API bool Contains(
        const TCHAR* _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;

    [[nodiscard]] CORE_API bool StartsWith(
        const FString& _Prefix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    [[nodiscard]] CORE_API bool StartsWith(
        const TCHAR* _Prefix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;

    [[nodiscard]] CORE_API bool EndsWith(
        const FString& _Suffix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    [[nodiscard]] CORE_API bool EndsWith(
        const TCHAR* _Suffix,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;

    [[nodiscard]] CORE_API int32 Find(
        const FString& _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase,
        ESearchDir::Type _SearchDir = ESearchDir::FromStart,
        int32 _StartPosition = INDEX_NONE) const;
    [[nodiscard]] CORE_API int32 Find(
        const TCHAR* _SubStr,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase,
        ESearchDir::Type _SearchDir = ESearchDir::FromStart,
        int32 _StartPosition = INDEX_NONE) const;

    [[nodiscard]] CORE_API FString Replace(
        const TCHAR* _From,
        const TCHAR* _To,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase) const;
    CORE_API int32 ReplaceInline(
        const TCHAR* _SearchText,
        const TCHAR* _ReplacementText,
        ESearchCase::Type _SearchCase = ESearchCase::IgnoreCase);

    [[nodiscard]] CORE_API bool Equals(
        const FString& _Other,
        ESearchCase::Type _SearchCase = ESearchCase::CaseSensitive) const;

    [[nodiscard]] CORE_API FString ToUpper() const;
    [[nodiscard]] CORE_API FString ToLower() const;
    [[nodiscard]] CORE_API FString TrimStartAndEnd() const;

    CORE_API int32 ParseIntoArray(
        std::vector<FString>& _OutArray,
        const TCHAR* _Delimiter,
        bool _CullEmpty = true) const;

    [[nodiscard]] CORE_API std::vector<TCHAR> GetCharArray() const;
    [[nodiscard]] CORE_API FString Left(int32 _Count) const;
    [[nodiscard]] CORE_API FString Right(int32 _Count) const;
    [[nodiscard]] CORE_API FString Mid(int32 _Start, int32 _Count = INT32_MAX) const;

    [[nodiscard]] CORE_API const std::wstring& ToWide() const noexcept;
    [[nodiscard]] CORE_API std::string ToUtf8() const;

    CORE_API FString& operator+=(const FString& _Other);
    CORE_API FString& operator+=(const TCHAR* _Other);

    friend FString operator+(FString _Left, const FString& _Right)
    {
        _Left += _Right;
        return _Left;
    }

    TCHAR& operator[](int32 _Index) { return __Data[_Index]; }
    const TCHAR& operator[](int32 _Index) const { return __Data[_Index]; }
    auto begin() noexcept { return __Data.begin(); }
    auto end() noexcept { return __Data.end(); }
    auto begin() const noexcept { return __Data.begin(); }
    auto end() const noexcept { return __Data.end(); }
    FString& AppendChar(TCHAR _Character) { __Data.push_back(_Character); return *this; }
    void InsertAt(int32 _Index, TCHAR _Character) { __Data.insert(__Data.begin() + _Index, _Character); }
    void RemoveAt(int32 _Index, int32 _Count = 1) { __Data.erase(_Index, _Count); }

    friend bool operator<(const FString& _Left, const FString& _Right) noexcept
    {
        return _Left.__Data < _Right.__Data;
    }

    friend bool operator==(const FString& _Left, const FString& _Right) noexcept
    {
        return _Left.__Data == _Right.__Data;
    }

    friend bool operator!=(const FString& _Left, const FString& _Right) noexcept
    {
        return !(_Left == _Right);
    }

    [[nodiscard]] static CORE_API FString FromUtf8(std::string_view _Utf8Text);

    template <typename... Args>
    [[nodiscard]] static FString Printf(const TCHAR* _Format, Args... _Values);

private:
    static std::wstring Utf8ToWide(std::string_view _Utf8Text);
    static std::string WideToUtf8(std::wstring_view _WideText);

    std::wstring __Data;
};

#include "UnrealString.inl"

namespace std
{
    template <>
    struct hash<FString>
    {
        size_t operator()(const FString& _Value) const noexcept
        {
            return hash<wstring>{}(_Value.ToWide());
        }
    };
}
