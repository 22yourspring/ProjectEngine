#pragma once

#include "UE/CoreTypes.h"
#include "UE/Math/MathFwd.h"
#include "UE/Math/UnrealMathUtility.h"
#include "UE/StringUtility.h"

#include <limits>

namespace UE
{
    namespace Math
    {
        template<typename T> struct TPlane;
        template<typename T> struct TVector4;

        template<typename T>
        struct TVector
        {
        public:
            using FReal = T;

            union
            {
                struct
                {
                    /** Vector's X component. */
                    T X;

                    /** Vector's Y component. */
                    T Y;

                    /** Vector's Z component. */
                    T Z;
                };
                T XYZ[3];
            };

            /** A zero vector (0,0,0) */
            CORE_API static const TVector<T> ZeroVector;

            /** One vector (1,1,1) */
            CORE_API static const TVector<T> OneVector;

            /** Unreal up vector (0,0,1) */
            CORE_API static const TVector<T> UpVector;

            /** Unreal down vector (0,0,-1) */
            CORE_API static const TVector<T> DownVector;

            /** Unreal forward vector (1,0,0) */
            CORE_API static const TVector<T> ForwardVector;

            /** Unreal backward vector (-1,0,0) */
            CORE_API static const TVector<T> BackwardVector;

            /** Unreal right vector (0,1,0) */
            CORE_API static const TVector<T> RightVector;

            /** Unreal left vector (0,-1,0) */
            CORE_API static const TVector<T> LeftVector;

            /** Unit X axis vector (1,0,0) */
            CORE_API static const TVector<T> XAxisVector;

            /** Unit Y axis vector (0,1,0) */
            CORE_API static const TVector<T> YAxisVector;

            /** Unit Z axis vector (0,0,1) */
            CORE_API static const TVector<T> ZAxisVector;

            /** @return Zero Vector (0,0,0) */
            static inline TVector<T> Zero() { return ZeroVector; }

            /** @return One Vector (1,1,1) */
            static inline TVector<T> One() { return OneVector; }

            /** @return Unit X Vector (1,0,0)  */
            static inline TVector<T> UnitX() { return XAxisVector; }

            /** @return Unit Y Vector (0,1,0)  */
            static inline TVector<T> UnitY() { return YAxisVector; }

            /** @return Unit Z Vector (0,0,1)  */
            static inline TVector<T> UnitZ() { return ZAxisVector; }

        public:
            /** Default constructor (no initialization). */
            FORCEINLINE TVector();

            /**
             * Constructor initializing all components to a single T value.
             *
             * @param InF Value to set all components to.
             */
            explicit FORCEINLINE TVector(T InF);

            /**
             * Constructor using initial values for each component.
             *
             * @param InX X Coordinate.
             * @param InY Y Coordinate.
             * @param InZ Z Coordinate.
             */
            FORCEINLINE TVector(T InX, T InY, T InZ);

            /**
             * Calculate cross product between this and another vector.
             *
             * @param V The other vector.
             * @return The cross product.
             */
            FORCEINLINE TVector<T> operator^(const TVector<T>& V) const;

            /**
             * Calculate cross product between this and another vector.
             *
             * @param V The other vector.
             * @return The cross product.
             */
            FORCEINLINE TVector<T> Cross(const TVector<T>& V2) const;

            /**
             * Calculate the cross product of two vectors.
             *
             * @param A The first vector.
             * @param B The second vector.
             * @return The cross product.
             */
            FORCEINLINE static TVector<T> CrossProduct(const TVector<T>& A, const TVector<T>& B);

            /**
             * Calculate the dot product between this and another vector.
             *
             * @param V The other vector.
             * @return The dot product.
             */
            FORCEINLINE T operator|(const TVector<T>& V) const;

            /**
             * Calculate the dot product between this and another vector.
             *
             * @param V The other vector.
             * @return The dot product.
             */
            FORCEINLINE T Dot(const TVector<T>& V) const;

            /**
             * Calculate the dot product of two vectors.
             *
             * @param A The first vector.
             * @param B The second vector.
             * @return The dot product.
             */
            FORCEINLINE static T DotProduct(const TVector<T>& A, const TVector<T>& B);

            /**
             * Gets the result of component-wise addition of this and another vector.
             *
             * @param V The vector to add to this.
             * @return The result of vector addition.
             */
            FORCEINLINE TVector<T> operator+(const TVector<T>& V) const;

