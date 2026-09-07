#include "pch.h"
#include "LaunchEngineLoop.h"
#include "GameEngine.h"
#include "RenderEngineSystem.h"
#include "AppTimeEngineSystem.h"
#include "InputEngineSystem.h"
#include "ResourceEngineSystem.h"

FEngineLoop::FEngineLoop() = default;
FEngineLoop::~FEngineLoop() = default;

HRESULT FEngineLoop::Initialize(POINT _Resolution)
{
	return InitializeWithWindow(GetActiveWindow(), _Resolution, false);
}

HRESULT FEngineLoop::InitializeForEmbeddedViewport(HWND _WindowHandle, POINT _Resolution)
{
	return InitializeWithWindow(_WindowHandle, _Resolution, true);
}

HRESULT FEngineLoop::InitializeWithWindow(
	HWND _WindowHandle,
	POINT _Resolution,
	bool _IsEmbeddedViewport)
{
	if (__bInitialized.load(std::memory_order_acquire))
		return S_FALSE;

	__Hwnd = _WindowHandle;
	if (nullptr == __Hwnd)
		return E_HANDLE;

	if (_IsEmbeddedViewport)
	{
		if (_Resolution.x <= 0 || _Resolution.y <= 0)
			return E_INVALIDARG;

		__Resolution = _Resolution;
	}
	else
	{
		if (FAILED(ResolutionInitialize(_Resolution)))
			return E_FAIL;
	}

	__Engine = std::make_unique<UGameEngine>();
	GEngine = __Engine.get();

	auto CleanupFailedEngineInitialization = [this]()
	{
		if (__Engine)
			__Engine->Deinitialize();
		GEngine = nullptr;
		__Engine.reset();
		__Hwnd = nullptr;
		__Resolution = {};
	};

	{
		GEngine->SetGameLoopStarted(false);

		if (FAILED(EngineSystemBootstrapper()))
		{
			CleanupFailedEngineInitialization();
			return E_FAIL;
		}

		if (FAILED(GEngine->Initialize()))
		{
			CleanupFailedEngineInitialization();
			return E_FAIL;
		}

		GEngine->SetGameLoopStarted(true);
	}
	
	{
		__ThreadRunning = true;
		__UpdateThread = std::thread(&FEngineLoop::Update, this);
		__RenderThread = std::thread(&FEngineLoop::Render, this);
	}

	__bInitialized.store(true, std::memory_order_release);

	return S_OK;
}

HRESULT FEngineLoop::ResolutionInitialize(POINT _Resolution)
{
	const bool bBorderlessFullscreen = _Resolution.x <= 0 || _Resolution.y <= 0;

	if (bBorderlessFullscreen)
	{
		const HMONITOR Monitor = MonitorFromWindow(__Hwnd, MONITOR_DEFAULTTONEAREST);

		MONITORINFO MonitorInfo = {};
		MonitorInfo.cbSize = sizeof(MonitorInfo);

		if (!GetMonitorInfo(Monitor, &MonitorInfo))
			return HRESULT_FROM_WIN32(GetLastError());

		SetWindowLongPtr(__Hwnd, GWL_STYLE, WS_POPUP);
		SetMenu(__Hwnd, nullptr);

		const RECT& MonitorRect = MonitorInfo.rcMonitor;
		__Resolution.x = MonitorRect.right - MonitorRect.left;
		__Resolution.y = MonitorRect.bottom - MonitorRect.top;

		if (!SetWindowPos(
			__Hwnd,
			HWND_TOP,
			MonitorRect.left,
			MonitorRect.top,
			__Resolution.x,
			__Resolution.y,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		constexpr DWORD WindowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
		__Resolution = _Resolution;

		SetWindowLongPtr(__Hwnd, GWL_STYLE, WindowStyle);

		RECT WindowRect = { 0, 0, __Resolution.x, __Resolution.y };
		if (!AdjustWindowRect(&WindowRect, WindowStyle, GetMenu(__Hwnd) != nullptr))
			return HRESULT_FROM_WIN32(GetLastError());

		if (!SetWindowPos(
			__Hwnd,
			HWND_TOP,
			0,
			0,
			WindowRect.right - WindowRect.left,
			WindowRect.bottom - WindowRect.top,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	return S_OK;
}

void FEngineLoop::Deinitialize()
{
	if (false == __bInitialized.exchange(false, std::memory_order_acq_rel) && nullptr == __Engine)
		return;

	Thread_Shutdown();

	if (__Engine)
	{
		__Engine->SetGameLoopStarted(false);
		__Engine->Deinitialize();
	}

	GEngine = nullptr;
	__Engine.reset();

	__Hwnd = nullptr;
	__Resolution = {};
}

HRESULT FEngineLoop::EngineSystemBootstrapper()
{
	if (nullptr == GEngine)
		return E_POINTER;

	if (nullptr == GEngine->CreateEngineSystem<AppTimeEngineSystem>())
		return E_FAIL;

	if (nullptr == GEngine->CreateEngineSystem<RenderEngineSystem>())
		return E_FAIL;	

	if (nullptr == GEngine->CreateEngineSystem<InputEngineSystem>())
		return E_FAIL;

	if (nullptr == GEngine->CreateEngineSystem<ResourceEngineSystem>())
		return E_FAIL;

	return S_OK;
}

void FEngineLoop::Progress()
{	
}

void FEngineLoop::Thread_Shutdown()
{	
	__ThreadRunning = false;
	
	if (__UpdateThread.joinable())
		__UpdateThread.join();

	if (__RenderThread.joinable())
		__RenderThread.join();
}

void FEngineLoop::Update()
{
	while (__ThreadRunning)
	{
		__Engine.get()->Tick(__DeltaTime);
		__Engine.get()->RunTickGroup(ETickingGroup::TG_PostUpdateWork, __DeltaTime);
	}	
}

void FEngineLoop::Render()
{
	while (__ThreadRunning)
	{
		if (auto Renderer = __Engine.get()->GetEngineSystem<RenderEngineSystem>())
			Renderer->Render();
	}
}
