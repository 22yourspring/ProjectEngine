#pragma once

#include <utility>

template<typename ThreadGeneratedFunc, typename ThreadShutdownFunc>
TRAIIPattern_ThreadGuard<ThreadGeneratedFunc, ThreadShutdownFunc>::TRAIIPattern_ThreadGuard(
    ThreadGeneratedFunc InInit,
    ThreadShutdownFunc InShutdown)
    : __Shutdown(std::move(InShutdown))
{
    __Result = InInit();
    __Active = SUCCEEDED(__Result);
}

template<typename ThreadGeneratedFunc, typename ThreadShutdownFunc>
TRAIIPattern_ThreadGuard<ThreadGeneratedFunc, ThreadShutdownFunc>::~TRAIIPattern_ThreadGuard()
{
    if (__Active)
        __Shutdown();
}

template<typename ThreadGeneratedFunc, typename ThreadShutdownFunc>
bool TRAIIPattern_ThreadGuard<ThreadGeneratedFunc, ThreadShutdownFunc>::Failed() const
{
    return FAILED(__Result);
}