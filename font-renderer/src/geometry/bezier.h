#pragma once

#include <freetype/freetype.h>
#include <vector>

namespace geometry {
long lerp(float a, float b, float f);

std::vector<FT_Vector_> bezierPoints(float interval, FT_Vector_ start,
                                     FT_Vector_ end, FT_Vector_ pivot);
} // namespace geometry
