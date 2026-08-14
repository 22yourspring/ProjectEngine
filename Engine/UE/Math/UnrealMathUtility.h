#pragma once

#include "UE/CoreTypes.h"
#include "UE/Math/MathFwd.h"

#include <type_traits>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

struct FLinearColor;

#define UE_PI 					(3.1415926535897932f)	/* Extra digits if needed: 3.1415926535897932384626433832795f */
#define UE_SMALL_NUMBER			(1.e-8f)
#define UE_KINDA_SMALL_NUMBER	(1.e-4f)
#define UE_BIG_NUMBER			(3.4e+38f)
#define UE_EULERS_NUMBER		(2.71828182845904523536f)
#define UE_GOLDEN_RATIO			(1.6180339887498948482045868343656381f)	/* Also known as divine proportion, golden mean, or golden section - related to the Fibonacci Sequence = (1 + sqrt(5)) / 2 */
#define UE_FLOAT_NON_FRACTIONAL (8388608.f) /* All single-precision floating point numbers greater than or equal to this have no fractional value. */
#define UE_INV_SQRT_2            (0.70710678118654752440084436210485f)


#define UE_DOUBLE_PI					(3.141592653589793238462643383279502884197169399)
#define UE_DOUBLE_SMALL_NUMBER			(1.e-8)
#define UE_DOUBLE_KINDA_SMALL_NUMBER	(1.e-4)
#define UE_DOUBLE_BIG_NUMBER			(3.4e+38)
#define UE_DOUBLE_EULERS_NUMBER			(2.7182818284590452353602874713526624977572)
#define UE_DOUBLE_GOLDEN_RATIO			(1.6180339887498948482045868343656381)	/* Also known as divine proportion, golden mean, or golden section - related to the Fibonacci Sequence = (1 + sqrt(5)) / 2 */
#define UE_DOUBLE_NON_FRACTIONAL		(4503599627370496.0) /* All double-precision floating point numbers greater than or equal to this have no fractional value. 2^52 */

#define UE_THRESH_VECTOR_NORMALIZED				(0.01f)
#define UE_THRESH_QUAT_NORMALIZED				(0.01f)
#define UE_DOUBLE_THRESH_QUAT_NORMALIZED		(0.01)
#define UE_DELTA								(0.00001f)

#define TEMPLATE_REQUIRES(...) typename std::enable_if<__VA_ARGS__, int>::type = 0

namespace UE::Math
{
	template<typename T> struct TMatrix;
	template<typename T> struct TPlane;
	template<typename T> struct TRotator;
	template<typename T> struct TSphere;
	template<typename T> struct TBox;
	template<typename T> struct TVector;
}
using FMatrix44f = UE::Math::TMatrix<float>;
using FMatrix44d = UE::Math::TMatrix<double>;

template<typename T>
struct TCustomLerp
{
	static constexpr bool Value = false;
};

namespace FMath
{
	template <typename T>
	static FORCEINLINE T Sqrt(T _Value)
	{
		return std::sqrt(_Value);
	}

	template <typename T>
	static FORCEINLINE T InvSqrt(T _Value)
	{
		return T(1) / std::sqrt(_Value);
	}

	template< class T >
	static constexpr FORCEINLINE T Abs(const T A)
	{
		return (A < (T)0) ? -A : A;
	}

	template<class T>
	static constexpr FORCEINLINE const T& Min(const T& A, const T& B)
	{
		return (B < A) ? B : A;
	}

	template<class T>
	static constexpr FORCEINLINE const T& Max(const T& A, const T& B)
	{
		return (A < B) ? B : A;
	}

	template<class T>
	static FORCEINLINE bool IsFinite(T Value)
	{
		return std::isfinite(Value);
	}

