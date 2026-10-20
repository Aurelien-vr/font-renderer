#include "auCharacter.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <freetype/ftimage.h>
#include <iostream>
#include <sys/types.h>
#include <tuple>
#include <vector>

auCharacter::auCharacter(FT_Face face, short resolution)
    : face(face), resolution(resolution) {}

void auCharacter::setOutlineEnd() {
  for (short index = 0; index < face->glyph->outline.n_contours; index++) {
    outlineEnd.push_back(face->glyph->outline.contours[index]);
  }
}

void auCharacter::setResolution() {

  float interval = 1.f / (resolution + 1);

  FT_Outline outline = face->glyph->outline;

  short indexOfContour = 0;
  short contourStart = 0;
  uint coutourStartExtended = 0;

  bool hadInnerContour = false;

  for (short i_point = 0; i_point < outline.n_points; i_point++) {

    short contourEnd = outline.contours[indexOfContour];
    char tag = FT_CURVE_TAG(outline.tags[i_point]);

    if (tag == FT_CURVE_TAG_ON) {
      vertices.push_back(outline.points[i_point]);

    } else if (tag == FT_CURVE_TAG_CONIC) {

      // Neighbours wrapped inside the CURRENT contour
      short prevIdx = (i_point == contourStart) ? contourEnd : i_point - 1;
      short nextIdx = (i_point == contourEnd) ? contourStart : i_point + 1;

      FT_Vector_ control = outline.points[i_point];
      FT_Vector_ start = outline.points[prevIdx];
      FT_Vector_ end = outline.points[nextIdx];

      bool prevIsOn = FT_CURVE_TAG(outline.tags[prevIdx]) == FT_CURVE_TAG_ON;
      bool nextIsOn = FT_CURVE_TAG(outline.tags[nextIdx]) == FT_CURVE_TAG_ON;

      // Previous point is also off-curve: the curve starts at the implied
      // on-curve point between them (their midpoint)
      if (!prevIsOn) {
        start = {lerp(start.x, control.x, 0.5), lerp(start.y, control.y, 0.5)};
      }

      // Next point is also off-curve: the curve ends at the implied
      // on-curve point between them (their midpoint)
      if (!nextIsOn) {
        end = {lerp(control.x, end.x, 0.5), lerp(control.y, end.y, 0.5)};
      }

      std::vector<FT_Vector_> bezierCurvePoints =
          bezierPoints(interval, start, end, control);

      // The implied on-curve point is never visited as an ON point,
      // so it must be added here
      if (!nextIsOn) {
        bezierCurvePoints.push_back(end);
      }

      vertices.insert(vertices.end(), bezierCurvePoints.begin(),
                      bezierCurvePoints.end());

    } else if (tag == FT_CURVE_TAG_CUBIC) {
      std::cout << "CUBIC" << std::endl;
    }

    if (i_point == contourEnd) {
      FT_Vector *start = outline.points + contourStart;
      int count = contourEnd - contourStart + 1;
      float sign = shoelace(start, count);

      std::vector<FT_Vector_> currentContour(
          vertices.begin() + coutourStartExtended, vertices.end());

      if (sign < 0) {
        if (!outerContour.empty()) {
          triangulation(outerContour, innerContours);
          outerContour.clear();
          innerContours.clear();
        }

        outerContour = std::move(currentContour);
      } else if (sign > 0) {
        // Hole belonging to the current outer contour.
        if (!outerContour.empty()) {
          innerContours.push_back(std::move(currentContour));
        }
      }

      contourStart = contourEnd + 1;
      coutourStartExtended = vertices.size();
      indexOfContour++;
    }
  }

  triangulation(outerContour, innerContours);
}

long auCharacter::lerp(float a, float b, float f) {
  return a * (1.f - f) + (b * f);
}

// Computes sample points along a quadratic Bézier curve using De Casteljau’s
// method. For each parameter t (stepped by `interval`), it:
//  1) linearly interpolates (lerps) between `start` and `pivot`  -> point A
//  2) linearly interpolates (lerps) between `pivot` and `end`    -> point B
//  3) linearly interpolates (lerps) between A and B using the same t
// The result of step (3) is the actual point on the Bézier curve at that t.
// All computed points are stored in `lerpPoints` and returned.

