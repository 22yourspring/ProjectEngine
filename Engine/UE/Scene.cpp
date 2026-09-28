#include "pch.h"
#include "Scene.h"
#include "PrimitiveComponent.h"
#include "DynamicRHI.h"

void FScene::AddPrimitive(UPrimitiveComponent* _Primitive)
{
	if (nullptr == _Primitive)
		return;

	std::unique_ptr<FPrimitiveSceneProxy> Proxy(_Primitive->CreateSceneProxy());
	if (nullptr == Proxy)
		return;

	Proxy->SetTransform(_Primitive->GetComponentTransform());
	std::lock_guard<std::mutex> Lock(__CommandMutex);
	__PendingCommands.push_back({ ECommandType::Add, _Primitive, std::move(Proxy), {} });
}

void FScene::RemovePrimitive(UPrimitiveComponent* _Primitive)
{
	if (nullptr == _Primitive)
		return;

	std::lock_guard<std::mutex> Lock(__CommandMutex);
	__PendingCommands.push_back({ ECommandType::Remove, _Primitive, nullptr, {} });
}

void FScene::UpdatePrimitiveTransform(UPrimitiveComponent* _Primitive, const FVector& _Location)
{
	if (nullptr == _Primitive)
		return;

	std::lock_guard<std::mutex> Lock(__CommandMutex);
	__PendingCommands.push_back({ ECommandType::Transform, _Primitive, nullptr, _Primitive->GetComponentTransform() });
}

void FScene::Render(FDynamicRHI& _DynamicRHI)
{
	std::vector<FSceneCommand> Commands;
	{
		std::lock_guard<std::mutex> Lock(__CommandMutex);
		Commands = std::move(__PendingCommands);
		__PendingCommands.clear();
	}

	for (FSceneCommand& Command : Commands)
	{
		if (ECommandType::Add == Command.Type)
			__PrimitiveProxies[Command.Primitive] = std::move(Command.Proxy);
		else if (ECommandType::Remove == Command.Type)
			__PrimitiveProxies.erase(Command.Primitive);
		else
		{
			auto Iter = __PrimitiveProxies.find(Command.Primitive);
			if (__PrimitiveProxies.end() != Iter)
				Iter->second->SetTransform(Command.Transform);
		}
	}

	for (const auto& Pair : __PrimitiveProxies)
		Pair.second->Draw(_DynamicRHI);
    std::vector<FDebugLine> Lines;
    {
        std::lock_guard<std::mutex> Lock(__CommandMutex);
        const auto Now = std::chrono::steady_clock::now();
        std::erase_if(__DebugLines, [&](const auto& _Line) { return !_Line.__Persistent && !_Line.__OneFrame && _Line.__Expires <= Now; });
        Lines = __DebugLines;
        std::erase_if(__DebugLines, [](const auto& _Line) { return _Line.__OneFrame; });
    }
    for (const auto& Line : Lines)
        _DynamicRHI.RHIDrawLine(int32(Line.__Start.X), int32(Line.__Start.Y), int32(Line.__End.X), int32(Line.__End.Y), Line.__Color);
}

void FScene::AddDebugLine(const FVector& _Start, const FVector& _End, const FColor& _Color, float _Duration, bool _Persistent)
{
    if (_Start.ContainsNaN() || _End.ContainsNaN()) return;
    const auto Expires = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>((std::max)(0.f, _Duration)));
    std::lock_guard<std::mutex> Lock(__CommandMutex);
    __DebugLines.push_back({_Start, _End, _Color, Expires, _Duration <= 0 && !_Persistent, _Persistent});
}

void FScene::FlushDebugLines()
{
    std::lock_guard<std::mutex> Lock(__CommandMutex);
    __DebugLines.clear();
}
