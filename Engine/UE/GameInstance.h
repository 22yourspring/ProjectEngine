#pragma once

#include "GameInstanceSubsystem.h"

#include <memory>
#include <mutex>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <vector>

UCLASS(MinimalAPI)
class ENGINE_API UGameInstance : public UObject
{
	GENERATED_BODY()

public:
	UGameInstance();
	virtual ~UGameInstance() override;

	HRESULT Initialize();
	void Shutdown();

	template<typename T>
	T* GetSubsystem() const
	{
		static_assert(std::is_base_of<UGameInstanceSubsystem, T>::value,
			"T must derive from UGameInstanceSubsystem.");

		std::lock_guard<std::mutex> Lock(__SubsystemMutex);
		const auto Iter = __Subsystems.find(std::type_index(typeid(T)));

		if (__Subsystems.end() == Iter)
			return nullptr;

		return static_cast<T*>(Iter->second.get());
	}

	template<typename T>
	T* CreateSubsystem()
	{
		static_assert(std::is_base_of<UGameInstanceSubsystem, T>::value,
			"T must derive from UGameInstanceSubsystem.");

		const std::type_index TypeIndex(typeid(T));
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);

		if (const auto Iter = __Subsystems.find(TypeIndex);
			Iter != __Subsystems.end())
		{
			return static_cast<T*>(Iter->second.get());
		}

		auto Subsystem = std::make_unique<T>();
		T* SubsystemPointer = Subsystem.get();
		SubsystemPointer->__GameInstance = this;
		__Subsystems.emplace(TypeIndex, std::move(Subsystem));

		if (__bInitialized && FAILED(SubsystemPointer->Initialize()))
		{
			__Subsystems.erase(TypeIndex);
			return nullptr;
		}

		return SubsystemPointer;
	}

private:
	std::unordered_map<std::type_index,
		std::unique_ptr<UGameInstanceSubsystem>> __Subsystems;
	mutable std::mutex __SubsystemMutex;
	bool __bInitialized = false;
};
