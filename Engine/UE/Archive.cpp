#include "pch.h"
#include "Archive.h"
#include "Package.h"
#include <istream>
#include <ostream>
#include <limits>

FArchive::FArchive(std::istream& _Input, uint64 _Size) : __Input(&_Input), __Remaining(_Size) {}
FArchive::FArchive(std::ostream& _Output) : __Output(&_Output) {}

void FArchive::AssetReference(UObject*& _Object)
{
    FString Path;
    if (!IsLoading() && _Object)
    {
        if (!dynamic_cast<UPackage*>(_Object->GetOuter())) { SetError(); return; }
        Path = _Object->GetPathName();
        if (Path.IsEmpty()) { SetError(); return; }
    }
    String(Path);
    if (IsLoading() && !IsError())
    {
        _Object = Path.IsEmpty() || !__AssetResolver ? nullptr : __AssetResolver(Path);
        if (!Path.IsEmpty() && !_Object) SetError();
    }
}

void FArchive::SetObjectReferences(std::function<uint32(UObject*)> _Save, std::function<UObject*(uint32)> _Load)
{
    __SaveReference = std::move(_Save); __LoadReference = std::move(_Load);
}
void FArchive::ObjectReference(UObject*& _Object)
{
    uint32 ID = 0;
    if (!IsLoading() && _Object)
    {
        if (!__SaveReference) { SetError(); return; }
        ID = __SaveReference(_Object);
        if (!ID) { SetError(); return; }
    }
    UInt32(ID);
    if (IsLoading())
    {
        _Object = ID && __LoadReference ? __LoadReference(ID) : nullptr;
        if (ID && !_Object) SetError();
    }
}

void FArchive::Serialize(void* _Data, uint64 _Size)
{
    if (__Error || _Size > static_cast<uint64>((std::numeric_limits<std::streamsize>::max)()))
    {
        SetError();
        return;
    }
    if (__Input)
    {
        if (_Size > __Remaining) { SetError(); return; }
        __Input->read(static_cast<char*>(_Data), static_cast<std::streamsize>(_Size));
        __Remaining -= _Size;
        __Error = !*__Input;
    }
    else
    {
        __Output->write(static_cast<const char*>(_Data), static_cast<std::streamsize>(_Size));
        __Error = !*__Output;
    }
}

void FArchive::UInt32(uint32& _Value)
{
    uint8 Data[4] = {};
    if (!IsLoading())
        for (uint32 Index = 0; Index < 4; ++Index) Data[Index] = static_cast<uint8>(_Value >> (Index * 8));
    Serialize(Data, 4);
    if (IsLoading() && !IsError())
        _Value = uint32(Data[0]) | uint32(Data[1]) << 8 | uint32(Data[2]) << 16 | uint32(Data[3]) << 24;
}

void FArchive::Bytes(std::vector<uint8>& _Data, uint32 _MaximumSize)
{
    if (_Data.size() > _MaximumSize) { SetError(); return; }
    uint32 Size = static_cast<uint32>(_Data.size());
    UInt32(Size);
    if (IsError() || Size > _MaximumSize || (IsLoading() && Size > Remaining())) { SetError(); return; }
    if (IsLoading()) _Data.resize(Size);
    if (Size) Serialize(_Data.data(), Size);
}

void FArchive::String(FString& _Value)
{
    std::vector<uint8> Data;
    if (!IsLoading())
    {
        const std::wstring Wide = _Value.ToWide();
        const int Size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Wide.data(), static_cast<int>(Wide.size()), nullptr, 0, nullptr, nullptr);
        if (!Wide.empty() && !Size) { SetError(); return; }
        Data.resize(Size);
        if (Size) WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Wide.data(), static_cast<int>(Wide.size()), reinterpret_cast<char*>(Data.data()), Size, nullptr, nullptr);
    }
    Bytes(Data, 32768);
    if (IsLoading() && !IsError())
    {
        if (Data.empty()) { _Value = FString(); return; }
        const int Size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(Data.data()), static_cast<int>(Data.size()), nullptr, 0);
        if (!Size) { SetError(); return; }
        std::wstring Wide(Size, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(Data.data()), static_cast<int>(Data.size()), Wide.data(), Size);
        if (Wide.find(L'\0') != std::wstring::npos) { SetError(); return; }
        _Value = FString(std::move(Wide));
    }
}
