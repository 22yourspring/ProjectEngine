#include "pch.h"
#include "UnrealString.h"

#include <algorithm>
#include <cwchar>
#include <cwctype>

namespace
{
    std::wstring ToComparable(
        std::wstring_view _Text,
        ESearchCase::Type _SearchCase)
    {
        std::wstring Result(_Text);

        if (_SearchCase == ESearchCase::IgnoreCase)
        {
            std::transform(
                Result.begin(),
                Result.end(),
                Result.begin(),
                [](TCHAR _Character)
                {
                    return static_cast<TCHAR>(std::towlower(_Character));
                });
        }

        return Result;
    }
}

FString::FString(const TCHAR* _Text)
    : __Data(_Text != nullptr ? _Text : L"")
{
}

FString::FString(std::wstring _Text)
    : __Data(std::move(_Text))
{
}

FString::FString(std::wstring_view _Text)
    : __Data(_Text)
{
}

FString::FString(const char* _Utf8Text)
    : __Data(Utf8ToWide(
        _Utf8Text != nullptr
            ? std::string_view(_Utf8Text)
            : std::string_view()))
{
}

FString::FString(const std::string& _Utf8Text)
    : __Data(Utf8ToWide(_Utf8Text))
{
}

FString::FString(std::string_view _Utf8Text)
    : __Data(Utf8ToWide(_Utf8Text))
{
}

const TCHAR* FString::c_str() const noexcept
{
    return __Data.c_str();
}

const TCHAR* FString::operator*() const noexcept
{
    return __Data.c_str();
}

int32 FString::Len() const noexcept
{
    return static_cast<int32>(__Data.size());
}

bool FString::IsEmpty() const noexcept
{
    return __Data.empty();
}

void FString::Empty(int32 _Slack)
{
    __Data.clear();

    if (_Slack > 0)
    {
        __Data.reserve(static_cast<std::size_t>(_Slack));
    }
}

void FString::Reset(int32 _NewReservedSize)
{
    __Data.clear();

    if (_NewReservedSize > static_cast<int32>(__Data.capacity()))
    {
        __Data.reserve(static_cast<std::size_t>(_NewReservedSize));
    }
}

void FString::Reserve(int32 _CharacterCount)
{
    if (_CharacterCount > 0)
    {
        __Data.reserve(static_cast<std::size_t>(_CharacterCount));
    }
}

FString& FString::Append(const FString& _Other)
{
    __Data.append(_Other.__Data);
    return *this;
}

FString& FString::Append(const TCHAR* _Text)
{
    if (_Text != nullptr)
    {
        __Data.append(_Text);
    }

    return *this;
}

FString& FString::Append(const TCHAR* _Text, int32 _Count)
{
    if (_Text != nullptr && _Count > 0)
    {
        __Data.append(_Text, static_cast<std::size_t>(_Count));
    }

    return *this;
}

bool FString::Contains(
    const FString& _SubStr,
    ESearchCase::Type _SearchCase) const
{
    return Find(_SubStr, _SearchCase) != INDEX_NONE;
}

bool FString::Contains(
    const TCHAR* _SubStr,
    ESearchCase::Type _SearchCase) const
{
    return Find(_SubStr, _SearchCase) != INDEX_NONE;
}

bool FString::StartsWith(
    const FString& _Prefix,
    ESearchCase::Type _SearchCase) const
{
    return StartsWith(*_Prefix, _SearchCase);
}

bool FString::StartsWith(
    const TCHAR* _Prefix,
    ESearchCase::Type _SearchCase) const
{
    if (_Prefix == nullptr)
    {
        return false;
    }

    const std::wstring Prefix(_Prefix);
    if (Prefix.size() > __Data.size())
    {
        return false;
    }

    return ToComparable(
        std::wstring_view(__Data).substr(0, Prefix.size()),
        _SearchCase) == ToComparable(Prefix, _SearchCase);
}

bool FString::EndsWith(
    const FString& _Suffix,
    ESearchCase::Type _SearchCase) const
{
    return EndsWith(*_Suffix, _SearchCase);
}