            /**
             * Gets the result of component-wise subtraction of this by another vector.
             *
             * @param V The vector to subtract from this.
             * @return The result of vector subtraction.
             */
            FORCEINLINE TVector<T> operator-(const TVector<T>& V) const;

            /**
             * Gets the result of subtracting from each component of the vector.
             *
             * @param Bias How much to subtract from each component.
             * @return The result of subtraction.
             */
            template<typename FArg, TEMPLATE_REQUIRES(std::is_arithmetic<FArg>::value)>
            FORCEINLINE TVector<T> operator-(FArg Bias) const
            {
                return TVector<T>(X - (T)Bias, Y - (T)Bias, Z - (T)Bias);
            }

            /**
             * Gets the result of adding to each component of the vector.
             *
             * @param Bias How much to add to each component.
             * @return The result of addition.
             */
            template<typename FArg, TEMPLATE_REQUIRES(std::is_arithmetic<FArg>::value)>
            FORCEINLINE TVector<T> operator+(FArg Bias) const
            {
                return TVector<T>(X + (T)Bias, Y + (T)Bias, Z + (T)Bias);
            }

            /**
             * Gets the result of scaling the vector (multiplying each component by a value).
             *
             * @param Scale What to multiply each component by.
             * @return The result of multiplication.
             */
            template<typename FArg, TEMPLATE_REQUIRES(std::is_arithmetic<FArg>::value)>
            FORCEINLINE TVector<T> operator*(FArg Scale) const
            {
                return TVector<T>(X * (T)Scale, Y * (T)Scale, Z * (T)Scale);
            }

            /**
             * Gets the result of dividing each component of the vector by a value.
             *
             * @param Scale What to divide each component by.
             * @return The result of division.
             */
            template<typename FArg, TEMPLATE_REQUIRES(std::is_arithmetic<FArg>::value)>
            TVector<T> operator/(FArg Scale) const
            {
                const T RScale = T(1) / Scale;
                return TVector<T>(X * RScale, Y * RScale, Z * RScale);
            }

            /**
             * Gets the result of component-wise multiplication of this vector by another.
             *
             * @param V The vector to multiply with.
             * @return The result of multiplication.
             */
            FORCEINLINE TVector<T> operator*(const TVector<T>& V) const;

            /**
             * Gets the result of component-wise division of this vector by another.
             *
             * @param V The vector to divide by.
             * @return The result of division.
             */
            FORCEINLINE TVector<T> operator/(const TVector<T>& V) const;

            // Binary comparison operators.

            /**
             * Check against another vector for equality.
             *
             * @param V The vector to check against.
             * @return true if the vectors are equal, false otherwise.
             */
            bool operator==(const TVector<T>& V) const;

            /**
             * Check against another vector for inequality.
             *
             * @param V The vector to check against.
             * @return true if the vectors are not equal, false otherwise.
             */
            bool operator!=(const TVector<T>& V) const;

            /**
             * Get a negated copy of the vector.
             *
             * @return A negated copy of the vector.
             */
            FORCEINLINE TVector<T> operator-() const;

            /**
             * Adds another vector to this.
             * Uses component-wise addition.
             *
             * @param V Vector to add to this.
             * @return Copy of the vector after addition.
             */
            FORCEINLINE TVector<T> operator+=(const TVector<T>& V);

            /**
             * Subtracts another vector from this.
             * Uses component-wise subtraction.
             *
             * @param V Vector to subtract from this.
             * @return Copy of the vector after subtraction.
             */
            FORCEINLINE TVector<T> operator-=(const TVector<T>& V);

            /**
             * Scales the vector.
             *
             * @param Scale Amount to scale this vector by.
             * @return Copy of the vector after scaling.
             */
            template<typename FArg, TEMPLATE_REQUIRES(std::is_arithmetic<FArg>::value)>
            FORCEINLINE TVector<T> operator*=(FArg Scale)
            {
                X *= Scale; Y *= Scale; Z *= Scale;
                return *this;
            }

            /**
             * Divides the vector by a number.
             *
             * @param V What to divide this vector by.
             * @return Copy of the vector after division.
             */
            template<typename FArg, TEMPLATE_REQUIRES(std::is_arithmetic<FArg>::value)>
            TVector<T> operator/=(FArg Scale)
            {
                const T RV = (T)1 / Scale;
                X *= RV; Y *= RV; Z *= RV;
                return *this;
            }