std::vector<FT_Vector_> auCharacter::bezierPoints(float interval,
                                                  FT_Vector_ start,
                                                  FT_Vector_ end,
                                                  FT_Vector_ pivot) {

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

void auCharacter::triangulation(
    std::vector<FT_Vector_> outerVertices,
    std::vector<std::vector<FT_Vector_>> listInerPolygon) {

  // Outer contour must be CCW (shoelace > 0) : Verification
  if (shoelace(outerVertices.data(), (int)outerVertices.size()) < 0) {
    std::reverse(outerVertices.begin(), outerVertices.end());
  }

  // Holes must be CW (shoelace < 0) : Verification
  for (std::vector<FT_Vector_> &hole : listInerPolygon) {
    if (shoelace(hole.data(), (int)hole.size()) > 0) {
      std::reverse(hole.begin(), hole.end());
    }
  }

  if (listInerPolygon.size() != 0) {
    outerVertices = bridge(outerVertices, listInerPolygon);
  }

  std::vector<FT_Vector_> triangleOutput;

  earClipping(outerVertices, triangleOutput);

  // append at the end
  verticesTriangles.insert(verticesTriangles.end(), triangleOutput.begin(),
                           triangleOutput.end());
}

// Return pseudo simple polygon
std::vector<FT_Vector_>
auCharacter::bridge(std::vector<FT_Vector_> outerVertices,
                    std::vector<std::vector<FT_Vector_>> listInerPolygon) {

  for (std::vector<FT_Vector_> innerPolygon : listInerPolygon) {

    // Find the vertex with max x : M
    int indexMax = 0;
    for (int index = 1; index < (int)innerPolygon.size(); index++) {
      if (innerPolygon[index].x > innerPolygon[indexMax].x)
        indexMax = index;
    }
    FT_Vector_ m = innerPolygon[indexMax];

    // Collect all intersections between the rightward ray from M and outer
    // polygon edges
    std::vector<std::tuple<FT_Vector_, FT_Vector_>> intersectionPairs; // (I, P)

    for (int opIndex = 0; opIndex < (int)outerVertices.size(); opIndex++) {

      FT_Vector_ vertexA;
      FT_Vector_ vertexB = outerVertices[opIndex];
      if (opIndex == 0)
        vertexA = outerVertices[outerVertices.size() - 1];
      else
        vertexA = outerVertices[opIndex - 1];

      // Skip edges entirely above or entirely below M.y
      if ((vertexA.y < m.y) && (vertexB.y < m.y))
        continue;
      if ((vertexA.y > m.y) && (vertexB.y > m.y))
        continue;

      // I lands exactly on vertexA
      if (std::abs((float)(vertexB.y - vertexA.y)) < 0.001f ||
          std::abs((float)(m.y - vertexA.y) / (float)(vertexB.y - vertexA.y)) <=
              0.01f) {
        if (vertexA.x > m.x)
          intersectionPairs.push_back(std::make_tuple(vertexA, vertexA));
        continue;
      }

      // I lands exactly on vertexB
      if (std::abs((float)(m.y - vertexB.y) / (float)(vertexB.y - vertexA.y)) <=
          0.01f) {
        if (vertexB.x > m.x)
          intersectionPairs.push_back(std::make_tuple(vertexB, vertexB));
        continue;
      }

      // General case: I is strictly inside the edge
      float fValue = (float)(m.y - vertexA.y) / (float)(vertexB.y - vertexA.y);
      FT_Vector_ i = {lerp(vertexA.x, vertexB.x, fValue), m.y};

      // Only keep intersections to the right of M
      if (i.x <= m.x)
        continue;

      FT_Vector_ p;
      if (vertexA.x > vertexB.x)
        p = vertexA;
      else if (vertexB.x > vertexA.x)
        p = vertexB;
      else {
        // same x => vertical edge, choose endpoint closest to y = m.y
        long da = std::labs(vertexA.y - m.y);
        long db = std::labs(vertexB.y - m.y);
        p = (da < db) ? vertexA : vertexB;
      }

      intersectionPairs.push_back(std::make_tuple(i, p));
    }

    if (intersectionPairs.empty())
      continue;

    std::tuple<FT_Vector_, FT_Vector_> closest = intersectionPairs[0];
    float closestX = std::get<0>(intersectionPairs[0]).x;

    for (int k = 0; k < (int)intersectionPairs.size(); k++) {
      float ix = std::get<0>(intersectionPairs[k]).x;
      if (ix < closestX) {
        closestX = ix;
        closest = intersectionPairs[k];
      }
    }

    FT_Vector_ I = std::get<0>(closest);
    FT_Vector_ P = std::get<1>(closest);

    // Default: bridge to P
    FT_Vector_ R = P;

    // Default indexR = index of P in outerVertices
    int indexR = -1;
    for (int i = 0; i < (int)outerVertices.size(); ++i) {
      if (outerVertices[i].x == P.x && outerVertices[i].y == P.y) {
        indexR = i;
        break;
      }
    }

    if (indexR == -1) {
      continue;
    }

    float minAngle = 360.0f;

    for (int index = 0; index < (int)outerVertices.size(); ++index) {
      FT_Vector_ v = outerVertices[index];

      if (v.x == P.x && v.y == P.y)
        continue; // exclude P per the paper

      if (!isInsideTriangle(m, I, P, v))
        continue;

      // Only reflex vertices of the OUTER polygon
      if (!isReflexOuterVertex(outerVertices, index))
        continue;

      float currentAngle = angleBetweenAtM(m, I, v);
      if (currentAngle < minAngle) {
        minAngle = currentAngle;
        R = v;
        indexR = index;
      }
    }

    std::vector<FT_Vector_> merged;
    merged.reserve(outerVertices.size() + innerPolygon.size() + 2);

    merged.insert(merged.end(), outerVertices.begin(),
                  outerVertices.begin() + indexR + 1);

    int n = (int)innerPolygon.size();
    for (int k = 0; k < n; ++k)
      merged.push_back(innerPolygon[(indexMax + k) % n]);

    merged.push_back(innerPolygon[indexMax]);
    merged.push_back(outerVertices[indexR]);

    merged.insert(merged.end(), outerVertices.begin() + indexR + 1,
                  outerVertices.end());

    // now merged is your new outer polygon
    outerVertices = std::move(merged);
  }

  return outerVertices;
}

float auCharacter::distance(FT_Vector_ a, FT_Vector_ b) {
  float dx = float(a.x) - float(b.x);
  float dy = float(a.y) - float(b.y);
  return std::sqrt(dx * dx + dy * dy);
}

float auCharacter::shoelace(FT_Vector *points, int count) {
  float area = 0.0f;
  for (int i = 0; i < count; i++) {
    FT_Vector a = points[i];
    FT_Vector b = points[(i + 1) % count];
    area += (float)(a.x * b.y - b.x * a.y);
  }
  return area * 0.5f;
}

long auCharacter::area(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c) {
  return (long)(b.y - a.y) * (c.x - b.x) - (long)(b.x - a.x) * (c.y - b.y);
}

bool auCharacter::isInsideTriangle(FT_Vector_ A, FT_Vector_ B, FT_Vector_ C,
                                   FT_Vector_ P) {
  float d1 = (P.x - B.x) * (A.y - B.y) - (A.x - B.x) * (P.y - B.y);
  float d2 = (P.x - C.x) * (B.y - C.y) - (B.x - C.x) * (P.y - C.y);
  float d3 = (P.x - A.x) * (C.y - A.y) - (C.x - A.x) * (P.y - A.y);

  bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

  return !(has_neg && has_pos);
}

float auCharacter::angleBetweenAtM(FT_Vector_ M, FT_Vector_ I, FT_Vector_ R) {
  float ax = float(I.x - M.x);
  float ay = float(I.y - M.y);
  float bx = float(R.x - M.x);
  float by = float(R.y - M.y);

  float dot = ax * bx + ay * by;
  float la = std::sqrt(ax * ax + ay * ay);
  float lb = std::sqrt(bx * bx + by * by);
  if (la <= 0.0f || lb <= 0.0f)
    return 360.0f;

  float c = dot / (la * lb);
  c = std::max(-1.0f, std::min(1.0f, c)); // clamp for safety

  return std::acos(c) * 180.0f / float(M_PI); // 0..180
}

bool auCharacter::isReflexOuterVertex(const std::vector<FT_Vector_> &outer,
                                      int i) {
  int n = (int)outer.size();
  // Consider &outer like a loop (closed polygone) of point where*
  // index(size + 1) = index(0)
  const FT_Vector_ &A = outer[(i - 1 + n) % n];
  const FT_Vector_ &B = outer[i];
  const FT_Vector_ &C = outer[(i + 1) % n];

  long long ux = (long long)B.x - A.x;
  long long uy = (long long)B.y - A.y;
  long long vx = (long long)C.x - B.x;
  long long vy = (long long)C.y - B.y;

  long long cross = ux * vy - uy * vx;
  if (cross == 0)
    return false;

  // Determine winding once per polygon ideally, but this is simplest:
  float area = shoelace((FT_Vector *)outer.data(), n);
  bool isCCW = (area > 0);

  return isCCW ? (cross < 0) : (cross > 0);
}

bool auCharacter::isPointsInsideTriangle(const std::vector<FT_Vector_> &polygon,
                                         int i) {

  // STRICT VERSION (IA)

  FT_Vector_ prev = polygon[(i - 1 + polygon.size()) % polygon.size()];
  FT_Vector_ mid = polygon[i];
  FT_Vector_ next = polygon[(i + 1) % polygon.size()];

  for (size_t indexVert = 0; indexVert < (size_t)polygon.size(); indexVert++) {

    if (indexVert == (i - 1 + polygon.size()) % polygon.size() ||
        indexVert == (size_t)i || indexVert == (i + 1) % polygon.size()) {
      continue;
    }

    FT_Vector_ P = polygon[indexVert];

    float d1 =
        (P.x - mid.x) * (prev.y - mid.y) - (prev.x - mid.x) * (P.y - mid.y);
    float d2 =
        (P.x - next.x) * (mid.y - next.y) - (mid.x - next.x) * (P.y - next.y);
    float d3 =
        (P.x - prev.x) * (next.y - prev.y) - (next.x - prev.x) * (P.y - prev.y);

    // Strict interior test: a point that lies exactly on (or on the
    // extension of) one of the triangle's edges must NOT block the ear.
    // Dense, near-collinear bezier sampling makes boundary/collinear
    // false-positives common, so d == 0 must not count as "inside" here.
    bool strictlyInside =
        (d1 > 0 && d2 > 0 && d3 > 0) || (d1 < 0 && d2 < 0 && d3 < 0);

    if (strictlyInside)
      return true;
  }
  return false;
}

bool auCharacter::isFullyInsidePolygone(const std::vector<FT_Vector_> &polygone,
                                        int i) {
  const int n = polygone.size();

  const int ia = (i + n - 1) % n; // a = i-1
  const int ib = (i + 1) % n;     // b = i+1

  FT_Vector_ a = polygone[ia];
  FT_Vector_ b = polygone[ib];
  FT_Vector_ prevA = polygone[(i + n - 2) % n]; // a.prev
  FT_Vector_ tip = polygone[i];                 // a.next and b.prev
  FT_Vector_ nextB = polygone[(i + 2) % n];     // b.next

  // Degenerate case: a and b coincide (duplicated bridge vertex)
  bool zeroLength = a.x == b.x && a.y == b.y && area(prevA, a, tip) > 0 &&
                    area(tip, b, nextB) > 0;

  // The diagonal does not cross any other edge
  if (intersectWithPolygone(polygone, i)) {
    return false;
  }

  // Degenerate case: only the crossing test applies
  if (zeroLength) {
    return true;
  }

  // The diagonal leaves a and b toward the interior of the polygone
  bool locallyVisible =
      locallyInside(polygone, ia, b) && locallyInside(polygone, ib, a);

  if (!locallyVisible) {
    return false;
  }

  // No opposite-facing sectors (neighbours collinear with the diagonal)
  bool noOppositeSectors = area(prevA, a, tip) != 0 || area(a, tip, b) != 0;

  if (!noOppositeSectors) {
    return false;
  }

  // The diagonal runs through the interior, not through a hole or exterior
  return middleInside(polygone, a, b);
}

bool auCharacter::locallyInside(const std::vector<FT_Vector_> &p, int idx,
                                FT_Vector_ target) {
  const int n = p.size();
  FT_Vector_ a = p[idx];
  FT_Vector_ prev = p[(idx + n - 1) % n];
  FT_Vector_ next = p[(idx + 1) % n];

  return area(prev, a, next) < 0
             ? area(a, target, next) >= 0 && area(a, prev, target) >= 0
             : area(a, target, prev) < 0 || area(a, next, target) < 0;
}

bool auCharacter::middleInside(const std::vector<FT_Vector_> &poly,
                               FT_Vector_ a, FT_Vector_ b) {
  double px = (a.x + b.x) / 2.0;
  double py = (a.y + b.y) / 2.0;
  bool inside = false;
  const int n = poly.size();

  for (int j = 0; j < n; j++) {
    FT_Vector_ p = poly[j];
    FT_Vector_ q = poly[(j + 1) % n];
    if (((p.y > py) != (q.y > py)) && q.y != p.y &&
        px < (double)(q.x - p.x) * (py - p.y) / (double)(q.y - p.y) + p.x)
      inside = !inside;
  }
  return inside;
}

void auCharacter::earClipping(std::vector<FT_Vector_> vertices,
                              std::vector<FT_Vector_> &triangleOutput) {

  while (vertices.size() > 3) {
    ulong earIndex = findEar(vertices);

    if (earIndex >= vertices.size()) {
      break;
    }

    triangleOutput.push_back(
        vertices[(earIndex - 1 + vertices.size()) % vertices.size()]);
    triangleOutput.push_back(vertices[earIndex]);
    triangleOutput.push_back(vertices[(earIndex + 1) % vertices.size()]);

    // std::cout << "DEBUG: About to erase index " << earIndex << std::endl;
    vertices.erase(vertices.begin() + earIndex);
  }

  // Add the last triangle to the return output
  if (vertices.size() == 3) {
    for (FT_Vector_ v : vertices)
      triangleOutput.push_back(v);
  } else {
    std::cerr << "earClipping failed, " << vertices.size()
              << " vertices left\n";
  }
}

ulong auCharacter::findEar(std::vector<FT_Vector_> vertices) {
  for (ulong i = 0; i < vertices.size(); i++) {

    if (isReflexOuterVertex(vertices, i)) {
      continue;
    }

    if (isPointsInsideTriangle(vertices, i)) {
      continue;
    }

    if (!isFullyInsidePolygone(vertices, i)) {
      continue;
    }

    return i;
  }

  return ULONG_MAX;
}

bool auCharacter::intersectProp(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c,
                                FT_Vector_ d) {
  if (collinear(&a, &b, &c) || collinear(&a, &b, &d) || collinear(&c, &d, &a) ||
      collinear(&c, &b, &d)) {
    return false;
  }

  auto Area2 = [](const FT_Vector_ &p1, const FT_Vector_ &p2,
                  const FT_Vector_ &p3) -> long {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);
  };

  auto Left = [&](const FT_Vector_ &p1, const FT_Vector_ &p2,
                  const FT_Vector_ &p3) -> bool {
    return Area2(p1, p2, p3) > 0;
  };

  auto Xor = [](bool x, bool y) -> bool {
    return static_cast<bool>(x) ^ static_cast<bool>(y);
  };

  return Xor(Left(a, b, c), Left(a, b, d)) && Xor(Left(c, d, a), Left(c, d, b));
}

