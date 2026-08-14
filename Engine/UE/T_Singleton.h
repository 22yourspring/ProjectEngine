#pragma once

template<typename T>
class TSingleton
{
public:
    static T& Get();
    static T* GetInstance();

protected:
    TSingleton() = default;
    ~TSingleton() = default;

private:
    TSingleton(const TSingleton&) = delete;
    TSingleton& operator=(const TSingleton&) = delete;

    TSingleton(TSingleton&&) = delete;
    TSingleton& operator=(TSingleton&&) = delete;
};

#include "T_Singleton.inl"