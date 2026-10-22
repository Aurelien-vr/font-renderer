#pragma once
#include <freetype/freetype.h>
#include <vector>

namespace earClipping {
// segment polygone[idx] -> target leaves idx inside the interior angle
bool locallyInside(const std::vector<FT_Vector_> &polygone, int idx,
                   FT_Vector_ target);

// midpoint of a-b is inside the polygone
bool middleInside(const std::vector<FT_Vector_> &polygone, FT_Vector_ a,
                  FT_Vector_ b);

float distance(FT_Vector_ a, FT_Vector_ b);

// This function return a negative value if the vertices of the subOutline is
// clockwise and a positive value is conterclockwise
float shoelace(FT_Vector *points, int count);

// Compute the area of a triangle
long area(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c);

// Return True if P is inside the triangle ABC (ABC and P are points not
// index)
bool isInsideTriangle(FT_Vector_ A, FT_Vector_ B, FT_Vector_ C, FT_Vector_ P);

// Return true if a points of polygone is inside the triable i-1, i, i+1
bool isPointsInsideTriangle(const std::vector<FT_Vector_> &polygon, int i);

float angleBetweenAtM(FT_Vector_ M, FT_Vector_ I, FT_Vector_ R);

// Take refference of the polygone vector and the index of the angle we want
// the information for
bool isReflexOuterVertex(const std::vector<FT_Vector_> &outer, int i);

// Use intersect to knwo if a segment i-1 and i+1 intersect with any edge of
// the polygone
bool intersectWithPolygone(const std::vector<FT_Vector_> &polygone, int i);

// Check if the segment i-1 and i+1 is inside the polygone
bool isFullyInsidePolygone(const std::vector<FT_Vector_> &polygone, int i);

// Return true if a vertices of "polygone" is inside the triangle i-1, i, i+1
bool isVerticesInside(const std::vector<FT_Vector_> &polygone, int i);

// return if there is a proper intersection and does not account for
// degenernecy (used in intersect)
bool intersectProp(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c, FT_Vector_ d);

// Return true if AB and CD intersect
bool intersect(FT_Vector_ a, FT_Vector_ b, FT_Vector_ c, FT_Vector_ d);

bool collinear(FT_Vector_ *a, FT_Vector_ *b, FT_Vector_ *c);

bool between(FT_Vector_ *a, FT_Vector_ *b, FT_Vector_ *c);

} // namespace earClipping
