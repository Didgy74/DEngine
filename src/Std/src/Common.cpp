module;

#include <cmath>

module DEngine.Std.Common;

import DEngine.FixedWidthTypes;

using namespace DEngine;

f32 Std::Ceil(f32 input) { return ceilf(input); }
f64 Std::Ceil(f64 input) { return ceil(input); }
f32 Std::Floor(f32 input) { return floorf(input); }
f64 Std::Floor(f64 input) { return floor(input); }
f32 Std::Round(f32 input) { return roundf(input); }
f64 Std::Round(f64 input) { return round(input); }