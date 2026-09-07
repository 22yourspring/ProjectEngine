#include "pch.h"
#include "Engine.h"
#include "World.h"

UEngine* GEngine = nullptr;

UEngine::UEngine() = default;
UEngine::~UEngine() = default;

HRESULT UEngine::Initialize()
{
	std::vector<std::pair<std::type_index, IEngineSystem*>> EngineSystems;

	{
		std::lock_guard<std::mutex> Lock(__EngineSystemMutex);

		for (auto& EngineSystemPair : __EngineSystems)
		{
			
			EngineSystems.emplace_back(EngineSystemPair.first, EngineSystemPair.second.Instance.get());
		}
	}

	for (auto& EngineSystemPair : EngineSystems)
	{
		if (false == InitializeEngineSystemEntry(
			EngineSystemPair.first, EngineSystemPair.second, EngineSystemPair.first.name(),
			EEngineSystemInitializeReason::EngineLoading))
			return E_FAIL;

		if (UWorld* World = GetWorld())
			World->RegisterEngineSystemTickFunction(EngineSystemPair.second);
	}

	return S_OK;
}

void UEngine::Deinitialize()
{
	std::vector<std::unique_ptr<IEngineSystem>> EngineSystems;

	{		
		std::lock_guard<std::mutex> Lock(__EngineSystemMutex);

		for (auto& EngineSystemPair : __EngineSystems)
		{                                                         
			
			if (EngineSystemPair.second.Instance)                    
			{                     
				EngineSystems.emplace_back(std::move(EngineSystemPair.second.Instance));
			}                                                      
		}                                                          
		
		__EngineSystems.clear();
	}

	for (std::unique_ptr<IEngineSystem>& EngineSystem : EngineSystems)
	{  
		if (EngineSystem)                                       
			EngineSystem->Deinitialize();                       
	}
}

void UEngine::Tick(float _DeltaTime)
{	
	std::vector<IEngineSystem*> EngineSystems;

	{		
		std::lock_guard<std::mutex> Lock(__EngineSystemMutex);

		for (auto& EngineSystemPair : __EngineSystems)
		{
			if (EngineSystemPair.second.Instance)
				EngineSystems.push_back(EngineSystemPair.second.Instance.get());
		}
	}
		
	for (auto EngineSystem : EngineSystems)
	{
		if (EngineSystem->IsTickable() && false == EngineSystem->UsesTickGroup())
			EngineSystem->Tick(_DeltaTime);
	}

}

void UEngine::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
}

void UEngine::SetGameLoopStarted(bool _bGameLoopStarted)
{
	__bGameLoopStarted.store(_bGameLoopStarted, std::memory_order_release);
}

bool UEngine::InitializeEngineSystemEntry(std::type_index _TypeIndex, IEngineSystem* _EngineSystem,
	const char* _TypeName, EEngineSystemInitializeReason _Reason)
{
	if (nullptr == _EngineSystem)
		return false;

	{		
		std::unique_lock<std::mutex> Lock(__EngineSystemMutex);
		FEngineSystemEntry& Entry = __EngineSystems[_TypeIndex];
				
		while (Entry.bInitializing)
			__EngineSystemCondition.wait(Lock);
				
		if (Entry.bInitialized)
			return true;
		
		Entry.bInitializing = true;
	}

	const bool bMeasureLazyInitialize =
		EEngineSystemInitializeReason::RuntimeLazyAccess == _Reason;

	std::chrono::steady_clock::time_point StartTime;
	if (bMeasureLazyInitialize)
		StartTime = std::chrono::steady_clock::now();

	const int Result = _EngineSystem->Initialize();

	std::chrono::steady_clock::time_point EndTime;
	if (bMeasureLazyInitialize)
		EndTime = std::chrono::steady_clock::now();

	{
		std::lock_guard<std::mutex> Lock(__EngineSystemMutex);
		FEngineSystemEntry& Entry = __EngineSystems[_TypeIndex];
		Entry.bInitializing = false;
		Entry.bInitialized = SUCCEEDED(Result);
	}

	if (SUCCEEDED(Result) && GetWorld() && _EngineSystem->UsesTickGroup())
		GetWorld()->RegisterEngineSystemTickFunction(_EngineSystem);

	__EngineSystemCondition.notify_all();

	if (bMeasureLazyInitialize && SUCCEEDED(Result) &&
		__bGameLoopStarted.load(std::memory_order_acquire))
	{
		constexpr long long WarningThresholdMs = 50;
		const auto ElapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			EndTime - StartTime).count();

		if (WarningThresholdMs < ElapsedTimeMs)
		{
			char Message[512] = {};

			sprintf_s(
				Message,
				"%s subsystem was initialized lazily at runtime and took %lld ms.\n"
				"Consider creating it in FEngineLoop::EngineSystemBootstrapper().",
				_TypeName,
				ElapsedTimeMs);

			MessageBoxA(nullptr, Message, "EngineSystem Lazy Initialize Warning", MB_OK);
		}
	}

	return SUCCEEDED(Result);
}
