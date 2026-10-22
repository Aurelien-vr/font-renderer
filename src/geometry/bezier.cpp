#include "bezier.h"

namespace geometry {

long lerp(float a, float b, float f) { return a * (1.f - f) + (b * f); }

// Computes sample points along a quadratic Bézier curve using De Casteljau’s
// method. For each parameter t (stepped by `interval`), it:
//  1) linearly interpolates (lerps) between `start` and `pivot`  -> point A
//  2) linearly interpolates (lerps) between `pivot` and `end`    -> point B
//  3) linearly interpolates (lerps) between A and B using the same t
// The result of step (3) is the actual point on the Bézier curve at that t.
// All computed points are stored in `lerpPoints` and returned.

std::vector<FT_Vector_> bezierPoints(float interval, FT_Vector_ start,
                                     FT_Vector_ end, FT_Vector_ pivot) {

  std::vector<FT_Vector_> lerpPoints;

  for (float f = interval; f < 1; f += interval) {
    FT_Vector_ lerpPointA = {lerp(start.x, pivot.x, f),
                             lerp(start.y, pivot.y, f)};
    FT_Vector_ lerpPointB = {lerp(pivot.x, end.x, f), lerp(pivot.y, end.y, f)};

    FT_Vector_ lerpPointCurve = {lerp(lerpPointA.x, lerpPointB.x, f),
                                 lerp(lerpPointA.y, lerpPointB.y, f)};

    lerpPoints.push_back(lerpPointCurve);
  }
  return lerpPoints;
}
} // namespace geometry