bool FString::EndsWith(
    const TCHAR* _Suffix,
    ESearchCase::Type _SearchCase) const
{
    if (_Suffix == nullptr)
    {
        return false;
    }

    const std::wstring Suffix(_Suffix);
    if (Suffix.size() > __Data.size())
    {
        return false;
    }

    return ToComparable(
        std::wstring_view(__Data).substr(__Data.size() - Suffix.size()),
        _SearchCase) == ToComparable(Suffix, _SearchCase);
}

int32 FString::Find(
    const FString& _SubStr,
    ESearchCase::Type _SearchCase,
    ESearchDir::Type _SearchDir,
    int32 _StartPosition) const
{
    return Find(*_SubStr, _SearchCase, _SearchDir, _StartPosition);
}

int32 FString::Find(
    const TCHAR* _SubStr,
    ESearchCase::Type _SearchCase,
    ESearchDir::Type _SearchDir,
    int32 _StartPosition) const
{
    if (_SubStr == nullptr)
    {
        return INDEX_NONE;
    }

    const std::wstring Source = ToComparable(__Data, _SearchCase);
    const std::wstring Search = ToComparable(_SubStr, _SearchCase);

    std::wstring::size_type Position = std::wstring::npos;

    if (_SearchDir == ESearchDir::FromStart)
    {
        const std::size_t Start = _StartPosition == INDEX_NONE
            ? 0
            : static_cast<std::size_t>((std::max)(_StartPosition, 0));
        Position = Source.find(Search, Start);
    }
    else
    {
        const std::size_t Start = _StartPosition == INDEX_NONE
            ? Source.size()
            : static_cast<std::size_t>((std::max)(_StartPosition, 0));
        Position = Source.rfind(Search, Start);
    }

    return Position == std::wstring::npos
        ? INDEX_NONE
        : static_cast<int32>(Position);
}

FString FString::Replace(
    const TCHAR* _From,
    const TCHAR* _To,
    ESearchCase::Type _SearchCase) const
{
    FString Result(*this);
    Result.ReplaceInline(_From, _To, _SearchCase);
    return Result;
}

int32 FString::ReplaceInline(
    const TCHAR* _SearchText,
    const TCHAR* _ReplacementText,
    ESearchCase::Type _SearchCase)
{
    if (_SearchText == nullptr || *_SearchText == L'\0')
    {
        return 0;
    }

    const std::wstring Replacement =
        _ReplacementText != nullptr ? _ReplacementText : L"";
    const std::size_t SearchLength = std::wcslen(_SearchText);
    int32 ReplacementCount = 0;
    int32 SearchStart = 0;

    for (;;)
    {
        const int32 Position = Find(
            _SearchText,
            _SearchCase,
            ESearchDir::FromStart,
            SearchStart);

        if (Position == INDEX_NONE)
        {
            break;
        }

        __Data.replace(
            static_cast<std::size_t>(Position),
            SearchLength,
            Replacement);

        SearchStart = Position + static_cast<int32>(Replacement.size());
        ++ReplacementCount;
    }

    return ReplacementCount;
}

bool FString::Equals(
    const FString& _Other,
    ESearchCase::Type _SearchCase) const
{
    return ToComparable(__Data, _SearchCase)
        == ToComparable(_Other.__Data, _SearchCase);
}

FString FString::ToUpper() const
{
    std::wstring Result(__Data);
    std::transform(
        Result.begin(),
        Result.end(),
        Result.begin(),
        [](TCHAR _Character)
        {
            return static_cast<TCHAR>(std::towupper(_Character));
        });
    return FString(std::move(Result));
}

FString FString::ToLower() const
{
    return FString(ToComparable(__Data, ESearchCase::IgnoreCase));
}

FString FString::TrimStartAndEnd() const
{
    const auto IsWhitespace = [](TCHAR _Character)
    {
        return std::iswspace(_Character) != 0;
    };

    const auto Begin = std::find_if_not(
        __Data.begin(),
        __Data.end(),
        IsWhitespace);
    const auto End = std::find_if_not(
        __Data.rbegin(),
        __Data.rend(),
        IsWhitespace).base();

    if (Begin >= End)
    {
        return {};
    }

    return FString(std::wstring(Begin, End));
}

