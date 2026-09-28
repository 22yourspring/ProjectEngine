#pragma once

template<typename T>
UClass* UClass::For()
{
    static UClass Class([](const UObject* _Object) { return dynamic_cast<const T*>(_Object) != nullptr; });
    return &Class;
}
