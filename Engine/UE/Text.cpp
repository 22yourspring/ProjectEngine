#include "pch.h"

#include "Text.h"

FText::FText(FString _String)
    : __Text(std::move(_String))
{
}

FText FText::FromString(const FString& _String)
{
    return FText(_String);
}

FText FText::FromString(FString&& _String)
{
    return FText(std::move(_String));
}

FText FText::FromString(const TCHAR* _String)
{
    return FText(FString(_String));
}

FText FText::FromString(std::wstring_view _String)
{
    return FText(FString(_String));
}

FText FText::FromString(std::string_view _Utf8String)
{
    return FText(FString(_Utf8String));
}

FText FText::AsCultureInvariant(const FString& _String)
{
    return FText(_String);
}

const FText& FText::GetEmpty()
{
    static const FText EmptyText;
    return EmptyText;
}

bool FText::IsEmpty() const noexcept
{
    return __Text.IsEmpty();
}

FString FText::ToString() const
{
    return __Text;
}
