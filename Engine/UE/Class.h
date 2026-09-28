#pragma once

class UObject;
class UClass
{
public:
    bool IsInstance(const UObject* _Object) const { return __Matches && __Matches(_Object); }
    template<typename T> static UClass* For();
private:
    explicit UClass(bool (*_Matches)(const UObject*)) : __Matches(_Matches) {}
    bool (*__Matches)(const UObject*);
};

#include "Class.inl"
