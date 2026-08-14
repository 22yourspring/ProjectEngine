#include "pch.h"
#include "Engine.h"
#include "World.h"

UEngine* GEngine = nullptr;

UEngine::UEngine() = default;
UEngine::~UEngine() = default;

HRESULT UEngine::Initialize()
{
	if (nullptr == __CurrentWorld)
		__CurrentWorld = std::make_unique<UWorld>();

	std::vector<std::pair<std::type_index, ISubsystem*>> Subsystems;

	{
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);

		for (auto& SubsystemPair : __Subsystems)
		{
			
			Subsystems.emplace_back(SubsystemPair.first, SubsystemPair.second.Instance.get());
		}
	}

	for (auto& SubsystemPair : Subsystems)
	{
		if (false == InitializeSubsystemEntry(
			SubsystemPair.first, SubsystemPair.second, SubsystemPair.first.name(),
			ESubsystemInitializeReason::EngineLoading))
			return E_FAIL;
		
	}

	return S_OK;
}

void UEngine::Deinitialize()
{
	__CurrentWorld.reset();

	std::vector<std::unique_ptr<ISubsystem>> Subsystems;

	{		
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);

		for (auto& SubsystemPair : __Subsystems)
		{                                                         
			
			if (SubsystemPair.second.Instance)                    
			{                     
				Subsystems.emplace_back(std::move(SubsystemPair.second.Instance));
			}                                                      
		}                                                          
		
		__Subsystems.clear();
	}

	for (std::unique_ptr<ISubsystem>& Subsystem : Subsystems)
	{  
		if (Subsystem)                                       
			Subsystem->Deinitialize();                       
	}
}

void UEngine::Tick(float _DeltaTime)
{	
	std::vector<ISubsystem*> Subsystems;

	{		
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);

		for (auto& SubsystemPair : __Subsystems)
		{
			if (SubsystemPair.second.Instance)
				Subsystems.push_back(SubsystemPair.second.Instance.get());
		}
	}
		
	for (auto Subsystem : Subsystems)
	{
		if (Subsystem->IsTickable())
			Subsystem->Tick(_DeltaTime);
	}

	if (__CurrentWorld)
		__CurrentWorld->Tick(_DeltaTime);
}

void UEngine::RunTickGroup(ETickingGroup _TickGroup, float _DeltaTime)
{
	if (__CurrentWorld)
		__CurrentWorld->RunTickGroup(_TickGroup, _DeltaTime);
}

void UEngine::SetGameLoopStarted(bool _bGameLoopStarted)
{
	__bGameLoopStarted.store(_bGameLoopStarted, std::memory_order_release);
}

bool UEngine::InitializeSubsystemEntry(std::type_index _TypeIndex, ISubsystem* _Subsystem,
	const char* _TypeName, ESubsystemInitializeReason _Reason)
{
	if (nullptr == _Subsystem)
		return false;

	{		
		std::unique_lock<std::mutex> Lock(__SubsystemMutex);
		FSubsystemEntry& Entry = __Subsystems[_TypeIndex];
				
		while (Entry.bInitializing)
			__SubsystemCondition.wait(Lock);
				
		if (Entry.bInitialized)
			return true;
		
		Entry.bInitializing = true;
	}

	const bool bMeasureLazyInitialize =
		ESubsystemInitializeReason::RuntimeLazyAccess == _Reason;

	std::chrono::steady_clock::time_point StartTime;
	if (bMeasureLazyInitialize)
		StartTime = std::chrono::steady_clock::now();

	const int Result = _Subsystem->Initialize();

	std::chrono::steady_clock::time_point EndTime;
	if (bMeasureLazyInitialize)
		EndTime = std::chrono::steady_clock::now();

	{
		std::lock_guard<std::mutex> Lock(__SubsystemMutex);
		FSubsystemEntry& Entry = __Subsystems[_TypeIndex];
		Entry.bInitializing = false;
		Entry.bInitialized = SUCCEEDED(Result);
	}

	__SubsystemCondition.notify_all();

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
				"Consider creating it in FEngineLoop::SubsystemBootstrapper().",
				_TypeName,
				ElapsedTimeMs);

			MessageBoxA(nullptr, Message, "Subsystem Lazy Initialize Warning", MB_OK);
		}
	}

	return SUCCEEDED(Result);
}
