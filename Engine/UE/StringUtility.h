#pragma once

#include <charconv>
#include <cstdio>
#include "UnrealString.h"
#include <string_view>
#include <type_traits>
#include <vector>

template <typename... Args>
FString StringFormat(const char* Format, Args... Values)
{
    const int Required = std::snprintf(nullptr, 0, Format, Values...);
    if (Required <= 0)
    {
        return {};
    }

    std::vector<char> Buffer(static_cast<std::size_t>(Required) + 1);
    std::snprintf(Buffer.data(), Buffer.size(), Format, Values...);
    return FString::FromUtf8(std::string_view(Buffer.data(), static_cast<std::size_t>(Required)));
}

template <typename T>
bool ParseValue(const FString& _Source, const FString& _Label, T& OutValue)
{
    const auto Source = _Source.ToUtf8();
    const auto Label = _Label.ToUtf8();
    const std::size_t LabelPosition = Source.find(Label);
    if (LabelPosition == std::string_view::npos)
    {
        return false;
    }

    const char* Begin = Source.data() + LabelPosition + Label.size();
    const char* End = Source.data() + Source.size();

    if constexpr (std::is_integral_v<T>)
    {
        const auto Result = std::from_chars(Begin, End, OutValue);
        return Result.ec == std::errc{};
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        const auto Result = std::from_chars(Begin, End, OutValue);
        return Result.ec == std::errc{};
    }
    else
    {
        return false;
    }
}
