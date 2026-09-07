#include "pch.h"
#include "RenderEngineSystem.h"
#include "LaunchEngineLoop.h"
#include "Engine.h"
#include "World.h"
#include "Scene.h"
#include "GameViewportClient.h"
#include "Viewport.h"
#include "SceneViewExtension.h"

RenderEngineSystem::RenderEngineSystem() = default;
RenderEngineSystem::~RenderEngineSystem() = default;

HRESULT RenderEngineSystem::Initialize()
{
    __DynamicRHI = PlatformCreateDynamicRHI();
    if (nullptr == __DynamicRHI || false == __DynamicRHI->Init())
        return E_FAIL;

    HWND WindowHandle = FEngineLoop::GetInstance()->GetHandle();
	const POINT Resolution = FEngineLoop::GetInstance()->GetResolution();

	if (nullptr == WindowHandle || Resolution.x <= 0 || Resolution.y <= 0)
        return E_FAIL;

    __GameViewportClient = std::make_unique<UGameViewportClient>();
    if (false == __GameViewportClient->Initialize(
        *__DynamicRHI,
        WindowHandle,
		static_cast<uint32>(Resolution.x),
		static_cast<uint32>(Resolution.y)))
    {
        __GameViewportClient.reset();
        __DynamicRHI->Shutdown();
        __DynamicRHI.reset();
        return E_FAIL;
    }

    return S_OK;
}

void RenderEngineSystem::Deinitialize()
{
	{
		std::lock_guard<std::mutex> Lock(__SceneViewExtensionMutex);
		__SceneViewExtensions.clear();
	}

    if (__DynamicRHI)
    {
        if (__GameViewportClient)
            __GameViewportClient->Deinitialize();

        __GameViewportClient.reset();
        __DynamicRHI->Shutdown();
        __DynamicRHI.reset();
    }
}

void RenderEngineSystem::Tick(float _DeltaTime)
{
}

void RenderEngineSystem::Render()
{
    FSceneViewport* SceneViewport = nullptr != __GameViewportClient
        ? __GameViewportClient->GetGameViewport()
        : nullptr;

	if (nullptr != SceneViewport && nullptr != __DynamicRHI)
	{
		HWND ViewportWindow = static_cast<HWND>(SceneViewport->GetWindowHandle());
		RECT ClientRect = {};
		if (nullptr != ViewportWindow && GetClientRect(ViewportWindow, &ClientRect))
		{
			const uint32 Width = static_cast<uint32>(ClientRect.right - ClientRect.left);
			const uint32 Height = static_cast<uint32>(ClientRect.bottom - ClientRect.top);
			if (Width > 0 && Height > 0 &&
				(Width != SceneViewport->GetSizeX() || Height != SceneViewport->GetSizeY()))
			{
				SceneViewport->ResizeFrame(*__DynamicRHI, Width, Height);
			}
		}
	}

    FRHIViewport* ViewportRHI = nullptr != SceneViewport
        ? SceneViewport->GetViewportRHI()
        : nullptr;

    if (nullptr == __DynamicRHI || nullptr == ViewportRHI ||
        false == __DynamicRHI->RHIBeginDrawingViewport(ViewportRHI, __ClearColor))
        return;

    if (GEngine)
        GEngine->RenderWorld(*__DynamicRHI);

	std::vector<std::shared_ptr<ISceneViewExtension>> SceneViewExtensions;
	{
		std::lock_guard<std::mutex> Lock(__SceneViewExtensionMutex);
		SceneViewExtensions = __SceneViewExtensions;
	}

	for (const std::shared_ptr<ISceneViewExtension>& Extension : SceneViewExtensions)
	{
		if (Extension)
			Extension->Render(*__DynamicRHI);
	}

    __DynamicRHI->RHIEndDrawingViewport(ViewportRHI, true);
}

void RenderEngineSystem::RegisterSceneViewExtension(
	std::shared_ptr<ISceneViewExtension> _Extension)
{
	if (nullptr == _Extension)
		return;

	std::lock_guard<std::mutex> Lock(__SceneViewExtensionMutex);
	__SceneViewExtensions.emplace_back(std::move(_Extension));
}
