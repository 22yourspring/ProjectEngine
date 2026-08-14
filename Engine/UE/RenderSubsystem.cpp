#include "pch.h"
#include "RenderSubsystem.h"
#include "LaunchEngineLoop.h"
#include "Engine.h"
#include "World.h"
#include "Scene.h"
#include "GameViewportClient.h"
#include "Viewport.h"
#include "SceneViewExtension.h"

RenderSubsystem::RenderSubsystem() = default;
RenderSubsystem::~RenderSubsystem() = default;

HRESULT RenderSubsystem::Initialize()
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

void RenderSubsystem::Deinitialize()
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

void RenderSubsystem::Tick(float _DeltaTime)
{
}

void RenderSubsystem::Render()
{
    FSceneViewport* SceneViewport = nullptr != __GameViewportClient
        ? __GameViewportClient->GetGameViewport()
        : nullptr;

    FRHIViewport* ViewportRHI = nullptr != SceneViewport
        ? SceneViewport->GetViewportRHI()
        : nullptr;

    if (nullptr == __DynamicRHI || nullptr == ViewportRHI ||
        false == __DynamicRHI->RHIBeginDrawingViewport(ViewportRHI, __ClearColor))
        return;

    if (UWorld* World = GEngine->GetWorld())
        World->GetScene()->Render(*__DynamicRHI);

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

void RenderSubsystem::RegisterSceneViewExtension(
	std::shared_ptr<ISceneViewExtension> _Extension)
{
	if (nullptr == _Extension)
		return;

	std::lock_guard<std::mutex> Lock(__SceneViewExtensionMutex);
	__SceneViewExtensions.emplace_back(std::move(_Extension));
}
