#pragma once

#include "../lib/freetype2/freetype/freetype.h"
#include "../lib/freetype2/freetype/ftimage.h"
#include "../lib/freetype2/freetype/fttypes.h"
#include "auVector2.h"
#include <freetype/ftimage.h>
#include <vector>

class auCharacter {
public:
  std::vector<FT_Vector_> verticies;
  std::vector<FT_Vector_> verticiesTriangles;
  std::vector<FT_Vector_> outerContour;
  std::vector<short> outlineEnd;
  std::vector<std::vector<FT_Vector_>> innerContours;

  float offset_letter;
  bool compound;
  FT_Face face;
  short resolution;

  auCharacter(FT_Face face, short resolution);

  void setResolution();

  void setVerticies(int size, auVector2 position, float space);
  void setOutlineEnd();
  int getSize() { return verticies.size(); };

private:
  std::vector<FT_Vector_> bezierPoints(float interval, FT_Vector_ start,
                                       FT_Vector_ end, FT_Vector_ pivot);
  long lerp(float a, float b, float f);

  void triangulation(std::vector<FT_Vector_> outerVertices,
                     std::vector<std::vector<FT_Vector_>> listInerPolygon);

  // Double linked list
  std::vector<FT_Vector_>
  bridge(std::vector<FT_Vector_> outerVertices,
         std::vector<std::vector<FT_Vector_>> listInerPolygon);

  void earClipping(std::vector<FT_Vector_> vertices);

  float distance(FT_Vector_ a, FT_Vector_ b);
  // This function return a negative value if the verticies of the subOutline is
  // clockwise and a positive value is conterclockwise
  float shoelace(FT_Vector *points, int count);
  float area(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c);
  bool isInsideTriangle(FT_Vector_ A, FT_Vector_ B, FT_Vector_ C, FT_Vector_ P);
  float angleBetweenAtM(FT_Vector_ M, FT_Vector_ I, FT_Vector_ R);
  // Take refference of the polygone vector and the index of the angle we want
  // the information for
  bool isReflexOuterVertex(const std::vector<FT_Vector_> &outer, int i);
  bool intersectWithPolygone(const std::vector<FT_Vector_> &polygone, int i);
};
