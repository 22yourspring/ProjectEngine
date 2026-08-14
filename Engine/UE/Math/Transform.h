// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Include the current implementation of a FTransform, depending on the vector processing mode
 */

#include "UE/CoreTypes.h"
#include "UE/Math/MathFwd.h" // IWYU pragma: export
#include "UE/Math/Quat.h"
#include "UE/Math/ScalarRegister.h"
#include "UE/Math/TransformNonVectorized.h"
#include "UE/Math/TransformVectorized.h"
#include <functional>

namespace UE::Math
{
	/**
	 * Creates a hash value from an FTransform.
	 *
	 * @param Transform the transform to create a hash value for
	 * @return The hash value from the components
	 */
	template<typename T>
	inline uint32 GetTypeHash(const TTransform<T>& Transform)
	{
		auto HashVector = [](const TVector<T>& Vector)
		{
			const std::size_t XHash = std::hash<T>{}(Vector.X);
			const std::size_t YHash = std::hash<T>{}(Vector.Y);
			const std::size_t ZHash = std::hash<T>{}(Vector.Z);
			return XHash ^ (YHash << 1) ^ (ZHash << 2);
		};
		auto Combine = [](std::size_t A, std::size_t B)
		{
			return A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2));
		};

		const TQuat<T>& Rotation = Transform.GetRotation();
		std::size_t RotationHash = std::hash<T>{}(Rotation.X);
		RotationHash = Combine(RotationHash, std::hash<T>{}(Rotation.Y));
		RotationHash = Combine(RotationHash, std::hash<T>{}(Rotation.Z));
		RotationHash = Combine(RotationHash, std::hash<T>{}(Rotation.W));

		std::size_t Hash = HashVector(Transform.GetTranslation());
		Hash = Combine(Hash, RotationHash);
		Hash = Combine(Hash, HashVector(Transform.GetScale3D()));
		return static_cast<uint32>(Hash);
	}
	
}
