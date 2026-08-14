#pragma once

template <typename ThreadGeneratedFunc, typename ThreadShutdownFunc>
class TRAIIPattern_ThreadGuard
{
public:
    TRAIIPattern_ThreadGuard(ThreadGeneratedFunc InInit, ThreadShutdownFunc InShutdown);

    ~TRAIIPattern_ThreadGuard();

    bool Failed() const;

private:
    ThreadShutdownFunc __Shutdown;
    int     __Result = E_FAIL;
    bool    __Active = false;
};

#include "T_RAIIPattern_ThreadGuard.inl"