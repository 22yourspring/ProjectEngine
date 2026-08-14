#pragma once

#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

template <typename... Args>
std::string StringFormat(const char* Format, Args... Values)
{
    const int Required = std::snprintf(nullptr, 0, Format, Values...);
    if (Required <= 0)
    {
        return {};
    }

    std::vector<char> Buffer(static_cast<std::size_t>(Required) + 1);
    std::snprintf(Buffer.data(), Buffer.size(), Format, Values...);
    return std::string(Buffer.data(), static_cast<std::size_t>(Required));
}

template <typename T>
bool ParseValue(std::string_view Source, std::string_view Label, T& OutValue)
{
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
