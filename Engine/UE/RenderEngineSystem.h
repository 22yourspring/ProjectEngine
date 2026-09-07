#pragma once

#include "EngineSystem.h"
#include "DynamicRHI.h"

#include <memory>
#include <mutex>
#include <vector>

class UGameViewportClient;
class ISceneViewExtension;

UCLASS()
class ENGINE_API RenderEngineSystem : public IEngineSystem
{
	GENERATED_BODY()

public:
    RenderEngineSystem();
    virtual ~RenderEngineSystem() override;

    virtual HRESULT Initialize() override;
    virtual void Deinitialize() override;

    virtual void Tick(float _DeltaTime) override;

    void Render();
	void RegisterSceneViewExtension(std::shared_ptr<ISceneViewExtension> _Extension);

private:
    std::unique_ptr<FDynamicRHI> __DynamicRHI;
    std::unique_ptr<UGameViewportClient> __GameViewportClient;

	std::mutex __SceneViewExtensionMutex;
	std::vector<std::shared_ptr<ISceneViewExtension>> __SceneViewExtensions;
    FColor __ClearColor = { 20, 20, 24, 255 };
};
