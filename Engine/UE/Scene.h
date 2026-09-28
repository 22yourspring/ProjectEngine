#pragma once

#include "EngineMinimal.h"
#include "Math/Vector.h"
#include "Math/Color.h"
#include "PrimitiveSceneProxy.h"
#include <mutex>
#include <chrono>

class UPrimitiveComponent;
class FDynamicRHI;

class FScene
{
public:
	void AddPrimitive(UPrimitiveComponent* _Primitive);
	void RemovePrimitive(UPrimitiveComponent* _Primitive);
	void UpdatePrimitiveTransform(UPrimitiveComponent* _Primitive, const FVector& _Location);
	ENGINE_API void Render(FDynamicRHI& _DynamicRHI);
    void AddDebugLine(const FVector& _Start, const FVector& _End, const FColor& _Color, float _Duration, bool _Persistent);
    void FlushDebugLines();

private:
    struct FDebugLine
    {
        FVector __Start, __End;
        FColor __Color;
        std::chrono::steady_clock::time_point __Expires;
        bool __OneFrame, __Persistent;
    };
    std::vector<FDebugLine> __DebugLines;
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
		FTransform Transform = FTransform::Identity;
	};

	std::mutex __CommandMutex;
	std::vector<FSceneCommand> __PendingCommands;
	std::unordered_map<const UPrimitiveComponent*, std::unique_ptr<FPrimitiveSceneProxy>> __PrimitiveProxies;
};
