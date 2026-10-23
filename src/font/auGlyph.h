#pragma once

#include "../lib/freetype2/freetype/freetype.h"
#include "../lib/freetype2/freetype/ftimage.h"
#include "../lib/freetype2/freetype/fttypes.h"
#include "auVector2.h"
#include <freetype/ftimage.h>
#include <vector>

class auCharacter {
public:
  /*
   * VARIABLE
   */

  // List of the verticies from freeType
  std::vector<FT_Vector_> vertices;
  // List of the vertices that form triangle after bezier and earCLipping
  std::vector<FT_Vector_> verticesTriangles;
  // List of outer contour after bezier
  std::vector<FT_Vector_> outerContour;
  // List of inner contour after bezier
  std::vector<std::vector<FT_Vector_>> innerContours;
  // List of ouline end index after bezier
  std::vector<short> outlineEnd;

  float offset_letter;
  bool compound;
  FT_Face face;
  short resolution;

  /*
   * FUNCTIONs
   */

  auCharacter(FT_Face face, short resolution);

  void setResolution();

  void setVertices(int size, auVector2 position, float space);

  void setOutlineEnd();

  int getSize() { return vertices.size(); };

private:
  /*
   * MAIN FUNCTION
  r a valid ear, you typically need points to NOT be inside the triangle
   */

  void triangulation(std::vector<FT_Vector_> outerVertices,
                     std::vector<std::vector<FT_Vector_>> listInerPolygon);

  // Double linked list
  std::vector<FT_Vector_>
  bridge(std::vector<FT_Vector_> outerVertices,
         std::vector<std::vector<FT_Vector_>> listInerPolygon);

  void earClipping(std::vector<FT_Vector_> vertices,
                   std::vector<FT_Vector_> &triangleOutput);

  ulong findEar(std::vector<FT_Vector_> vertices);
};
