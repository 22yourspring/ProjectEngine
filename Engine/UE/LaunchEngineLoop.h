#pragma once

#include "EngineLoop.h"
#include <atomic>
#include <memory>
#include <thread>

class UEngine;
class AppTimeEngineSystem;

class LAUNCH_API FEngineLoop : public IEngineLoop, public TSingleton<FEngineLoop>
{
	friend class TSingleton<FEngineLoop>;
	friend class AppTimeEngineSystem;

private:
	FEngineLoop();
	~FEngineLoop();

public:
	virtual HRESULT Initialize(POINT _Resolution = {}) override;
	HRESULT InitializeForEmbeddedViewport(HWND _WindowHandle, POINT _Resolution);
	virtual void Deinitialize() override;
	HRESULT ResolutionInitialize(POINT _Resolution);
	HRESULT EngineSystemBootstrapper();

	virtual void Progress() override;

	void Thread_Shutdown();

	void Update();
	void Render();

public:
	inline HWND	GetHandle() const { return __Hwnd; }
	inline POINT GetResolution() const { return __Resolution; }
	inline UEngine* GetEngine() const { return __Engine.get(); }
	
protected:
	double	__MaxTickTime;
	float	__DeltaTime = 0.0f;

private:
	HWND						__Hwnd = nullptr;
	POINT						__Resolution = {};

private:
	std::atomic_bool			__ThreadRunning;
	std::atomic_bool			__bInitialized = false;
	std::thread					__UpdateThread;
	std::thread					__RenderThread;
	std::unique_ptr<UEngine>	__Engine;
};
