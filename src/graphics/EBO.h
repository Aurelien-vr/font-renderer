#pragma once

#include "../lib/glad.h"
#include <vector>

class EBO {
public:
  GLuint ID;
  EBO(std::vector<GLuint> &indices);

  void Bind();
  void Unbind();
  void Delete();
};
