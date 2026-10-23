#pragma once
#include <memory>
#include <string>

#include "auColor.h"
#include "auVector2.h"

struct GLFWwindow;

class auFontRenderer {
public:
  auFontRenderer(GLFWwindow *window, auVector2 position, int size,
                 auColor color,
                 const std::string &shader_directory = "ressources");
  ~auFontRenderer();

  void loadFont();
  int auSetText(std::string text);
  int setFont(std::string path);
  int auDraw(std::string letter);

private:
  class Impl;
  std::unique_ptr<Impl> _impl;
};
