#pragma once

#include <glad.h>

class VBO {
public:
  GLuint ID;
  VBO(GLfloat *verticies, GLsizeiptr size);

  void Bind();
  void Unbind();
  void Delete();
};
