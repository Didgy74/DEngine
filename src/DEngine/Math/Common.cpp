#include "DEngine/Math/Common.hpp"

#include <cmath>

DEngine::f32 DEngine::Math::Sqrt(f32 input)
{
    return std::sqrtf(input);
}

DEngine::f64 DEngine::Math::Sqrt(f64 input)
{
    return std::sqrtl(input);
}

DEngine::f32 DEngine::Math::Pow(f32 coefficient, f32 exponent)
{
    return std::powf(coefficient, exponent);
}

DEngine::f64 DEngine::Math::Pow(f64 coefficient, f64 exponent)
{
    return std::powl(coefficient, exponent);
}