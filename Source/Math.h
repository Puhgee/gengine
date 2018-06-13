//
//  Math.h
//  GEngine
//
//  Created by Clark Kromenaker on 10/16/17.
//
#pragma once
#include <cmath>

namespace Math
{
    // Dictates that floating-point numbers within 0.000001 units are equal to one another.
    static const float kEpsilon = 1.0e-6f;
    
    // Pi-related constants.
    static const float kPi = 3.1415926535897932384626433832795f;
    static const float k2Pi = 2.0f * kPi;
    static const float kPiOver2 = kPi / 2.0f;
    static const float kPiOver4 = kPi / 4.0f;
    
    // Calculates the square root of a value.
    inline float Sqrt(float val)
    {
        return std::sqrtf(val);
    }
    
    // Calculates the inverse square root of a value.
    inline float InvSqrt(float val)
    {
        return (1.0f / std::sqrtf(val));
    }
    
    inline bool IsZero(float val)
    {
        return (fabsf(val) < kEpsilon);
    }
    
    inline bool AreEqual(float a, float b)
    {
        return IsZero(a - b);
    }
    
    inline float Mod(float num1, float num2)
    {
        return fmod(num1, num2);
    }
    
    inline float Sin(float radians)
    {
        return std::sinf(radians);
    }
    
    inline float Asin(float ratio)
    {
        return std::asinf(ratio);
    }
    
    inline float Cos(float radians)
    {
        return std::cosf(radians);
    }
    
    inline float Acos(float ratio)
    {
        return std::acosf(ratio);
    }
    
    inline float Tan(float radians)
    {
        return std::tanf(radians);
    }
    
    inline float Atan(float ratio)
    {
        return std::atanf(ratio);
    }
    
    inline float Floor(float val)
    {
        return std::floor(val);
    }
    
    inline float Ceil(float val)
    {
        return std::ceil(val);
    }
    
    inline float Round(float val)
    {
        return std::round(val);
    }
    
    inline int FloorToInt(float val)
    {
        return (int)Floor(val);
    }
    
    inline int CeilToInt(float val)
    {
        return (int)Ceil(val);
    }
    
    inline int RoundToInt(float val)
    {
        return (int)Round(val);
    }
    
    inline float Clamp(float value, float min, float max)
    {
        return std::min(max, std::max(value, min));
    }
    
    inline int Clamp(int value, int min, int max)
    {
        return std::min(max, std::max(value, min));
    }
    
    inline float Abs(float val)
    {
        return std::abs(val);
    }
    
    inline float ToDegrees(float radians)
    {
        return (radians * (180.0f / kPi));
    }
    
    inline float ToRadians(float degrees)
    {
        return (degrees * (kPi / 180.0f));
    }
}