            /**
             * Multiplies the vector with another vector, using component-wise multiplication.
             *
             * @param V What to multiply this vector with.
             * @return Copy of the vector after multiplication.
             */
            TVector<T> operator*=(const TVector<T>& V);

            /**
             * Divides the vector by another vector, using component-wise division.
             *
             * @param V What to divide vector by.
             * @return Copy of the vector after division.
             */
            TVector<T> operator/=(const TVector<T>& V);

            /**
             * Gets specific component of the vector.
             *
             * @param Index the index of vector component
             * @return reference to component.
             */
            T& operator[](int32 Index);

            /**
             * Gets specific component of the vector.
             *
             * @param Index the index of vector component
             * @return Copy of the component.
             */
            T operator[](int32 Index)const;

            /**
            * Gets a specific component of the vector.
            *
            * @param Index The index of the component required.
            *
            * @return Reference to the specified component.
            */
            T& Component(int32 Index);

            /**
            * Gets a specific component of the vector.
            *
            * @param Index The index of the component required.
            * @return Copy of the specified component.
            */
            T Component(int32 Index) const;

        public:

            // Simple functions.

            /**
             * Set the values of the vector directly.
             *
             * @param InX New X coordinate.
             * @param InY New Y coordinate.
             * @param InZ New Z coordinate.
             */
            void Set(T InX, T InY, T InZ);

            [[nodiscard]] bool Equals(
                const TVector<T>& _Vector,
                T _Tolerance = static_cast<T>(UE_KINDA_SMALL_NUMBER)) const;

            [[nodiscard]] bool AllComponentsEqual(
                T _Tolerance = static_cast<T>(UE_KINDA_SMALL_NUMBER)) const;

            [[nodiscard]] bool IsValidIndex(int32 _Index) const;

            [[nodiscard]] T GetMax() const;
            [[nodiscard]] T GetAbsMax() const;
            [[nodiscard]] T GetMin() const;
            [[nodiscard]] T GetAbsMin() const;

            [[nodiscard]] TVector<T> ComponentMin(const TVector<T>& _Other) const;
            [[nodiscard]] TVector<T> ComponentMax(const TVector<T>& _Other) const;

            /**
             * Get a copy of this vector with absolute value of each component.
             *
             * @return A copy of this vector with absolute value of each component.
             */
            TVector<T> GetAbs() const;

            /**
             * Get the length (magnitude) of this vector.
             *
             * @return The length of this vector.
             */
            T Size() const;

            /**
             * Get the length (magnitude) of this vector.
             *
             * @return The length of this vector.
             */
            T Length() const;

            /**
             * Get the squared length of this vector.
             *
             * @return The squared length of this vector.
             */
            T SizeSquared() const;

            [[nodiscard]] T SquaredLength() const;
            [[nodiscard]] T Size2D() const;
            [[nodiscard]] T SizeSquared2D() const;

            [[nodiscard]] bool IsNearlyZero(
                T _Tolerance = static_cast<T>(UE_KINDA_SMALL_NUMBER)) const;

            [[nodiscard]] bool IsZero() const;
            [[nodiscard]] bool IsUnit(
                T _LengthSquaredTolerance = static_cast<T>(UE_KINDA_SMALL_NUMBER)) const;

            /**
             * Checks whether vector is normalized.
             *
             * @return true if normalized, false otherwise.
             */
            bool IsNormalized() const;

            /**
             * Normalize this vector in-place if it is larger than a given tolerance. Leaves it unchanged if not.
             *
             * @param Tolerance Minimum squared length of vector for normalization.
             * @return true if the vector was normalized correctly, false otherwise.
             */
            bool Normalize(T Tolerance = UE_SMALL_NUMBER);

            [[nodiscard]] TVector<T> GetSafeNormal(
                T _Tolerance = static_cast<T>(UE_SMALL_NUMBER),
                const TVector<T>& _ResultIfZero = ZeroVector) const;

            [[nodiscard]] TVector<T> GetUnsafeNormal() const;

            [[nodiscard]] TVector<T> GetSafeNormal2D(
                T _Tolerance = static_cast<T>(UE_SMALL_NUMBER),
                const TVector<T>& _ResultIfZero = ZeroVector) const;

