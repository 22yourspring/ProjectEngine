#pragma once

template<typename T>
T* LoadObject(UObject* _Outer, const TCHAR* _ObjectPath)
{
    T* Object = dynamic_cast<T*>(LoadAssetObject(_ObjectPath));
    return Object && (!_Outer || Object->GetOuter() == _Outer) ? Object : nullptr;
}
