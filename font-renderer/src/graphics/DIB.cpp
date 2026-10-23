#include "DIB.h"

DIB::DIB(DrawElementsIndirectCommand *cmdBuffer, GLsizeiptr size) {
  glGenBuffers(1, &ID);

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ID);

  glBufferData(GL_DRAW_INDIRECT_BUFFER, size, cmdBuffer, GL_DYNAMIC_DRAW);
}

void DIB::Bind() { glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ID); }

void DIB::Unbind() { glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0); }

void DIB::Delete() { glDeleteBuffers(1, &ID); }
