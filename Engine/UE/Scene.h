#pragma once

#include "EngineSystem.h"
#include "Math/Vector.h"
#include "PrimitiveSceneProxy.h"
#include <mutex>

class UPrimitiveComponent;
class FDynamicRHI;

class FScene
{
public:
	void AddPrimitive(UPrimitiveComponent* _Primitive);
	void RemovePrimitive(UPrimitiveComponent* _Primitive);
	void UpdatePrimitiveTransform(UPrimitiveComponent* _Primitive, const FVector& _Location);
	void Render(FDynamicRHI& _DynamicRHI);

private:
	enum class ECommandType
	{
		Add,
		Remove,
		Transform
	};

	struct FSceneCommand
	{
		ECommandType Type = ECommandType::Transform;
		const UPrimitiveComponent* Primitive = nullptr;
		std::unique_ptr<FPrimitiveSceneProxy> Proxy;
		FVector Location = {};
	};

	std::mutex __CommandMutex;
	std::vector<FSceneCommand> __PendingCommands;
	std::unordered_map<const UPrimitiveComponent*, std::unique_ptr<FPrimitiveSceneProxy>> __PrimitiveProxies;
};
