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

	Proxy->SetWorldLocation(_Primitive->GetWorldLocation());
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
	__PendingCommands.push_back({ ECommandType::Transform, _Primitive, nullptr, _Location });
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
				Iter->second->SetWorldLocation(Command.Location);
		}
	}

	for (const auto& Pair : __PrimitiveProxies)
		Pair.second->Draw(_DynamicRHI);
}
