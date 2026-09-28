#pragma once

#include "CoreMinimal.h"
#include "define.h"
#include <iosfwd>
#include <functional>

class UObject;

class CORE_API FArchive
{
public:
    explicit FArchive(std::istream& _Input, uint64 _Size);
    explicit FArchive(std::ostream& _Output);
    bool IsLoading() const { return __Input != nullptr; }
    bool IsError() const { return __Error; }
    void SetError() { __Error = true; }
    uint64 Remaining() const { return __Remaining; }
    void Serialize(void* _Data, uint64 _Size);
    void UInt32(uint32& _Value);
    void Bytes(std::vector<uint8>& _Data, uint32 _MaximumSize);
    void String(FString& _Value);
    void SetObjectReferences(std::function<uint32(UObject*)> _Save, std::function<UObject*(uint32)> _Load);
    void ObjectReference(UObject*& _Object);
    void SetAssetResolver(std::function<UObject*(const FString&)> _Resolver) { __AssetResolver = std::move(_Resolver); }
    void AssetReference(UObject*& _Object);

private:
    std::istream* __Input = nullptr;
    std::ostream* __Output = nullptr;
    uint64 __Remaining = 0;
    bool __Error = false;
    std::function<uint32(UObject*)> __SaveReference;
    std::function<UObject*(uint32)> __LoadReference;
    std::function<UObject*(const FString&)> __AssetResolver;
};
