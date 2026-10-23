#pragma once

#include "VBO.h"
#include <glad.h>

class VAO {
public:
  GLuint ID;
  VAO();

  void LinkVBO(VBO &VBO, GLuint layout);

  void Bind();
  void Unbind();
  void Delete();
};
