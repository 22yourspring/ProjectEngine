#include "pch.h"

#include "NameTypes.h"

const FName NAME_None;

FName::FName(const FString& _Name)
    : __Name(_Name)
{
}

FName::FName(FString&& _Name) noexcept
    : __Name(std::move(_Name))
{
}

FName::FName(const TCHAR* _Name)
    : __Name(_Name)
{
}

FName::FName(std::wstring_view _Name)
    : __Name(_Name)
{
}

FName::FName(const char* _Utf8Name)
    : __Name(_Utf8Name)
{
}

FName::FName(std::string_view _Utf8Name)
    : __Name(_Utf8Name)
{
}

bool FName::IsNone() const noexcept
{
    return __Name.IsEmpty();
}

bool FName::IsValid() const noexcept
{
    return !IsNone();
}

FString FName::ToString() const
{
    return __Name;
}

const FString& FName::GetPlainNameString() const noexcept
{
    return __Name;
}