bool auCharacter::collinear(FT_Vector_ *a, FT_Vector_ *b, FT_Vector_ *c) {
  long long cross = (long long)(b->x - a->x) * (c->y - a->y) -
                    (long long)(b->y - a->y) * (c->x - a->x);
  return cross == 0;
}

bool auCharacter::between(FT_Vector_ *a, FT_Vector_ *b, FT_Vector_ *c) {

  if (!collinear(a, b, c)) {
    return false;
  }

  if (a->x != b->x) {
    return ((a->x <= c->x && c->x <= b->x) || (a->x >= c->x && c->x >= b->x));
  } else {
    return ((a->y <= c->y && c->y <= b->y) || (a->y >= c->y && c->y >= b->y));
  }
}

bool auCharacter::intersectWithPolygone(const std::vector<FT_Vector_> &polygone,
                                        int i) {
  const int n = polygone.size();

  FT_Vector_ a = polygone[(i + n - 1) % n]; // i-1
  FT_Vector_ b = polygone[(i + 1) % n];     // i+1

  for (int j = 0; j < n; j++) {
    FT_Vector_ c = polygone[j];
    FT_Vector_ d = polygone[(j + 1) % n];

    // Skip edges sharing an endpoint with the diagonal (by coordinates,
    // so duplicated bridge vertices are also skipped)
    if ((c.x == a.x && c.y == a.y) || (d.x == a.x && d.y == a.y) ||
        (c.x == b.x && c.y == b.y) || (d.x == b.x && d.y == b.y)) {
      continue;
    }

    if (intersect(a, b, c, d)) {
      return true;
    }
  }

  return false;
}

bool auCharacter::intersect(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c,
                            FT_Vector_ d) {
  if (intersectProp(a, b, c, d)) {
    return true;
  }

  return between(&a, &b, &c) || between(&a, &b, &d) || between(&c, &d, &a) ||
         between(&c, &d, &b);
}
