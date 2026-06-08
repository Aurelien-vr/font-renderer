#include "auCharacter.h"
#include "auFontRendering.h"
#include <cmath>
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
  // std::cout << "NB vertex : " << outline.n_points << std::endl;
  // for (int i = 0; i < outline.n_contours; i++) {
  //   std::cout << "contours " << i << " : " << outline.contours[i] <<
  //   std::endl;
  // }

  short indexOfContour = 0;
  short contourStart = 0;
  uint coutourStartExtended = 0;

  for (short i_point = 0; i_point < outline.n_points; i_point++) {

    if (outline.tags[i_point] == FT_CURVE_TAG_ON) {
      verticies.push_back(outline.points[i_point]);

    } else if (outline.tags[i_point] == FT_CURVE_TAG_CONIC) {

      // TODO:Two successive conic ‘off’ points force the rasterizer to create
      // (during the scan-line conversion process exclusively) a virtual ‘on’
      // point inbetween, at their exact middle. This greatly facilitates the
      // definition of successive conic Bézier arcs. Moreover, it is the way
      // outlines are described in the TrueType specification.

      std::vector<FT_Vector_> bezierCurvePoints;
      // TODO:
      // If 2 FT_CURVE_TAG_CONIC folowed each other (FreeType doc)
      // Maybe wrap this in a function backToBackCubic to simplify the set
      // resolution.
      if (outline.tags[i_point + 1] == FT_CURVE_TAG_CONIC) {
        FT_Vector_ virtualPoint = {
            lerp(outline.points[i_point].x, outline.points[i_point + 1].x, 0.5),
            lerp(outline.points[i_point].y, outline.points[i_point + 1].y,
                 0.5)};

        bezierCurvePoints = bezierPoints(interval, outline.points[i_point - 1],
                                         virtualPoint, outline.points[i_point]);

        // NOTE: need to pass by intermediate vector to add the seconde conic
        // curve points to the original one very wanky
        std::vector<FT_Vector_> scdBezierCurvePoints =
            bezierPoints(interval, virtualPoint, outline.points[i_point + 2],
                         outline.points[i_point + 1]);

        bezierCurvePoints.insert(bezierCurvePoints.end(),
                                 scdBezierCurvePoints.begin(),
                                 scdBezierCurvePoints.end());

        i_point++;
      } else {
        bezierCurvePoints =
            bezierPoints(interval, outline.points[i_point - 1],
                         outline.points[i_point + 1], outline.points[i_point]);
      }

      verticies.insert(verticies.end(), bezierCurvePoints.begin(),
                       bezierCurvePoints.end());

    } else if (outline.tags[i_point] == FT_CURVE_TAG_CUBIC) {
      // There is no cubic tag on the default font for the first 126 ascii
      // character
      std::cout << "CUBIC" << std::endl;
    }

    if (i_point == outline.contours[indexOfContour]) {

      // contourStart -> index of the start of the countour
      // outline.contours[indexOfContour] -> index of the end of the contour

      FT_Vector_ *start = outline.points + contourStart;
      int count = outline.contours[indexOfContour] - contourStart + 1;
      float sign = shoelace(start, count);

      if (sign < 0) {
        if (outerContour.empty()) {
          // add the points from index contourStart to index
          // outline.contours[indexOfContour] to outerPolygone
          //
          // If the inner polygone was not emplty it means there is multiple
          // outerPolygone and i don't know how to handle that
          // Insert in outer contour the verticies of the outer contour.
          outerContour.insert(outerContour.end(),
                              verticies.begin() + coutourStartExtended,
                              verticies.end());
        }

      } else if (sign > 0) {
        // add the points from index contourStart to index
        // outline.contours[indexOfContour] to the list of innerPolygon

        std::vector<FT_Vector_> innerContour;
        innerContour.insert(innerContour.end(),
                            verticies.begin() + coutourStartExtended,
                            verticies.end());

        innerContours.push_back(innerContour);

      } else if (sign == 0) {
        std::cout << "Not inner or outer polygone: DO NOTHING" << std::endl;
      }

      contourStart = outline.contours[indexOfContour] + 1;
      // The start of the next coutour is the actual index +1 = the size at that
      // moment.
      coutourStartExtended = verticies.size();
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

  std::cout << "Size list inner poly: " << listInerPolygon.size() << std::endl;

  if (listInerPolygon.size() != 0) {
    // The goal i to transform the polygone with hole in a pseudo simple
    // polygone on witch earClipping can be used
    outerVertices = bridge(outerVertices, listInerPolygon);
  }

  earClipping(outerVertices);
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

    std::cout << "M=(" << m.x << "," << m.y << ") indexMax=" << indexMax
              << "\n";
    std::cout << "P=(" << P.x << "," << P.y << ") I=(" << I.x << "," << I.y
              << ")\n";
    std::cout << "R=(" << R.x << "," << R.y << ") indexR=" << indexR << "\n";

    std::cout << "BEFORE MERGE" << std::endl;
    std::cout << std::endl;
    for (FT_Vector_ v : outerVertices) {
      std::cout << "(x: " << v.x << " ,y:" << v.y << ")";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    for (FT_Vector_ v : innerPolygon) {
      std::cout << "(x: " << v.x << " ,y:" << v.y << ")";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    std::vector<FT_Vector_> merged;
    merged.reserve(outerVertices.size() + innerPolygon.size() + 2);

    // 1) copy outer up to indexR
    merged.insert(merged.end(), outerVertices.begin(),
                  outerVertices.begin() + indexR + 1);

    // 2) insert inner starting "after M" wrapping around, and ending at M
    int n = (int)innerPolygon.size();
    int start = (indexMax + 1) % n;

    for (int i = start; i < n; ++i)
      merged.push_back(innerPolygon[i]);

    for (int i = 0; i <= indexMax; ++i)
      merged.push_back(innerPolygon[i]);

    // 3) repeat the first inserted inner vertex to close the hole walk
    merged.push_back(innerPolygon[start]);

    // 4) repeat outer[indexR] to create the second bridge edge back
    merged.push_back(outerVertices[indexR]);

    // 5) copy the rest of outer (after indexR)
    merged.insert(merged.end(), outerVertices.begin() + indexR + 1,
                  outerVertices.end());

    // now merged is your new outer polygon
    outerVertices = std::move(merged);

    std::cout << "AFTER MERGE" << std::endl;
    std::cout << std::endl;
    for (FT_Vector_ v : outerVertices) {
      std::cout << "(x: " << v.x << " ,y:" << v.y << ")";
    }
    std::cout << std::endl;
    std::cout << std::endl;
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

float auCharacter::area(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c) {
  return abs((a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) / 2.0);
}

#include <cmath>

bool auCharacter::isInsideTriangle(FT_Vector_ A, FT_Vector_ B, FT_Vector_ C,
                                   FT_Vector_ P) {
  float big = area(A, B, C);
  float a1 = area(P, B, C);
  float a2 = area(A, P, C);
  float a3 = area(A, B, P);

  const float eps = 1e-3f; // adjust if your coordinates are large
  return std::fabs(big - (a1 + a2 + a3)) <= eps;
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

bool auCharacter::intersectWithPolygone(const std::vector<FT_Vector_> &polygone,
                                        int i) {}

void auCharacter::earClipping(std::vector<FT_Vector_> verticies) {}
