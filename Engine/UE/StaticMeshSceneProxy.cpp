#include "pch.h"
#include "StaticMeshSceneProxy.h"
#include "StaticMesh.h"

FStaticMeshSceneProxy::FStaticMeshSceneProxy(const UStaticMesh& _StaticMesh)
	: __Width(_StaticMesh.GetWidth()),
	  __Height(_StaticMesh.GetHeight()),
	  __Color(_StaticMesh.GetColor())
{
}

void FStaticMeshSceneProxy::Draw(FDynamicRHI& _DynamicRHI) const
{
	const int32 X = static_cast<int32>(__WorldLocation.X);
	const int32 Y = static_cast<int32>(__WorldLocation.Y);
	_DynamicRHI.RHIDrawRectangle(X, Y, X + __Width, Y + __Height, __Color);
}
