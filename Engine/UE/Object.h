#pragma once

#include "EngineMinimal.h"
#include "Class.h"

class UWorld;

UCLASS(Abstract, MinimalAPI)
class COREUOBJECT_API UObject
{
	GENERATED_BODY()

public:
	UObject();
    static UClass* StaticClass() { return UClass::For<UObject>(); }
    bool IsA(const UClass* _Class) const { return _Class && _Class->IsInstance(this); }
    virtual UWorld* GetWorld() const { return __Outer ? __Outer->GetWorld() : nullptr; }
    UObject(const UObject&) = delete;
    UObject& operator=(const UObject&) = delete;
    UObject(UObject&&) noexcept;
    UObject& operator=(UObject&&) noexcept;
	virtual ~UObject();
	const FString& GetName() const { return __ObjectName; }
	UObject* GetOuter() const { return __Outer; }
	FString GetPathName() const;
	virtual void Serialize(class FArchive& _Archive);
	virtual void PostLoad() {}

private:
	friend class FPackageStore;
	friend class UMaterialInstanceDynamic;
	std::vector<std::unique_ptr<UObject>> __OwnedObjects;
	FString __ObjectName;
	UObject* __Outer = nullptr;
	std::vector<FString>	__Tag;

public:
    void AddTag(const FString& _Tag);

	void RemoveTag(const FString& _Tag);
	bool HasTag(const FString& _Tag) const;
};
