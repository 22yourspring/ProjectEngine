#pragma once

#include "Object.h"

class FDynamicRHI;
class FSceneViewport;

class FViewportClient
{
public:
	virtual ~FViewportClient() = default;
};

UCLASS(Within=Engine, Transient, Config=Engine)
class ENGINE_API UGameViewportClient final : public UObject, public FViewportClient
{
	GENERATED_BODY()

public:
	~UGameViewportClient();

	bool Initialize(
		FDynamicRHI& _DynamicRHI,
		void* _WindowHandle,
		uint32 _SizeX,
		uint32 _SizeY);

	void Deinitialize();
	FSceneViewport* GetGameViewport() const { return __GameViewport.get(); }

private:
	std::unique_ptr<FSceneViewport> __GameViewport;
};