	template<class T> static FORCEINLINE T Pow(T A, T B) { return std::pow(A, B); }
	template<class T> static FORCEINLINE T Exp(T A) { return std::exp(A); }
	template<class T> static FORCEINLINE T Exp2(T A) { return std::exp2(A); }
	template<class T> static FORCEINLINE T Loge(T A) { return std::log(A); }
	template<class T> static FORCEINLINE T Log2(T A) { return std::log2(A); }
	template<class T> static FORCEINLINE T Sin(T A) { return std::sin(A); }
	template<class T> static FORCEINLINE T Cos(T A) { return std::cos(A); }
	template<class T> static FORCEINLINE T Tan(T A) { return std::tan(A); }
	template<class T> static FORCEINLINE T Asin(T A) { return std::asin(A); }
	template<class T> static FORCEINLINE T FastAsin(T A) { return std::asin(A); }
	template<class T> static FORCEINLINE T Acos(T A) { return std::acos(A); }
	template<class T> static FORCEINLINE T Atan(T A) { return std::atan(A); }
	template<class T> static FORCEINLINE T Atan2(T Y, T X) { return std::atan2(Y, X); }
	template<class T> static FORCEINLINE T Fmod(T A, T B) { return std::fmod(A, B); }
	template<class T> static FORCEINLINE T InvSqrtEst(T A) { return T(1) / std::sqrt(A); }
	template<class T> static FORCEINLINE T FloorToFloat(T A) { return static_cast<T>(std::floor(A)); }
	template<class T> static FORCEINLINE double FloorToDouble(T A) { return std::floor(static_cast<double>(A)); }
	template<class T> static FORCEINLINE T CeilToFloat(T A) { return static_cast<T>(std::ceil(A)); }
	template<class T> static FORCEINLINE double CeilToDouble(T A) { return std::ceil(static_cast<double>(A)); }
	template<class T> static FORCEINLINE T TruncToFloat(T A) { return static_cast<T>(std::trunc(A)); }
	template<class T> static FORCEINLINE double TruncToDouble(T A) { return std::trunc(static_cast<double>(A)); }
	template<class T> static FORCEINLINE T RoundToFloat(T A) { return static_cast<T>(std::round(A)); }
	template<class T> static FORCEINLINE double RoundToDouble(T A) { return std::round(static_cast<double>(A)); }
	template<class T> static FORCEINLINE T RoundHalfToEven(T A) { return static_cast<T>(std::nearbyint(A)); }
	template<class T> static FORCEINLINE void SinCos(T* OutSin, T* OutCos, T A) { *OutSin = std::sin(A); *OutCos = std::cos(A); }
	template<class T> static constexpr FORCEINLINE T Square(T A) { return A * A; }
	template<class T> static constexpr FORCEINLINE T Sign(T A) { return A < T(0) ? T(-1) : T(1); }
	template<class T> static constexpr FORCEINLINE T Clamp(T X, T A, T B) { return X < A ? A : (X > B ? B : X); }
	template<class T> static FORCEINLINE bool IsNaN(T A) { return std::isnan(A); }
	template<class T> static constexpr FORCEINLINE bool IsNearlyZero(T A, T Tolerance = static_cast<T>(UE_SMALL_NUMBER)) { return Abs(A) <= Tolerance; }
	template<class T> static constexpr FORCEINLINE bool IsNearlyEqual(T A, T B, T Tolerance = static_cast<T>(UE_SMALL_NUMBER)) { return Abs(A - B) <= Tolerance; }
	template<class T> static FORCEINLINE int64 FloorToInt64(T A) { return static_cast<int64>(std::floor(A)); }
	template<class T> static FORCEINLINE int64 CeilToInt64(T A) { return static_cast<int64>(std::ceil(A)); }
	template<class T> static FORCEINLINE int32 FloorToInt32(T A) { return static_cast<int32>(std::floor(A)); }
	template<class T> static FORCEINLINE int32 CeilToInt32(T A) { return static_cast<int32>(std::ceil(A)); }
	template<class T> static FORCEINLINE int32 TruncToInt(T A) { return static_cast<int32>(std::trunc(A)); }
	template<class T> static FORCEINLINE int32 RoundToInt(T A) { return static_cast<int32>(std::round(A)); }
	static FORCEINLINE float FRand() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
	static FORCEINLINE float FRandRange(float InMin, float InMax) { return InMin + (InMax - InMin) * FRand(); }
	static FORCEINLINE double FRandRange(double InMin, double InMax) { return InMin + (InMax - InMin) * static_cast<double>(FRand()); }
	static FORCEINLINE float Modf(float A, float* OutIntegral) { return std::modf(A, OutIntegral); }
	static FORCEINLINE double Modf(double A, double* OutIntegral) { return std::modf(A, OutIntegral); }

	template<class T>
	static FORCEINLINE T UnwindRadians(T A)
	{
		while (A > static_cast<T>(UE_PI))
		{
			A -= static_cast<T>(2) * static_cast<T>(UE_PI);
		}
		while (A < -static_cast<T>(UE_PI))
		{
			A += static_cast<T>(2) * static_cast<T>(UE_PI);
		}
		return A;
	}

