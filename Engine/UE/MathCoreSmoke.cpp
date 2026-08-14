#include "pch.h"

#include "UnrealString.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Math/Vector4.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "Math/Plane.h"
#include "Math/Matrix.h"
#include "Math/Color.h"
#include "Math/Transform.h"
#include "Math/Sphere.h"
#include "Math/PolynomialRootSolver.h"

void CompileMathCoreSmokeTest()
{
	const FString Text(TEXT("  ProjectEngine,String,Math  "));
	const FString Trimmed = Text.TrimStartAndEnd();
	const bool ContainsMath = Text.Contains(TEXT("math"));
	const bool StartsWithProject = Trimmed.StartsWith(TEXT("Project"));
	const FString Replaced = Text.Replace(TEXT("String"), TEXT("FString"));
	const FString Left = Trimmed.Left(7);
	const FString Right = Trimmed.Right(4);
	const FString Middle = Trimmed.Mid(2, 6);
	std::vector<FString> Parts;
	const int32 PartCount = Trimmed.ParseIntoArray(Parts, TEXT(","), true);
	const std::vector<TCHAR> Characters = Trimmed.GetCharArray();

	const FVector A(1.0, 0.0, 0.0);
	const FVector B(0.0, 1.0, 0.0);
	const FVector Cross = A ^ B;
	const double Dot = A | B;
	const FQuat Rotation(FVector::UpVector, FMath::DegreesToRadians(90.0));
	const FVector Rotated = Rotation.RotateVector(A);
	const FTransform Transform(Rotation, FVector(10.0, 0.0, 0.0));
	const FVector Position = Transform.TransformPosition(A);
	const FSphere Sphere(Position, 10.0);
	const FLinearColor Color(1.0f, 0.5f, 0.25f, 1.0f);
	const double Coefficients[] = {-1.0, 0.0, 1.0};
	const UE::Math::TPolynomialRootSolver<double, 2> Roots(Coefficients, -2.0, 2.0);

	(void)Cross;
	(void)ContainsMath;
	(void)StartsWithProject;
	(void)Replaced;
	(void)Left;
	(void)Right;
	(void)Middle;
	(void)PartCount;
	(void)Characters;
	(void)Dot;
	(void)Rotated;
	(void)Sphere;
	(void)Color;
	(void)Roots;
}
