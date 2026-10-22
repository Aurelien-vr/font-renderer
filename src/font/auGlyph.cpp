#include "auGlyph.h"
#include "bezier.h"
#include "earClipping.h"
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
        start = {geometry::lerp(start.x, control.x, 0.5),
                 geometry::lerp(start.y, control.y, 0.5)};
      }

      // Next point is also off-curve: the curve ends at the implied
      // on-curve point between them (their midpoint)
      if (!nextIsOn) {
        end = {geometry::lerp(control.x, end.x, 0.5),
               geometry::lerp(control.y, end.y, 0.5)};
      }

      std::vector<FT_Vector_> bezierCurvePoints =
          geometry::bezierPoints(interval, start, end, control);

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
      float sign = earClipping::shoelace(start, count);

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

void auCharacter::triangulation(
    std::vector<FT_Vector_> outerVertices,
    std::vector<std::vector<FT_Vector_>> listInerPolygon) {

  // Outer contour must be CCW (shoelace > 0) : Verification
  if (earClipping::shoelace(outerVertices.data(), (int)outerVertices.size()) <
      0) {
    std::reverse(outerVertices.begin(), outerVertices.end());
  }

  // Holes must be CW (shoelace < 0) : Verification
  for (std::vector<FT_Vector_> &hole : listInerPolygon) {
    if (earClipping::shoelace(hole.data(), (int)hole.size()) > 0) {
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
      FT_Vector_ i = {geometry::lerp(vertexA.x, vertexB.x, fValue), m.y};

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

      if (!earClipping::isInsideTriangle(m, I, P, v))
        continue;

      // Only reflex vertices of the OUTER polygon
      if (!earClipping::isReflexOuterVertex(outerVertices, index))
        continue;

      float currentAngle = earClipping::angleBetweenAtM(m, I, v);
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

    if (earClipping::isReflexOuterVertex(vertices, i)) {
      continue;
    }

    if (earClipping::isPointsInsideTriangle(vertices, i)) {
      continue;
    }

    if (!earClipping::isFullyInsidePolygone(vertices, i)) {
      continue;
    }

    return i;
  }

  return ULONG_MAX;
}