	template<class T>
	static constexpr FORCEINLINE T DegreesToRadians(T Degrees)
	{
		return Degrees * static_cast<T>(UE_DOUBLE_PI / 180.0);
	}

	template<class T>
	static constexpr FORCEINLINE T RadiansToDegrees(T Radians)
	{
		return Radians * static_cast<T>(180.0 / UE_DOUBLE_PI);
	}

	template<class T, class U>
	static constexpr FORCEINLINE auto Lerp(const T& A, const T& B, const U& Alpha)
	{
		if constexpr (TCustomLerp<T>::Value)
		{
			return TCustomLerp<T>::Lerp(A, B, Alpha);
		}
		else
		{
			return A + (B - A) * Alpha;
		}
	}

	template<typename T, typename U>
	UE::Math::TRotator<T> LerpRange(
		const UE::Math::TRotator<T>& A,
		const UE::Math::TRotator<T>& B,
		U Alpha);

	template<class T>
	static constexpr FORCEINLINE T FloatSelect(T Comparand, T ValueGEZero, T ValueLTZero)
	{
		return Comparand >= T(0) ? ValueGEZero : ValueLTZero;
	}

	template<class T>
	static constexpr FORCEINLINE T DivideAndRoundDown(T Dividend, T Divisor)
	{
		return Dividend / Divisor;
	}

	template<class T>
	static constexpr FORCEINLINE T DivideAndRoundUp(T Dividend, T Divisor)
	{
		return (Dividend + Divisor - 1) / Divisor;
	}

	template<class T>
	static FORCEINLINE T GridSnap(T Location, T Grid)
	{
		return Grid == T(0) ? Location : std::floor((Location + Grid * T(0.5)) / Grid) * Grid;
	}

	template<class T>
	T ClampAngle(T AngleDegrees, T MinAngleDegrees, T MaxAngleDegrees);

	template<class T>
	static constexpr FORCEINLINE T CubicInterp(
		const T& P0, const T& T0, const T& P1, const T& T1, float Alpha)
	{
		const float A2 = Alpha * Alpha;
		const float A3 = A2 * Alpha;
		return (2.f * A3 - 3.f * A2 + 1.f) * P0
			+ (A3 - 2.f * A2 + Alpha) * T0
			+ (-2.f * A3 + 3.f * A2) * P1
			+ (A3 - A2) * T1;
	}

	template<class T>
	static constexpr FORCEINLINE T CubicInterpDerivative(
		const T& P0, const T& T0, const T& P1, const T& T1, float Alpha)
	{
		const float A2 = Alpha * Alpha;
		return (6.f * A2 - 6.f * Alpha) * P0
			+ (3.f * A2 - 4.f * Alpha + 1.f) * T0
			+ (-6.f * A2 + 6.f * Alpha) * P1
			+ (3.f * A2 - 2.f * Alpha) * T1;
	}

	template<class T>
	static constexpr FORCEINLINE T CubicInterpSecondDerivative(
		const T& P0, const T& T0, const T& P1, const T& T1, float Alpha)
	{
		return (12.f * Alpha - 6.f) * P0
			+ (6.f * Alpha - 4.f) * T0
			+ (-12.f * Alpha + 6.f) * P1
			+ (6.f * Alpha - 2.f) * T1;
	}

	static FORCEINLINE int32 Rand()
	{
		return std::rand();
	}

	bool MatrixInverse(FMatrix44f* DstMatrix, const FMatrix44f* SrcMatrix);
	bool MatrixInverse(FMatrix44d* DstMatrix, const FMatrix44d* SrcMatrix);