            [[nodiscard]] TVector<T> GetUnsafeNormal2D() const;

            [[nodiscard]] TVector<T> Reciprocal() const;
            [[nodiscard]] bool IsUniform(
                T _Tolerance = static_cast<T>(UE_KINDA_SMALL_NUMBER)) const;
            [[nodiscard]] TVector<T> MirrorByVector(const TVector<T>& _MirrorNormal) const;
            [[nodiscard]] TVector<T> RotateAngleAxis(T _AngleDeg, const TVector<T>& _Axis) const;
            [[nodiscard]] TVector<T> RotateAngleAxisRad(T _AngleRad, const TVector<T>& _Axis) const;
            [[nodiscard]] TVector<T> ProjectOnTo(const TVector<T>& _Vector) const;
            [[nodiscard]] TVector<T> ProjectOnToNormal(const TVector<T>& _Normal) const;
            [[nodiscard]] bool ContainsNaN() const;

            [[nodiscard]] CORE_API TRotator<T> ToOrientationRotator() const;
            [[nodiscard]] CORE_API TQuat<T> ToOrientationQuat() const;
            [[nodiscard]] TRotator<T> Rotation() const
            {
                return ToOrientationRotator();
            }

            [[nodiscard]] static CORE_API TVector<T> SlerpVectorToDirection(
                const TVector<T>& _Vector,
                const TVector<T>& _Direction,
                T _Alpha);
            [[nodiscard]] static CORE_API TVector<T> SlerpNormals(
                const TVector<T>& _NormalA,
                const TVector<T>& _NormalB,
                T _Alpha);

            [[nodiscard]] std::string ToString() const;
            bool InitFromString(std::string_view _Source);

            [[nodiscard]] static T Dist(const TVector<T>& _Left, const TVector<T>& _Right);
            [[nodiscard]] static T Distance(const TVector<T>& _Left, const TVector<T>& _Right)
            {
                return Dist(_Left, _Right);
            }

            [[nodiscard]] static T DistSquared(const TVector<T>& _Left, const TVector<T>& _Right);
            [[nodiscard]] static T Dist2D(const TVector<T>& _Left, const TVector<T>& _Right);
            [[nodiscard]] static T DistSquared2D(const TVector<T>& _Left, const TVector<T>& _Right);
            [[nodiscard]] static T Triple(
                const TVector<T>& _X,
                const TVector<T>& _Y,
                const TVector<T>& _Z);

            /**
             * Converts a vector containing radian values to a vector containing degree values.
             *
             * @param RadVector	Vector containing radian values
             * @return Vector  containing degree values
             */
            static TVector<T> RadiansToDegrees(const TVector<T>& RadVector);

            /**
             * Converts a vector containing degree values to a vector containing radian values.
             *
             * @param DegVector	Vector containing degree values
             * @return Vector containing radian values
             */
            static TVector<T> DegreesToRadians(const TVector<T>& DegVector);

            // Conversion from other type.
            template<typename FArg, TEMPLATE_REQUIRES(!std::is_same_v<T, FArg>)>
            explicit TVector(const TVector<FArg>& From) : TVector<T>((T)From.X, (T)From.Y, (T)From.Z) {}

            TVector(const TVector4<T>& From);

            [[nodiscard]] TVector<T> MirrorByPlane(const TPlane<T>& Plane) const;
            [[nodiscard]] static T PointPlaneDist(const TVector<T>& Point, const TVector<T>& PlaneBase, const TVector<T>& PlaneNormal);
            [[nodiscard]] static TVector<T> PointPlaneProject(const TVector<T>& Point, const TPlane<T>& Plane);
            [[nodiscard]] static TVector<T> PointPlaneProject(const TVector<T>& Point, const TVector<T>& A, const TVector<T>& B, const TVector<T>& C);
        };
    }
}

namespace UE::Math
{
    template<typename FArg, typename T, TEMPLATE_REQUIRES(std::is_arithmetic_v<FArg>)>
    FORCEINLINE TVector<T> operator*(FArg Scale, const TVector<T>& Vector)
    {
        return Vector * Scale;
    }
}

namespace UE
{
    namespace Math
    {
        /* FVector inline functions
         *****************************************************************************/

