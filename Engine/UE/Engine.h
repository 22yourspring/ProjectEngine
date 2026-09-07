#pragma once

#include "EngineMinimal.h"
#include "Object.h"
#include "NameTypes.h"
#include "EngineSystem.h"
#include <atomic>                 
#include <chrono>                 
#include <mutex>
#include <condition_variable>
#include <typeindex>         
#include <type_traits>       
#include <functional>

class FEngineLoop;
class UWorld;
class UGameInstance;
class AGameModeBase;
class FDynamicRHI;

UENUM()
enum class EEngineSystemInitializeReason
{
	EngineLoading,   
	ExplicitPreload, 
	RuntimeLazyAccess
};

UCLASS(Abstract, Transient, MinimalAPI)
class ENGINE_API UEngine : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class FEngineLoop;

public:
	using FGameModeFactory = std::function<std::unique_ptr<AGameModeBase>()>;
	using FWorldInitializer = std::function<bool(UWorld*)>;
	UEngine();
	virtual ~UEngine() override;

	virtual HRESULT Initialize();
	virtual void Deinitialize();

	template <typename T>
	T* GetEngineSystem();

	virtual void Tick(float _DeltaTime) override;
	virtual void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime);
	virtual bool LoadMap(const FName& _LevelName) { UNREFERENCED_PARAMETER(_LevelName); return false; }
	virtual bool StopPlay() { return false; }
	virtual void RenderWorld(FDynamicRHI& _DynamicRHI) { UNREFERENCED_PARAMETER(_DynamicRHI); }
	virtual UWorld* GetWorld() const { return nullptr; }
	virtual UGameInstance* GetGameInstance() const { return nullptr; }
	virtual void SetDefaultGameModeFactory(FGameModeFactory _Factory)
	{
		UNREFERENCED_PARAMETER(_Factory);
	}
	virtual void SetWorldInitializer(FWorldInitializer _Initializer)
	{
		UNREFERENCED_PARAMETER(_Initializer);
	}

private:
	template <typename T>
	T* CreateEngineSystem(); 
	template <typename T>
	T* CreateEngineSystemInternal(EEngineSystemInitializeReason _Reason);

	template<typename T>
	bool InitializeEngineSystem(T* _EngineSystem, EEngineSystemInitializeReason _Reason);
	bool InitializeEngineSystemEntry(std::type_index _TypeIndex, IEngineSystem* _EngineSystem,
		const char* _TypeName, EEngineSystemInitializeReason _Reason);

	void SetGameLoopStarted(bool _bGameLoopStarted);

private:
	struct FEngineSystemEntry
	{
		std::unique_ptr<IEngineSystem> Instance;
		bool bInitialized = false;
		bool bInitializing = false;
	};

private:
	std::unordered_map<std::type_index, FEngineSystemEntry>	__EngineSystems;

	std::mutex					__EngineSystemMutex;
	std::condition_variable		__EngineSystemCondition;
	std::atomic_bool			__bGameLoopStarted = false;
};


extern ENGINE_API UEngine* GEngine;

#include "Engine.inl"
