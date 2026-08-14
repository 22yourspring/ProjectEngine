#pragma once

#include "Subsystem.h"
#include "DynamicRHI.h"

#include <memory>
#include <mutex>
#include <vector>

class UGameViewportClient;
class ISceneViewExtension;

class RenderSubsystem : public ISubsystem
{
public:
    RenderSubsystem();
    virtual ~RenderSubsystem() override;

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
