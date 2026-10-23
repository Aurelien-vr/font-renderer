#include "auFontRenderer.h"
#include "DIB.h"
#include "DrawElementsIndirectCommand.h"
#include "EBO.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "auGlyph.h"
#include "auColor.h"
#include "liberation_sans.h"
#include <freetype/freetype.h>
#include <freetype/fttypes.h>

#include <GLFW/glfw3.h>
#include <cstddef>
#include <filesystem>
#include <freetype/ftimage.h>
#include <numeric>
#include <vector>

class auFontRenderer::Impl {
public:
  Impl(GLFWwindow *window, auVector2 position, int size, auColor color,
       const std::string &shader_directory)
      : window(window),
        shaders((std::filesystem::path(shader_directory) / "default.vert")
                    .string(),
                (std::filesystem::path(shader_directory) / "default.frag")
                    .string()),
        size(size), position(position), color(color) {}

  GLFWwindow *window;
  Shader shaders;
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

  FT_Library library = nullptr;
  FT_Face face = nullptr;

  int auInitFreeType();
  int auLoadFace();
  int auLoadFace(std::string path);
  int getGlyph(FT_ULong charcode);
  void clearBuffer();
  void onResize();
  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height);
  ~Impl();
};

auFontRenderer::auFontRenderer(GLFWwindow *window, auVector2 position, int size,
                               auColor color,
                               const std::string &shader_directory)
    : _impl(
          std::make_unique<Impl>(window, position, size, color, shader_directory)) {

  glfwSetWindowUserPointer(window, _impl.get());
  glfwSetFramebufferSizeCallback(window, Impl::framebuffer_size_callback);

  _impl->auInitFreeType();
  _impl->auLoadFace();
}

auFontRenderer::~auFontRenderer() = default;

auFontRenderer::Impl::~Impl() {
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

int auFontRenderer::Impl::auInitFreeType() {
  return !FT_Init_FreeType(&library);
}

int auFontRenderer::Impl::auLoadFace() {
  if (face) {
    FT_Done_Face(face);
    face = nullptr;
  }
  return FT_New_Memory_Face(library, LiberationMono_Regular_ttf,
                            LiberationMono_Regular_ttf_len, 0, &face);
}

int auFontRenderer::Impl::auLoadFace(std::string path) {
  if (face) {
    FT_Done_Face(face);
    face = nullptr;
  }
  return FT_New_Face(library, path.c_str(), 0, &face);
}

int auFontRenderer::Impl::getGlyph(FT_ULong charcode) {
  FT_UInt index = FT_Get_Char_Index(face, charcode);
  return FT_Load_Glyph(
      face, index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING);
}

void auFontRenderer::loadFont() {

  _impl->clearBuffer();
  int offset = 0;
  _impl->sentence.clear();
  _impl->listCharacters.clear();
  _impl->listOffset.clear();
  _impl->cmdBuffer.clear();

  glfwGetFramebufferSize(_impl->window, &_impl->screenWidth,
                         &_impl->screenHeight);

  for (FT_ULong c = 0; c < 127; c++) {
    // Set the face in the variable face
    // It is then pass to the character to retrive the outline points
    _impl->getGlyph(c);

    if (_impl->face != NULL) {
      _impl->listOffset.push_back(_impl->face->glyph->advance);
    }

    auCharacter chara = auCharacter(_impl->face, 20);
    chara.setResolution();

    // Use to keep the characters if needed later (Not used for now)
    _impl->listCharacters.push_back(chara);

    // Put the verticies of the character inside a large list for the VBO
    for (FT_Vector_ point : chara.verticesTriangles) {
      _impl->sentence.push_back(point.x);
      _impl->sentence.push_back(point.y);
    }

    _impl->cmdBuffer.push_back(
        {uint32_t(chara.verticesTriangles.size()), 1, uint32_t(offset), 0, 0});

    offset += chara.verticesTriangles.size();
  }

  _impl->vao = new VAO();
  _impl->vao->Bind();

  // Create a VBO and bind it
  _impl->vbo = new VBO(_impl->sentence.data(),
                       _impl->sentence.size() * sizeof(float));

  _impl->vao->LinkVBO(*_impl->vbo, 0);

  // Create a EBO with increasing value (0,1,2,3,...)
  // The EBO is useless but needed in the setup
  // it is a dummy EBO
  // In the meantime of a efficent EBO
  int total_vertices = _impl->sentence.size() / 2;
  std::vector<GLuint> indices(total_vertices);
  std::iota(indices.begin(), indices.end(), 0);

  _impl->ebo = new EBO(indices);

  _impl->dib = new DIB(_impl->cmdBuffer.data(),
                       _impl->cmdBuffer.size() *
                           sizeof(DrawElementsIndirectCommand));

  _impl->vao->Unbind();
  _impl->vbo->Unbind();
  _impl->ebo->Unbind();
  _impl->dib->Unbind();
}

int auFontRenderer::auDraw(std::string letter) {
  _impl->shaders.Activate();

  _impl->shaders.SetVec2("position", _impl->position.x, _impl->position.y);
  _impl->shaders.SetFloat("uPtSize", static_cast<float>(_impl->size));
  _impl->shaders.SetInt("uEM_size", _impl->face->units_per_EM);
  _impl->shaders.SetVec2("uScreenSize", static_cast<float>(_impl->screenWidth),
                         static_cast<float>(_impl->screenHeight));

  _impl->offsetBuffer = {0, 0};

  _impl->vao->Bind();
  _impl->dib->Bind();

  for (unsigned char c : letter) {
    _impl->shaders.SetVec2("offset", static_cast<float>(_impl->offsetBuffer.x),
                           static_cast<float>(_impl->offsetBuffer.y));

    glMultiDrawElementsIndirect(
        GL_TRIANGLES, GL_UNSIGNED_INT,
        reinterpret_cast<void *>(static_cast<size_t>(c) *
                                 sizeof(DrawElementsIndirectCommand)),
        1, sizeof(DrawElementsIndirectCommand));

    _impl->offsetBuffer.x += _impl->listOffset[c].x;
    _impl->offsetBuffer.y += _impl->listOffset[c].y;
  }

  return 0;
}

void auFontRenderer::Impl::clearBuffer() {
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

void auFontRenderer::Impl::framebuffer_size_callback(GLFWwindow *window,
                                                     int width, int height) {
  glViewport(0, 0, width, height);
  // IA magic need to better understand how it works
  auto *self = static_cast<Impl *>(glfwGetWindowUserPointer(window));
  self->shaders.Activate();
  self->shaders.SetVec2("uScreenSize", width, height);
}