        template<typename T>
        inline TVector<T> TVector<T>::RadiansToDegrees(const TVector<T>& RadVector)
        {
            return RadVector * (180.f / UE_PI);
        }

        template<typename T>
        inline TVector<T> TVector<T>::DegreesToRadians(const TVector<T>& DegVector)
        {
            return DegVector * (UE_PI / 180.f);
        }

        template<typename T>
        FORCEINLINE TVector<T>::TVector()
        {
        }

        template<typename T>
        FORCEINLINE TVector<T>::TVector(T InF)
            : X(InF), Y(InF), Z(InF)
        {
        }

        template<typename T>
        FORCEINLINE TVector<T>::TVector(T InX, T InY, T InZ)
            : X(InX), Y(InY), Z(InZ)
        {
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator^(const TVector<T>& V) const
        {
            return TVector<T>
                (
                    Y * V.Z - Z * V.Y,
                    Z * V.X - X * V.Z,
                    X * V.Y - Y * V.X
                );
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::Cross(const TVector<T>& V) const
        {
            return *this ^ V;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::CrossProduct(const TVector<T>& A, const TVector<T>& B)
        {
            return A ^ B;
        }

        template<typename T>
        FORCEINLINE T TVector<T>::operator|(const TVector<T>& V) const
        {
            return X * V.X + Y * V.Y + Z * V.Z;
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Dot(const TVector<T>& V) const
        {
            return *this | V;
        }

        template<typename T>
        FORCEINLINE T TVector<T>::DotProduct(const TVector<T>& A, const TVector<T>& B)
        {
            return A | B;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator+(const TVector<T>& V) const
        {
            return TVector<T>(X + V.X, Y + V.Y, Z + V.Z);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator-(const TVector<T>& V) const
        {
            return TVector<T>(X - V.X, Y - V.Y, Z - V.Z);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator*(const TVector<T>& V) const
        {
            return TVector<T>(X * V.X, Y * V.Y, Z * V.Z);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator/(const TVector<T>& V) const
        {
            return TVector<T>(X / V.X, Y / V.Y, Z / V.Z);
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::operator==(const TVector<T>& V) const
        {
            return X == V.X && Y == V.Y && Z == V.Z;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::operator!=(const TVector<T>& V) const
        {
            return X != V.X || Y != V.Y || Z != V.Z;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator-() const
        {
            return TVector<T>(-X, -Y, -Z);
        }


        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator+=(const TVector<T>& V)
        {
            X += V.X; Y += V.Y; Z += V.Z;

            return *this;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator-=(const TVector<T>& V)
        {
            X -= V.X; Y -= V.Y; Z -= V.Z;

            return *this;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator*=(const TVector<T>& V)
        {
            X *= V.X; Y *= V.Y; Z *= V.Z;

            return *this;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::operator/=(const TVector<T>& V)
        {
            X /= V.X; Y /= V.Y; Z /= V.Z;

            return *this;
        }

        template<typename T>
        FORCEINLINE T& TVector<T>::operator[](int32 Index)
        {
            return XYZ[Index];
        }

        template<typename T>
        FORCEINLINE T TVector<T>::operator[](int32 Index) const
        {
            return XYZ[Index];
        }

        template<typename T>
        FORCEINLINE void TVector<T>::Set(T InX, T InY, T InZ)
        {
            X = InX;
            Y = InY;
            Z = InZ;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::Equals(
            const TVector<T>& _Vector,
            T _Tolerance) const
        {
            return FMath::Abs(X - _Vector.X) <= _Tolerance
                && FMath::Abs(Y - _Vector.Y) <= _Tolerance
                && FMath::Abs(Z - _Vector.Z) <= _Tolerance;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::AllComponentsEqual(T _Tolerance) const
        {
            return FMath::Abs(X - Y) <= _Tolerance
                && FMath::Abs(Y - Z) <= _Tolerance
                && FMath::Abs(X - Z) <= _Tolerance;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::IsValidIndex(int32 _Index) const
        {
            return _Index >= 0 && _Index < 3;
        }

        template<typename T>
        FORCEINLINE T TVector<T>::GetMax() const
        {
            return FMath::Max(FMath::Max(X, Y), Z);
        }

        template<typename T>
        FORCEINLINE T TVector<T>::GetAbsMax() const
        {
            return FMath::Max(FMath::Max(FMath::Abs(X), FMath::Abs(Y)), FMath::Abs(Z));
        }

        template<typename T>
        FORCEINLINE T TVector<T>::GetMin() const
        {
            return FMath::Min(FMath::Min(X, Y), Z);
        }

        template<typename T>
        FORCEINLINE T TVector<T>::GetAbsMin() const
        {
            return FMath::Min(FMath::Min(FMath::Abs(X), FMath::Abs(Y)), FMath::Abs(Z));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::ComponentMin(const TVector<T>& _Other) const
        {
            return TVector<T>(
                FMath::Min(X, _Other.X),
                FMath::Min(Y, _Other.Y),
                FMath::Min(Z, _Other.Z));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::ComponentMax(const TVector<T>& _Other) const
        {
            return TVector<T>(
                FMath::Max(X, _Other.X),
                FMath::Max(Y, _Other.Y),
                FMath::Max(Z, _Other.Z));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::GetAbs() const
        {
            return TVector<T>(FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z));
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Size() const
        {
            return FMath::Sqrt(X * X + Y * Y + Z * Z);
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Length() const
        {
            return Size();
        }

        template<typename T>
        FORCEINLINE T TVector<T>::SizeSquared() const
        {
            return X * X + Y * Y + Z * Z;
        }

        template<typename T>
        FORCEINLINE T TVector<T>::SquaredLength() const
        {
            return SizeSquared();
        }

        template<typename T>
        FORCEINLINE T TVector<T>::PointPlaneDist(
            const TVector<T>& Point,
            const TVector<T>& PlaneBase,
            const TVector<T>& PlaneNormal)
        {
            return (Point - PlaneBase) | PlaneNormal;
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Size2D() const
        {
            return FMath::Sqrt(X * X + Y * Y);
        }

        template<typename T>
        FORCEINLINE T TVector<T>::SizeSquared2D() const
        {
            return X * X + Y * Y;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::IsNearlyZero(T _Tolerance) const
        {
            return FMath::Abs(X) <= _Tolerance
                && FMath::Abs(Y) <= _Tolerance
                && FMath::Abs(Z) <= _Tolerance;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::IsZero() const
        {
            return X == static_cast<T>(0)
                && Y == static_cast<T>(0)
                && Z == static_cast<T>(0);
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::IsUnit(T _LengthSquaredTolerance) const
        {
            return FMath::Abs(static_cast<T>(1) - SizeSquared()) < _LengthSquaredTolerance;
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::Normalize(T Tolerance)
        {
            const T SquareSum = X * X + Y * Y + Z * Z;
            if (SquareSum > Tolerance)
            {
                const T Scale = FMath::InvSqrt(SquareSum);
                X *= Scale; Y *= Scale; Z *= Scale;
                return true;
            }
            return false;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::GetSafeNormal(
            T _Tolerance,
            const TVector<T>& _ResultIfZero) const
        {
            const T SquareSum = SizeSquared();
            if (SquareSum == static_cast<T>(1))
            {
                return *this;
            }
            if (SquareSum < _Tolerance)
            {
                return _ResultIfZero;
            }
            return *this * FMath::InvSqrt(SquareSum);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::GetUnsafeNormal() const
        {
            const T Scale = FMath::InvSqrt(X * X + Y * Y + Z * Z);
            return TVector<T>(X * Scale, Y * Scale, Z * Scale);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::GetSafeNormal2D(
            T _Tolerance,
            const TVector<T>& _ResultIfZero) const
        {
            const T SquareSum = SizeSquared2D();
            if (SquareSum == static_cast<T>(1))
            {
                return TVector<T>(X, Y, static_cast<T>(0));
            }
            if (SquareSum < _Tolerance)
            {
                return _ResultIfZero;
            }
            const T Scale = FMath::InvSqrt(SquareSum);
            return TVector<T>(X * Scale, Y * Scale, static_cast<T>(0));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::GetUnsafeNormal2D() const
        {
            const T Scale = FMath::InvSqrt(X * X + Y * Y);
            return TVector<T>(X * Scale, Y * Scale, static_cast<T>(0));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::RotateAngleAxis(T _AngleDeg, const TVector<T>& _Axis) const
        {
            return RotateAngleAxisRad(FMath::DegreesToRadians(_AngleDeg), _Axis);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::RotateAngleAxisRad(T _AngleRad, const TVector<T>& _Axis) const
        {
            T S;
            T C;
            FMath::SinCos(&S, &C, _AngleRad);

            const T XX = _Axis.X * _Axis.X;
            const T YY = _Axis.Y * _Axis.Y;
            const T ZZ = _Axis.Z * _Axis.Z;
            const T XY = _Axis.X * _Axis.Y;
            const T YZ = _Axis.Y * _Axis.Z;
            const T ZX = _Axis.Z * _Axis.X;
            const T XS = _Axis.X * S;
            const T YS = _Axis.Y * S;
            const T ZS = _Axis.Z * S;
            const T OMC = static_cast<T>(1) - C;

            return TVector<T>(
                (OMC * XX + C) * X + (OMC * XY - ZS) * Y + (OMC * ZX + YS) * Z,
                (OMC * XY + ZS) * X + (OMC * YY + C) * Y + (OMC * YZ - XS) * Z,
                (OMC * ZX - YS) * X + (OMC * YZ + XS) * Y + (OMC * ZZ + C) * Z);
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::Reciprocal() const
        {
            const T Maximum = (std::numeric_limits<T>::max)();
            return TVector<T>(
                X != static_cast<T>(0) ? static_cast<T>(1) / X : Maximum,
                Y != static_cast<T>(0) ? static_cast<T>(1) / Y : Maximum,
                Z != static_cast<T>(0) ? static_cast<T>(1) / Z : Maximum);
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::IsUniform(T _Tolerance) const
        {
            return GetMax() - GetMin() <= _Tolerance;
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::MirrorByVector(const TVector<T>& _MirrorNormal) const
        {
            return *this - _MirrorNormal * (static_cast<T>(2) * (*this | _MirrorNormal));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::ProjectOnTo(const TVector<T>& _Vector) const
        {
            return _Vector * ((*this | _Vector) / (_Vector | _Vector));
        }

        template<typename T>
        FORCEINLINE TVector<T> TVector<T>::ProjectOnToNormal(const TVector<T>& _Normal) const
        {
            return _Normal * (*this | _Normal);
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::ContainsNaN() const
        {
            return !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z);
        }

        template<typename T>
        inline std::string TVector<T>::ToString() const
        {
            return StringFormat(
                "X=%.9g Y=%.9g Z=%.9g",
                static_cast<double>(X),
                static_cast<double>(Y),
                static_cast<double>(Z));
        }

        template<typename T>
        inline bool TVector<T>::InitFromString(std::string_view _Source)
        {
            return ParseValue(_Source, "X=", X)
                && ParseValue(_Source, "Y=", Y)
                && ParseValue(_Source, "Z=", Z);
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Dist(const TVector<T>& _Left, const TVector<T>& _Right)
        {
            return (_Right - _Left).Size();
        }

        template<typename T>
        FORCEINLINE T TVector<T>::DistSquared(const TVector<T>& _Left, const TVector<T>& _Right)
        {
            return (_Right - _Left).SizeSquared();
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Dist2D(const TVector<T>& _Left, const TVector<T>& _Right)
        {
            return (_Right - _Left).Size2D();
        }

        template<typename T>
        FORCEINLINE T TVector<T>::DistSquared2D(const TVector<T>& _Left, const TVector<T>& _Right)
        {
            return (_Right - _Left).SizeSquared2D();
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Triple(
            const TVector<T>& _X,
            const TVector<T>& _Y,
            const TVector<T>& _Z)
        {
            return _X.X * (_Y.Y * _Z.Z - _Y.Z * _Z.Y)
                + _X.Y * (_Y.Z * _Z.X - _Y.X * _Z.Z)
                + _X.Z * (_Y.X * _Z.Y - _Y.Y * _Z.X);
        }

        template<typename T>
        FORCEINLINE bool TVector<T>::IsNormalized() const
        {
            return (FMath::Abs(1.f - SizeSquared()) < UE_THRESH_VECTOR_NORMALIZED);
        }

        template<typename T>
        FORCEINLINE T& TVector<T>::Component(int32 Index)
        {
            return XYZ[Index];
        }

        template<typename T>
        FORCEINLINE T TVector<T>::Component(int32 Index) const
        {
            return XYZ[Index];
        }

    } // namespace UE::Math
} // namespace UE
