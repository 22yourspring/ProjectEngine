#pragma once

#include "UE/CoreTypes.h"

struct FLinearColor;
template<typename T> struct TIntVector4;

namespace UE
{
	namespace Math
	{
		template<typename T> struct TVector;
		template<typename T> struct TVector2;
		template<typename T> struct TIntPoint;
		template<typename T> struct TIntRect;
		template<typename T> struct TVector4;
		template<typename T> struct TRotator;
		template<typename T> struct TQuat;
		template<typename T> struct TPlane;
		template<typename T> struct TMatrix;
		template<typename T> struct TBox;
		template<typename T> struct TSphere;
		template<typename T> struct TTransform;
		template<typename T> struct TRotationMatrix;
		template<typename T> struct TRotationTranslationMatrix;
	}
}

using FVector3f = UE::Math::TVector<float>;
using FVector3d = UE::Math::TVector<double>;
using FVector = FVector3d;

using FVector2f = UE::Math::TVector2<float>;
using FVector2d = UE::Math::TVector2<double>;
using FVector2D = FVector2d;

using FInt32Point = UE::Math::TIntPoint<int32>;
using FInt64Point = UE::Math::TIntPoint<int64>;
using FUint32Point = UE::Math::TIntPoint<uint32>;
using FUint64Point = UE::Math::TIntPoint<uint64>;
using FIntPoint = FInt32Point;
using FUintPoint = FUint32Point;

using FInt32Rect = UE::Math::TIntRect<int32>;
using FInt64Rect = UE::Math::TIntRect<int64>;
using FUint32Rect = UE::Math::TIntRect<uint32>;
using FUint64Rect = UE::Math::TIntRect<uint64>;
using FIntRect = FInt32Rect;
using FUintRect = FUint32Rect;

using FVector4f = UE::Math::TVector4<float>;
using FVector4d = UE::Math::TVector4<double>;
using FVector4 = FVector4d;

using FRotator3f = UE::Math::TRotator<float>;
using FRotator3d = UE::Math::TRotator<double>;
using FRotator = FRotator3d;

using FQuat4f = UE::Math::TQuat<float>;
using FQuat4d = UE::Math::TQuat<double>;
using FQuat = FQuat4d;

using FPlane4f = UE::Math::TPlane<float>;
using FPlane4d = UE::Math::TPlane<double>;
using FPlane = FPlane4d;

using FMatrix44f = UE::Math::TMatrix<float>;
using FMatrix44d = UE::Math::TMatrix<double>;
using FMatrix = FMatrix44d;

using FBox3f = UE::Math::TBox<float>;
using FBox3d = UE::Math::TBox<double>;
using FBox = FBox3d;

using FSphere3f = UE::Math::TSphere<float>;
using FSphere3d = UE::Math::TSphere<double>;
using FSphere = FSphere3d;

using FTransform3f = UE::Math::TTransform<float>;
using FTransform3d = UE::Math::TTransform<double>;
using FTransform = FTransform3d;

using FRotationMatrix44f = UE::Math::TRotationMatrix<float>;
using FRotationMatrix44d = UE::Math::TRotationMatrix<double>;
using FRotationMatrix = FRotationMatrix44d;

using FRotationTranslationMatrix44f = UE::Math::TRotationTranslationMatrix<float>;
using FRotationTranslationMatrix44d = UE::Math::TRotationTranslationMatrix<double>;
using FRotationTranslationMatrix = FRotationTranslationMatrix44d;
