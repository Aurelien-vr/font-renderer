#pragma once

#include <glad.h>
#include "DrawElementsIndirectCommand.h"

class DIB {
public:
  GLuint ID;

  DIB(DrawElementsIndirectCommand *cmdBuffer, GLsizeiptr size);

  void Bind();
  void Unbind();
  void Delete();
};
