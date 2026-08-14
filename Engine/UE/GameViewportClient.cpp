#include "pch.h"
#include "GameViewportClient.h"
#include "Viewport.h"

UGameViewportClient::~UGameViewportClient() = default;

bool UGameViewportClient::Initialize(
	FDynamicRHI& _DynamicRHI,
	void* _WindowHandle,
	uint32 _SizeX,
	uint32 _SizeY)
{
	if (nullptr == _WindowHandle)
		return false;

	__GameViewport = std::make_unique<FSceneViewport>(this, _WindowHandle);
	if (false == __GameViewport->Initialize(_DynamicRHI, _SizeX, _SizeY))
	{
		__GameViewport.reset();
		return false;
	}

	return true;
}

void UGameViewportClient::Deinitialize()
{
	if (__GameViewport)
		__GameViewport->ReleaseRHI();

	__GameViewport.reset();
}
