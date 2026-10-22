#pragma once

#include "VBO.h"
#include "../lib/glad.h"

class VAO {
public:
  GLuint ID;
  VAO();

  void LinkVBO(VBO &VBO, GLuint layout);

  void Bind();
  void Unbind();
  void Delete();
};
