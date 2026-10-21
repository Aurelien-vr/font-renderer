#include "auFontRenderer.h"
#include "../lib/freetype2/freetype/freetype.h"
#include "../lib/freetype2/freetype/fttypes.h"
#include "DIB.h"
#include "DrawElementsIndirectCommand.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "auGlyph.h"
#include "liberation_sans.h"

#include <GLFW/glfw3.h>
#include <cstddef>
#include <freetype/ftimage.h>
#include <numeric>
#include <vector>

auFontRendering::auFontRendering(GLFWwindow *window, auVector2 position,
                                 int size, auColor color)
    : window(window),
      shaders("ressources/default.vert", "ressources/default.frag"), size(size),
      position(position), color(color) {

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  auInitFreeType();
  auLoadFace();
}

auFontRendering::~auFontRendering() {
  shaders.Delete();
  if (face)
    FT_Done_Face(face);
  if (library)
    FT_Done_FreeType(library);

  delete vao;
  delete vbo;
  delete ebo;
  delete dib;
  vao = nullptr;
  vbo = nullptr;
  ebo = nullptr;
  dib = nullptr;
}

int auFontRendering::auInitFreeType() { return !FT_Init_FreeType(&library); }

int auFontRendering::auLoadFace() {
  return FT_New_Memory_Face(library, LiberationMono_Regular_ttf,
                            LiberationMono_Regular_ttf_len, 0, &face);
}

int auFontRendering::auLoadFace(std::string path) {
  return FT_New_Face(library, path.c_str(), 0, &face);
}

int auFontRendering::getGlyph(FT_ULong charcode) {
  FT_UInt index = FT_Get_Char_Index(face, charcode);
  return FT_Load_Glyph(
      face, index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING);
}

void auFontRendering::loadFont() {

  int offset = 0;

  clearBuffer();

  glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

  for (FT_ULong c = 0; c < 127; c++) {
    // Set the face in the variable face
    // It is then pass to the character to retrive the outline points
    getGlyph(c);

    if (face != NULL) {
      listOffset.push_back(face->glyph->advance);
    }

    auCharacter chara = auCharacter(face, 20);
    chara.setResolution();

    // Use to keep the characters if needed later (Not used for now)
    listCharacters.push_back(chara);

    // Put the verticies of the character inside a large list for the VBO
    for (FT_Vector_ point : chara.verticesTriangles) {
      sentence.push_back(point.x);
      sentence.push_back(point.y);
    }

    cmdBuffer.push_back(
        {uint32_t(chara.verticesTriangles.size()), 1, uint32_t(offset), 0, 0});

    offset += chara.verticesTriangles.size();
  }

  vao = new VAO();
  vao->Bind();

  // Create a VBO and bind it
  vbo = new VBO(sentence.data(), sentence.size() * sizeof(float));

  vao->LinkVBO(*vbo, 0);

  // Create a EBO with increasing value (0,1,2,3,...)
  // The EBO is useless but needed in the setup
  // it is a dummy EBO
  // In the meantime of a efficent EBO
  int total_vertices = sentence.size() / 2;
  std::vector<GLuint> indices(total_vertices);
  std::iota(indices.begin(), indices.end(), 0);

  ebo = new EBO(indices);

  dib = new DIB(cmdBuffer.data(),
                cmdBuffer.size() * sizeof(DrawElementsIndirectCommand));

  vao->Unbind();
  vbo->Unbind();
  ebo->Unbind();
  dib->Unbind();
}

int auFontRendering::auDraw(std::string letter) {
  shaders.Activate();

  shaders.SetVec2("position", position.x, position.y);
  shaders.SetFloat("uPtSize", static_cast<float>(size));
  shaders.SetInt("uEM_size", face->units_per_EM);
  shaders.SetVec2("uScreenSize", static_cast<float>(screenWidth),
                  static_cast<float>(screenHeight));

  offsetBuffer = {0, 0};

  vao->Bind();
  dib->Bind();

  for (unsigned char c : letter) {
    shaders.SetVec2("offset", static_cast<float>(offsetBuffer.x),
                    static_cast<float>(offsetBuffer.y));

    glMultiDrawElementsIndirect(
        GL_TRIANGLES, GL_UNSIGNED_INT,
        reinterpret_cast<void *>(static_cast<size_t>(c) *
                                 sizeof(DrawElementsIndirectCommand)),
        1, sizeof(DrawElementsIndirectCommand));

    offsetBuffer.x += listOffset[c].x;
    offsetBuffer.y += listOffset[c].y;
  }

  return 0;
}

void auFontRendering::clearBuffer() {
  if (vao != NULL) {
    delete vao;
    vao = nullptr;
  }

  if (vbo != NULL) {
    delete vbo;
    vbo = nullptr;
  }

  if (ebo != NULL) {
    delete ebo;
    ebo = nullptr;
  }

  if (dib != NULL) {
    delete dib;
    dib = nullptr;
  }
}

void auFontRendering::framebuffer_size_callback(GLFWwindow *window, int width,
                                                int height) {
  glViewport(0, 0, width, height);
  // IA magic need to better understand how it works
  auto *self = static_cast<auFontRendering *>(glfwGetWindowUserPointer(window));
  self->shaders.Activate();
  self->shaders.SetVec2("uScreenSize", width, height);
}