	template <typename T>
	UE::Math::TVector<T> ClosestPointOnLine(
		const UE::Math::TVector<T>& LineStart,
		const UE::Math::TVector<T>& LineEnd,
		const UE::Math::TVector<T>& Point);
	FVector ClosestPointOnInfiniteLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);
	bool LineExtentBoxIntersection(
		const FBox& InBox,
		const FVector& Start,
		const FVector& End,
		const FVector& Extent,
		FVector& HitLocation,
		FVector& HitNormal,
		float& HitTime);
	float PointDistToLine(const FVector& Point, const FVector& Direction, const FVector& Origin, FVector& OutClosestPoint);
	float PointDistToLine(const FVector& Point, const FVector& Direction, const FVector& Origin);

	template <typename T>
	UE::Math::TVector<T> ClosestPointOnSegment(
		const UE::Math::TVector<T>& Point,
		const UE::Math::TVector<T>& StartPoint,
		const UE::Math::TVector<T>& EndPoint);
	template <typename T>
	UE::Math::TVector2<T> ClosestPointOnSegment2D(
		const UE::Math::TVector2<T>& Point,
		const UE::Math::TVector2<T>& StartPoint,
		const UE::Math::TVector2<T>& EndPoint);
	float PointDistToSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);
	float PointDistToSegmentSquared(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);

	template<typename T>
	void SegmentDistToSegmentSafe(
		UE::Math::TVector<T> A1,
		UE::Math::TVector<T> B1,
		UE::Math::TVector<T> A2,
		UE::Math::TVector<T> B2,
		UE::Math::TVector<T>& OutP1,
		UE::Math::TVector<T>& OutP2);
	void SegmentDistToSegment(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);
	float GetTForSegmentPlaneIntersect(const FVector& StartPoint, const FVector& EndPoint, const FPlane& Plane);
	bool SegmentPlaneIntersection(const FVector& StartPoint, const FVector& EndPoint, const FPlane& Plane, FVector& OutIntersectionPoint);
	bool SegmentTriangleIntersection(
		const FVector& StartPoint,
		const FVector& EndPoint,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		FVector& OutIntersectPoint,
		FVector& OutTriangleNormal);
	bool SegmentIntersection2D(
		const FVector& SegmentStartA,
		const FVector& SegmentEndA,
		const FVector& SegmentStartB,
		const FVector& SegmentEndB,
		FVector& OutIntersectionPoint);

	uint32 ComputeProjectedSphereScissorRect(
		FIntRect& InOutScissorRect,
		FVector SphereOrigin,
		float Radius,
		FVector ViewOrigin,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjMatrix);
	int32 PlaneAABBRelativePosition(const FPlane& Plane, const FBox& AABB);
	bool PlaneAABBIntersection(const FPlane& Plane, const FBox& AABB);
	bool SphereConeIntersection(
		const FVector& SphereCenter,
		float SphereRadius,
		const FVector& ConeAxis,
		float ConeAngleSin,
		float ConeAngleCos);
	FVector ClosestPointOnTriangleToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);
	FVector GetBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);
	FVector GetBaryCentric2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C);
	bool ComputeBarycentricTri(
		const FVector& Point,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		FVector& OutBarycentric,
		double Tolerance = UE_KINDA_SMALL_NUMBER);
	FVector ComputeBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);
	FVector4 ComputeBaryCentric3D(
		const FVector& Point,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D);
	FVector ClosestPointOnTetrahedronToPoint(
		const FVector& Point,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D);
	void SphereDistToLine(
		FVector SphereOrigin,
		float SphereRadius,
		FVector LineOrigin,
		FVector NormalizedLineDir,
		FVector& OutClosestPoint);
	bool GetDistanceWithinConeSegment(
		FVector Point,
		FVector ConeStartPoint,
		FVector ConeLine,
		float RadiusAtStart,
		float RadiusAtEnd,
		float& PercentageOut);
	bool PointsAreCoplanar(const std::vector<FVector>& Points, float Tolerance = UE_KINDA_SMALL_NUMBER);
	bool GetDotDistance(FVector2D& OutDotDist, const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ);
	FVector2D GetAzimuthAndElevation(const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ);
	FVector VInterpNormalRotationTo(const FVector& Current, const FVector& Target, float DeltaTime, float RotationSpeedDegrees);
	FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);
	FVector VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);
	FVector2D Vector2DInterpConstantTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);
	template <typename T>
	UE::Math::TVector2<T> Vector2DInterpTo(
		const UE::Math::TVector2<T>& Current,
		const UE::Math::TVector2<T>& Target,
		float DeltaTime,
		float InterpSpeed);
	FRotator RInterpConstantTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);
	FRotator RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);
	FLinearColor CInterpTo(const FLinearColor& Current, const FLinearColor& Target, float DeltaTime, float InterpSpeed);
	template<class T>
	UE::Math::TQuat<T> QInterpConstantTo(
		const UE::Math::TQuat<T>& Current,
		const UE::Math::TQuat<T>& Target,
		float DeltaTime,
		float InterpSpeed);
	template<class T>
	UE::Math::TQuat<T> QInterpTo(
		const UE::Math::TQuat<T>& Current,
		const UE::Math::TQuat<T>& Target,
		float DeltaTime,
		float InterpSpeed);
	FVector VRandCone(const FVector& Dir, float ConeHalfAngleRad);
	FVector VRandCone(const FVector& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad);
	FVector2D RandPointInCircle(float CircleRadius);
	FVector RandPointInBox(const FBox& Box);
	FVector GetReflectionVector(const FVector& Direction, const FVector& SurfaceNormal);
	float TruncateToHalfIfClose(float Value, float Tolerance = UE_SMALL_NUMBER);
	double TruncateToHalfIfClose(double Value, double Tolerance = UE_SMALL_NUMBER);
	float RoundHalfToEven(float Value);
	double RoundHalfToEven(double Value);
	float RoundHalfFromZero(float Value);
	double RoundHalfFromZero(double Value);
	float RoundHalfToZero(float Value);
	double RoundHalfToZero(double Value);
	std::string FormatIntToHumanReadable(int32 Value);
	bool Eval(std::string Expression, float& OutValue);
	void WindRelativeAnglesDegrees(float Angle0, float& InOutAngle1);
	void WindRelativeAnglesDegrees(double Angle0, double& InOutAngle1);
	float FixedTurn(float Current, float Desired, float DeltaRate);
	void ApplyScaleToFloat(float& Destination, const FVector& DeltaScale, float Magnitude);
	float PerlinNoise1D(float X);
	float PerlinNoise2D(const FVector2D& Location);
	float PerlinNoise3D(const FVector& Location);

	template<typename T>
	UE::Math::TVector<T> RayPlaneIntersection(
		const UE::Math::TVector<T>& RayOrigin,
		const UE::Math::TVector<T>& RayDirection,
		const UE::Math::TPlane<T>& Plane);

	template<typename T>
	T RayPlaneIntersectionParam(
		const UE::Math::TVector<T>& RayOrigin,
		const UE::Math::TVector<T>& RayDirection,
		const UE::Math::TPlane<T>& Plane);

	template<typename T>
	UE::Math::TVector<T> LinePlaneIntersection(
		const UE::Math::TVector<T>& Point1,
		const UE::Math::TVector<T>& Point2,
		const UE::Math::TPlane<T>& Plane);

	template<typename T>
	bool IntersectPlanes3(
		UE::Math::TVector<T>& I,
		const UE::Math::TPlane<T>& P1,
		const UE::Math::TPlane<T>& P2,
		const UE::Math::TPlane<T>& P3);

	template<typename T>
	bool IntersectPlanes2(
		UE::Math::TVector<T>& I,
		UE::Math::TVector<T>& D,
		const UE::Math::TPlane<T>& P1,
		const UE::Math::TPlane<T>& P2);

	template<typename T>
	UE::Math::TSphere<T> ComputeBoundingSphereForCone(
		const UE::Math::TVector<T>& ConeOrigin,
		const UE::Math::TVector<T>& ConeDirection,
		T ConeRadius,
		T CosConeAngle,
		T SinConeAngle);

	template<typename T>
	bool PointBoxIntersection(
		const UE::Math::TVector<T>& Point,
		const UE::Math::TBox<T>& Box);

	template<typename T>
	bool LineBoxIntersection(
		const UE::Math::TBox<T>& Box,
		const UE::Math::TVector<T>& Start,
		const UE::Math::TVector<T>& End,
		const UE::Math::TVector<T>& StartToEnd);

	template<typename T>
	bool LineBoxIntersection(
		const UE::Math::TBox<T>& Box,
		const UE::Math::TVector<T>& Start,
		const UE::Math::TVector<T>& End,
		const UE::Math::TVector<T>& StartToEnd,
		const UE::Math::TVector<T>& OneOverStartToEnd);

	template<typename T>
	bool SphereAABBIntersection(
		const UE::Math::TVector<T>& SphereCenter,
		T RadiusSquared,
		const UE::Math::TBox<T>& AABB);

	template<typename T>
	bool SphereAABBIntersection(
		const UE::Math::TSphere<T>& Sphere,
		const UE::Math::TBox<T>& AABB);
}
