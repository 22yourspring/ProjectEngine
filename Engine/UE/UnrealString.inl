#pragma once

#include <cwchar>
#include <vector>

template <typename... Args>
FString FString::Printf(const TCHAR* _Format, Args... _Values)
{
    if (_Format == nullptr)
    {
        return {};
    }

    std::vector<TCHAR> Buffer(256);

    for (;;)
    {
        const int Written = std::swprintf(
            Buffer.data(),
            Buffer.size(),
            _Format,
            _Values...);

        if (Written >= 0 && static_cast<std::size_t>(Written) < Buffer.size())
        {
            return FString(std::wstring(
                Buffer.data(),
                static_cast<std::size_t>(Written)));
        }

        Buffer.resize(Buffer.size() * 2);
    }
}
