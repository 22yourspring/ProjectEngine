#pragma once

#include "UE/CoreTypes.h"
#include "UE/Math/MathFwd.h"
#include "UE/Math/UnrealMathUtility.h"

namespace UE
{
    namespace Math
    {
        template<typename T>
        struct TVector2
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
                };

                T XY[2];
            };

            /** Global 2D zero vector constant (0,0) */
            CORE_API static const TVector2<T> ZeroVector;

            /**
            * Global 2D one vector (poorly named) constant (1,1).
            *
            * @note Incorrectly named "unit" vector though its magnitude/length/size is not one. Would fix, though likely used all over the world. Use `Unit45Deg` below for an actual unit vector.
            */
            CORE_API static const TVector2<T> UnitVector;

            /**
            * Global 2D unit vector constant along the 45 degree angle or symmetrical positive axes (sqrt(.5),sqrt(.5)) or (.707,.707). https://en.wikipedia.org/wiki/Unit_vector
            *
            * @note The `UnitVector` above is actually a value with axes of 1 rather than a magnitude of one.
            */
            CORE_API static const TVector2<T> Unit45Deg;

            static inline TVector2<T> Zero() { return TVector2<T>::ZeroVector; }
            static inline TVector2<T> One() { return TVector2<T>::UnitVector; }
            static inline TVector2<T> UnitX() { return TVector2<T>(1, 0); }
            static inline TVector2<T> UnitY() { return TVector2<T>(0, 1); }

        public:

            /** Default constructor (no initialization). */
            FORCEINLINE TVector2<T>() {}

            /**
            * Constructor using initial values for each component.
            *
            * @param InX X coordinate.
            * @param InY Y coordinate.
            */
            FORCEINLINE TVector2<T>(T InX, T InY);

            /**
            * Constructor initializing both components to a single T value.
            *
            * @param InF Value to set both components to.
            */
            explicit FORCEINLINE TVector2<T>(T InF);

            /**
            * Constructs a vector from an FVector.
            * Copies the X and Y components from the FVector.
            *
            * @param V Vector to copy from.
            */
            explicit FORCEINLINE TVector2<T>(const TVector<T>& V);

        public:

            /**
            * Gets the result of adding two vectors together.
            *
            * @param V The other vector to add to this.
            * @return The result of adding the vectors together.
            */
            FORCEINLINE TVector2<T> operator+(const TVector2<T>& V) const;

            /**
            * Gets the result of subtracting a vector from this one.
            *
            * @param V The other vector to subtract from this.
            * @return The result of the subtraction.
            */
            FORCEINLINE TVector2<T> operator-(const TVector2<T>& V) const;

            /**
            * Gets the result of scaling the vector (multiplying each component by a value).
            *
            * @param Scale How much to scale the vector by.
            * @return The result of scaling this vector.
            */
            FORCEINLINE TVector2<T> operator*(T Scale) const;

            /**
            * Gets the result of dividing each component of the vector by a value.
            *
            * @param Scale How much to divide the vector by.
            * @return The result of division on this vector.
            */
            TVector2<T> operator/(T Scale) const;

            /**
            * Gets the result of adding A to each component of the vector.
            *
            * @param A T to add to each component.
            * @return The result of adding A to each component.
            */
            FORCEINLINE TVector2<T> operator+(T A) const;

            /**
            * Gets the result of subtracting A from each component of the vector.
            *
            * @param A T to subtract from each component
            * @return The result of subtracting A from each component.
            */
            FORCEINLINE TVector2<T> operator-(T A) const;

            /**
            * Gets the result of component-wise multiplication of this vector by another.
            *
            * @param V The other vector to multiply this by.
            * @return The result of the multiplication.
            */
            FORCEINLINE TVector2<T> operator*(const TVector2<T>& V) const;

            /**
            * Gets the result of component-wise division of this vector by another.
            *
            * @param V The other vector to divide this by.
            * @return The result of the division.
            */
            TVector2<T> operator/(const TVector2<T>& V) const;

            /**
            * Calculates dot product of this vector and another.
            *
            * @param V The other vector.
            * @return The dot product.
            */
            FORCEINLINE T operator|(const TVector2<T>& V) const;

            /**
            * Calculates cross product of this vector and another.
            *
            * @param V The other vector.
            * @return The cross product.
            */
            FORCEINLINE T operator^(const TVector2<T>& V) const;

        public:

            /**
            * Compares this vector against another for equality.
            *
            * @param V The vector to compare against.
            * @return true if the two vectors are equal, otherwise false.
            */
            bool operator==(const TVector2<T>& V) const;

            /**
            * Compares this vector against another for inequality.
            *
            * @param V The vector to compare against.
            * @return true if the two vectors are not equal, otherwise false.
            */
            bool operator!=(const TVector2<T>& V) const;

            /**
            * Gets a negated copy of the vector.
            *
            * @return A negated copy of the vector.
            */
            FORCEINLINE TVector2<T> operator-() const;

            /**
            * Adds another vector to this.
            *
            * @param V The other vector to add.
            * @return Copy of the vector after addition.
            */
            FORCEINLINE TVector2<T> operator+=(const TVector2<T>& V);

            /**
            * Subtracts another vector from this.
            *
            * @param V The other vector to subtract.
            * @return Copy of the vector after subtraction.
            */
            FORCEINLINE TVector2<T> operator-=(const TVector2<T>& V);

            /**
            * Scales this vector.
            *
            * @param Scale The scale to multiply vector by.
            * @return Copy of the vector after scaling.
            */
            FORCEINLINE TVector2<T> operator*=(T Scale);

            /**
            * Divides this vector.
            *
            * @param V What to divide vector by.
            * @return Copy of the vector after division.
            */
            TVector2<T> operator/=(T V);

            /**
            * Multiplies this vector with another vector, using component-wise multiplication.
            *
            * @param V The vector to multiply with.
            * @return Copy of the vector after multiplication.
            */
            TVector2<T> operator*=(const TVector2<T>& V);

            /**
            * Divides this vector by another vector, using component-wise division.
            *
            * @param V The vector to divide by.
            * @return Copy of the vector after division.
            */
            TVector2<T> operator/=(const TVector2<T>& V);

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
            * @return copy of component value.
            */
            T operator[](int32 Index) const;

            /**
            * Gets a specific component of the vector.
            *
            * @param Index The index of the component required.
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

            /**
            * Calculates the dot product of two vectors.
            *
            * @param A The first vector.
            * @param B The second vector.
            * @return The dot product.
            */
            FORCEINLINE static T DotProduct(const TVector2<T>& A, const TVector2<T>& B);

            /**
            * Calculate the cross product of two vectors.
            *
            * @param A The first vector.
            * @param B The second vector.
            * @return The cross product.
            */
            FORCEINLINE static T CrossProduct(const TVector2<T>& A, const TVector2<T>& B);
            /**
            * Set the values of the vector directly.
            *
            * @param InX New X coordinate.
            * @param InY New Y coordinate.
            */
            void Set(T InX, T InY);

            /**
            * Get the length (magnitude) of this vector.
            *
            * @return The length of this vector.
            * @see Length - This function is a synonym for Length()
            */
            T Size() const;

            /** Get the squared length of this vector. */
            T SizeSquared() const;

            /**
            * Get the length (magnitude) of this vector.
            *
            * @return The length of this vector.
            * @see Size - This function is a synonym for Size()
            */
            FORCEINLINE T Length() const { return Size(); }

            /**
            * Get the dot product of this vector against another.
            *
            * @param V2 The vector to measure dot product against.
            * @return The dot product.
            */
            T Dot(const TVector2<T>& V2) const;

            /**
            * Normalize this vector in-place if it is large enough, set it to (0,0) otherwise.
            * (Note this is different from TVector<>::Normalize, which leaves the vector unchanged if it is too small to normalize.)
            *
            * @param Tolerance Minimum squared length of vector for normalization.
            * @see GetSafeNormal()
            * @return true if the vector was normalized correctly, false if it was too small and set to zero.
            */
            bool Normalize(T Tolerance = UE_SMALL_NUMBER);

            /**
            * Get a copy of this vector with absolute value of each component.
            *
            * @return A copy of this vector with absolute value of each component.
            */
            FORCEINLINE TVector2<T> GetAbs() const;

            // Conversion from other type.
            template<typename FArg, TEMPLATE_REQUIRES(!std::is_same_v<T, FArg>)>
            explicit TVector2(const TVector2<FArg>& From) : TVector2<T>((T)From.X, (T)From.Y) {}

            explicit TVector2(const TVector4<T>& From);
        };

        /* TVector2<T> inline functions
        *****************************************************************************/

        /**
         * Multiplies a Vector2 by a scaling factor.
         *
         * @param Scale Scaling factor.
         * @param V Vector2 to scale.
         * @return Result of multiplication.
         */
        template<typename T, typename T2, TEMPLATE_REQUIRES(std::is_arithmetic<T2>::value)>
        FORCEINLINE TVector2<T> operator*(T2 Scale, const TVector2<T>& V)
        {
            return V.operator*(Scale);
        }

        template<typename T>
        FORCEINLINE TVector2<T>::TVector2(T InX, T InY)
            : X(InX), Y(InY)
        {
        }

        template<typename T>
        FORCEINLINE TVector2<T>::TVector2(T InF)
            : X(InF), Y(InF)
        {
        }

        template<typename T>
        FORCEINLINE TVector2<T>::TVector2(const TVector<T>& V)
            : X(V.X), Y(V.Y)
        {
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator+(const TVector2<T>& V) const
        {
            return TVector2<T>(X + V.X, Y + V.Y);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator-(const TVector2<T>& V) const
        {
            return TVector2<T>(X - V.X, Y - V.Y);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator*(T Scale) const
        {
            return TVector2<T>(X * Scale, Y * Scale);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator/(T Scale) const
        {
            const T RScale = 1.f / Scale;
            return TVector2<T>(X * RScale, Y * RScale);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator+(T A) const
        {
            return TVector2<T>(X + A, Y + A);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator-(T A) const
        {
            return TVector2<T>(X - A, Y - A);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator*(const TVector2<T>& V) const
        {
            return TVector2<T>(X * V.X, Y * V.Y);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator/(const TVector2<T>& V) const
        {
            return TVector2<T>(X / V.X, Y / V.Y);
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::operator|(const TVector2<T>& V) const
        {
            return X * V.X + Y * V.Y;
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::operator^(const TVector2<T>& V) const
        {
            return X * V.Y - Y * V.X;
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::DotProduct(const TVector2<T>& A, const TVector2<T>& B)
        {
            return A | B;
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::CrossProduct(const TVector2<T>& A, const TVector2<T>& B)
        {
            return A ^ B;
        }

        template<typename T>
        FORCEINLINE bool TVector2<T>::operator==(const TVector2<T>& V) const
        {
            return X == V.X && Y == V.Y;
        }

        template<typename T>
        FORCEINLINE bool TVector2<T>::operator!=(const TVector2<T>& V) const
        {
            return X != V.X || Y != V.Y;
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator-() const
        {
            return TVector2<T>(-X, -Y);
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator+=(const TVector2<T>& V)
        {
            X += V.X; Y += V.Y;
            return *this;
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator-=(const TVector2<T>& V)
        {
            X -= V.X; Y -= V.Y;
            return *this;
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator*=(T Scale)
        {
            X *= Scale; Y *= Scale;
            return *this;
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator/=(T V)
        {
            const T RV = 1.f / V;
            X *= RV; Y *= RV;
            return *this;
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator*=(const TVector2<T>& V)
        {
            X *= V.X; Y *= V.Y;
            return *this;
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::operator/=(const TVector2<T>& V)
        {
            X /= V.X; Y /= V.Y;
            return *this;
        }

        template<typename T>
        FORCEINLINE T& TVector2<T>::operator[](int32 Index)
        {
            return ((Index == 0) ? X : Y);
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::operator[](int32 Index) const
        {
            return ((Index == 0) ? X : Y);
        }

        template<typename T>
        FORCEINLINE void TVector2<T>::Set(T InX, T InY)
        {
            X = InX;
            Y = InY;
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::Size() const
        {
            return FMath::Sqrt(X * X + Y * Y);
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::SizeSquared() const
        {
            return X * X + Y * Y;
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::Dot(const TVector2<T>& V2) const
        {
            return X * V2.X + Y * V2.Y;
        }

        template<typename T>
        FORCEINLINE bool TVector2<T>::Normalize(T Tolerance)
        {
            const T SquareSum = X * X + Y * Y;
            if (SquareSum > Tolerance)
            {
                const T Scale = FMath::InvSqrt(SquareSum);
                X *= Scale;
                Y *= Scale;
                return true;
            }
            X = 0.0f;
            Y = 0.0f;
            return false;
        }

        template<typename T>
        FORCEINLINE T& TVector2<T>::Component(int32 Index)
        {
            return XY[Index];
        }

        template<typename T>
        FORCEINLINE T TVector2<T>::Component(int32 Index) const
        {
            return XY[Index];
        }

        template<typename T>
        FORCEINLINE TVector2<T> TVector2<T>::GetAbs() const
        {
            return TVector2<T>(FMath::Abs(X), FMath::Abs(Y));
        }
    }
}
