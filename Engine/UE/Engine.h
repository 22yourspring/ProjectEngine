#pragma once

#include "EngineSystem.h"
#include "Object.h"
#include "Subsystem.h"
#include <atomic>                 
#include <chrono>                 
#include <mutex>
#include <condition_variable>
#include <typeindex>         
#include <type_traits>       

class FEngineLoop;
class UWorld;

UENUM()
enum class ESubsystemInitializeReason
{
	EngineLoading,   
	ExplicitPreload, 
	RuntimeLazyAccess
};

UCLASS(Abstract, Transient, MinimalAPI)
class UEngine : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend class FEngineLoop;

public:
	UEngine();
	virtual ~UEngine() override;

	HRESULT Initialize();
	void Deinitialize();

	template <typename T>
	T* GetSubsystem();

	virtual void Tick(float _DeltaTime) override;
	void RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime);
	UWorld* GetWorld() const { return __CurrentWorld.get(); }

private:
	template <typename T>
	T* CreateSubsystem(); 
	template <typename T>
	T* CreateSubsystemInternal(ESubsystemInitializeReason _Reason);

	template<typename T>
	bool InitializeSubsystem(T* _Subsystem, ESubsystemInitializeReason _Reason);
	bool InitializeSubsystemEntry(std::type_index _TypeIndex, ISubsystem* _Subsystem,
		const char* _TypeName, ESubsystemInitializeReason _Reason);

	void SetGameLoopStarted(bool _bGameLoopStarted);

private:
	struct FSubsystemEntry
	{
		std::unique_ptr<ISubsystem> Instance;
		bool bInitialized = false;
		bool bInitializing = false;
	};

private:
	std::unordered_map<std::type_index, FSubsystemEntry>	__Subsystems;

	std::mutex					__SubsystemMutex;
	std::condition_variable		__SubsystemCondition;
	std::atomic_bool			__bGameLoopStarted = false;
	std::unique_ptr<UWorld>		__CurrentWorld;
};


extern UEngine* GEngine;

#include "Engine.inl"
