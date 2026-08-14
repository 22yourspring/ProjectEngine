#pragma once

#include "SceneComponent.h"

class FPrimitiveSceneProxy;

UCLASS(Abstract, MinimalAPI)
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	virtual ~UPrimitiveComponent() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() const { return nullptr; }

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

protected:
	virtual void OnUpdateTransform() override;
};
