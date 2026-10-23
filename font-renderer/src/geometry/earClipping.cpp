#include "earClipping.h"
#include <cmath>
#include <cstddef>

namespace earClipping {
float distance(FT_Vector_ a, FT_Vector_ b) {
  float dx = float(a.x) - float(b.x);
  float dy = float(a.y) - float(b.y);
  return std::sqrt(dx * dx + dy * dy);
}

float shoelace(FT_Vector *points, int count) {
  float area = 0.0f;
  for (int i = 0; i < count; i++) {
    FT_Vector a = points[i];
    FT_Vector b = points[(i + 1) % count];
    area += (float)(a.x * b.y - b.x * a.y);
  }
  return area * 0.5f;
}

long area(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c) {
  return (long)(b.y - a.y) * (c.x - b.x) - (long)(b.x - a.x) * (c.y - b.y);
}

bool isInsideTriangle(FT_Vector_ A, FT_Vector_ B, FT_Vector_ C, FT_Vector_ P) {
  float d1 = (P.x - B.x) * (A.y - B.y) - (A.x - B.x) * (P.y - B.y);
  float d2 = (P.x - C.x) * (B.y - C.y) - (B.x - C.x) * (P.y - C.y);
  float d3 = (P.x - A.x) * (C.y - A.y) - (C.x - A.x) * (P.y - A.y);

  bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

  return !(has_neg && has_pos);
}

float angleBetweenAtM(FT_Vector_ M, FT_Vector_ I, FT_Vector_ R) {
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

bool isReflexOuterVertex(const std::vector<FT_Vector_> &outer, int i) {
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

bool isPointsInsideTriangle(const std::vector<FT_Vector_> &polygon, int i) {

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

bool isFullyInsidePolygone(const std::vector<FT_Vector_> &polygone, int i) {
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

bool locallyInside(const std::vector<FT_Vector_> &p, int idx,
                   FT_Vector_ target) {
  const int n = p.size();
  FT_Vector_ a = p[idx];
  FT_Vector_ prev = p[(idx + n - 1) % n];
  FT_Vector_ next = p[(idx + 1) % n];

  return area(prev, a, next) < 0
             ? area(a, target, next) >= 0 && area(a, prev, target) >= 0
             : area(a, target, prev) < 0 || area(a, next, target) < 0;
}

bool middleInside(const std::vector<FT_Vector_> &poly, FT_Vector_ a,
                  FT_Vector_ b) {
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

bool intersectProp(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c, FT_Vector_ d) {
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

bool collinear(FT_Vector_ *a, FT_Vector_ *b, FT_Vector_ *c) {
  long long cross = (long long)(b->x - a->x) * (c->y - a->y) -
                    (long long)(b->y - a->y) * (c->x - a->x);
  return cross == 0;
}

bool between(FT_Vector_ *a, FT_Vector_ *b, FT_Vector_ *c) {

  if (!collinear(a, b, c)) {
    return false;
  }

  if (a->x != b->x) {
    return ((a->x <= c->x && c->x <= b->x) || (a->x >= c->x && c->x >= b->x));
  } else {
    return ((a->y <= c->y && c->y <= b->y) || (a->y >= c->y && c->y >= b->y));
  }
}

bool intersectWithPolygone(const std::vector<FT_Vector_> &polygone, int i) {
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

bool intersect(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c, FT_Vector_ d) {
  if (intersectProp(a, b, c, d)) {
    return true;
  }

  return between(&a, &b, &c) || between(&a, &b, &d) || between(&c, &d, &a) ||
         between(&c, &d, &b);
}
} // namespace earClipping
