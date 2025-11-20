module;

#include <cmath>

module DEngine.Math.Common;

import DEngine.FixedWidthTypes;

using namespace DEngine;

f32 Math::Ceil(f32 input) { return ceilf(input); }
f64 Math::Ceil(f64 input) { return ceil(input); }
f32 Math::Floor(f32 input) { return floorf(input); }
f64 Math::Floor(f64 input) { return floor(input); }
f32 Math::Round(f32 input) { return roundf(input); }
f64 Math::Round(f64 input) { return round(input); }

f32 Math::Sqrt(f32 input) { return sqrtf(input); }
f64 Math::Sqrt(f64 input) { return sqrt(input); }

f32 Math::Pow(f32 coefficient, f32 exponent) { return powf(coefficient, exponent); }

f64 Math::Pow(f64 coefficient, f64 exponent) { return pow(coefficient, exponent); }
