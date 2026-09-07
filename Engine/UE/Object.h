#pragma once

#include "EngineMinimal.h"

UCLASS(Abstract, MinimalAPI)
class COREUOBJECT_API UObject
{
	GENERATED_BODY()

public:
	UObject();
	virtual ~UObject();

private:
	std::vector<std::string>	__Tag;

public:
	template<typename T>
	void AddTag(T&& _Tag);

	void RemoveTag(const std::string& _Tag);
	bool HasTag(const std::string& _Tag) const;
};
