#pragma once

#include "Delegate.h"
#include "MulticastDelegate.h"


#define DECLARE_DELEGATE(DelegateName) \
        using DelegateName = TDelegate<void()>

#define DECLARE_DELEGATE_OneParam(DelegateName, ParamType) \
        using DelegateName = TDelegate<void(ParamType)>

#define DECLARE_DELEGATE_TwoParams(DelegateName, ParamType1, ParamType2) \
        using DelegateName = TDelegate<void(ParamType1, ParamType2)>

#define DECLARE_MULTICAST_DELEGATE(DelegateName) \
        using DelegateName = TMulticastDelegate<void()>

#define DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, ParamType) \
        using DelegateName = TMulticastDelegate<void(ParamType)>

#define DECLARE_MULTICAST_DELEGATE_TwoParams(DelegateName, ParamType1, ParamType2) \
        using DelegateName = TMulticastDelegate<void(ParamType1, ParamType2)>