#pragma once
#include "../lib/freetype2/freetype/fttypes.h"
#include "DrawElementsIndirectCommand.h"
#include "Shader.h"
#include "auGlyph.h"
#include <freetype/ftimage.h>
#include <ft2build.h>
#include <string>
#include <vector>

#include "EBO.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"

#include FT_FREETYPE_H

#include "../lib/GLFW/glfw3.h"
#include "DIB.h"
#include "auColor.h"
#include "auVector2.h"

class auFontRenderer {
public:
  GLFWwindow *window;
  Shader shaders;

  auFontRenderer(GLFWwindow *window, auVector2 position, int size,
                 auColor color);
  ~auFontRenderer();

  void loadFont();
  int auSetText(std::string text);
  int setFont(std::string path);
  int auDraw(std::string letter);

private:
  int size;
  auVector2 position;
  auColor color;

  int screenHeight, screenWidth;

  FT_Vector_ offsetBuffer{0, 0};

  VAO *vao = nullptr;
  VBO *vbo = nullptr;
  EBO *ebo = nullptr;
  DIB *dib = nullptr;

  // All the verticies of all the glyph to draw
  // Needed to send all the data at once to the VBO
  std::vector<float> sentence;

  // Liste of the characters containing the data for the drawing as a list of
  // object auCharacters
  std::vector<auCharacter> listCharacters;
  std::vector<FT_Vector_> listOffset;
  std::vector<DrawElementsIndirectCommand> cmdBuffer;

  FT_Library library;
  FT_Face face;

  int auInitFreeType();
  int auLoadFace();
  int auLoadFace(std::string path);
  int getGlyph(FT_ULong charcode);
  void clearBuffer();
  void onResize();
  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height);
};

// Are the parameter adressed by indirect in the funtion
// glMultiDrawElementsIndirect