int32 FString::ParseIntoArray(
    std::vector<FString>& _OutArray,
    const TCHAR* _Delimiter,
    bool _CullEmpty) const
{
    _OutArray.clear();

    if (_Delimiter == nullptr || *_Delimiter == L'\0')
    {
        if (!_CullEmpty || !IsEmpty())
        {
            _OutArray.emplace_back(*this);
        }
        return static_cast<int32>(_OutArray.size());
    }

    const std::wstring Delimiter(_Delimiter);
    std::size_t Start = 0;

    while (Start <= __Data.size())
    {
        const std::size_t End = __Data.find(Delimiter, Start);
        const std::size_t Count = End == std::wstring::npos
            ? __Data.size() - Start
            : End - Start;

        if (!_CullEmpty || Count > 0)
        {
            _OutArray.emplace_back(
                std::wstring_view(__Data).substr(Start, Count));
        }

        if (End == std::wstring::npos)
        {
            break;
        }

        Start = End + Delimiter.size();
    }

    return static_cast<int32>(_OutArray.size());
}

std::vector<TCHAR> FString::GetCharArray() const
{
    std::vector<TCHAR> Result(__Data.begin(), __Data.end());
    Result.push_back(L'\0');
    return Result;
}

FString FString::Left(int32 _Count) const
{
    const int32 Count = std::clamp(_Count, 0, Len());
    return FString(std::wstring_view(__Data).substr(
        0,
        static_cast<std::size_t>(Count)));
}

FString FString::Right(int32 _Count) const
{
    const int32 Count = std::clamp(_Count, 0, Len());
    return FString(std::wstring_view(__Data).substr(
        __Data.size() - static_cast<std::size_t>(Count)));
}

FString FString::Mid(int32 _Start, int32 _Count) const
{
    const int32 Start = std::clamp(_Start, 0, Len());
    const int32 Available = Len() - Start;
    const int32 Count = std::clamp(_Count, 0, Available);
    return FString(std::wstring_view(__Data).substr(
        static_cast<std::size_t>(Start),
        static_cast<std::size_t>(Count)));
}

const std::wstring& FString::ToWide() const noexcept
{
    return __Data;
}

std::string FString::ToUtf8() const
{
    return WideToUtf8(__Data);
}

FString& FString::operator+=(const FString& _Other)
{
    return Append(_Other);
}

FString& FString::operator+=(const TCHAR* _Other)
{
    return Append(_Other);
}

FString FString::FromUtf8(std::string_view _Utf8Text)
{
    return FString(_Utf8Text);
}

std::wstring FString::Utf8ToWide(std::string_view _Utf8Text)
{
    if (_Utf8Text.empty())
    {
        return {};
    }

#if defined(_WIN32)
    const int Required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        _Utf8Text.data(),
        static_cast<int>(_Utf8Text.size()),
        nullptr,
        0);

    if (Required <= 0)
    {
        return {};
    }

    std::wstring Result(static_cast<std::size_t>(Required), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        _Utf8Text.data(),
        static_cast<int>(_Utf8Text.size()),
        Result.data(),
        Required);
    return Result;
#else
    return std::wstring(_Utf8Text.begin(), _Utf8Text.end());
#endif
}

std::string FString::WideToUtf8(std::wstring_view _WideText)
{
    if (_WideText.empty())
    {
        return {};
    }

#if defined(_WIN32)
    const int Required = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        _WideText.data(),
        static_cast<int>(_WideText.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (Required <= 0)
    {
        return {};
    }

    std::string Result(static_cast<std::size_t>(Required), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        _WideText.data(),
        static_cast<int>(_WideText.size()),
        Result.data(),
        Required,
        nullptr,
        nullptr);
    return Result;
#else
    return std::string(_WideText.begin(), _WideText.end());
#endif
}
