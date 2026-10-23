#include "Shader.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <sstream>

std::string readShaderFile(const std::string &filePath) {
  std::ifstream fileStream(filePath);
  if (!fileStream.is_open()) {
    std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath
              << std::endl;
    return "";
  }

  std::stringstream stringStream;
  stringStream << fileStream.rdbuf();
  fileStream.close();
  return stringStream.str();
}

Shader::Shader(std::string vertPath, std::string fragPath) {
  std::string vertCode = readShaderFile(vertPath);
  std::string fragCode = readShaderFile(fragPath);

  const char *vertexSource = vertCode.c_str();
  const char *fragmentSource = fragCode.c_str();

  int sucess;
  char infoLog[512];

  if (glfwGetCurrentContext() == NULL) {
    std::cerr << "FATAL::ERROR Can't use openGL function without openGL context"
              << std::endl;
  }

  // Vertex Shader
  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  // Fragement Shader
  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  // Error handleing
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &sucess);
  if (!sucess) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog
              << std::endl;
  }

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &sucess);
  if (!sucess) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::COMPILATION_FAILED" << infoLog << std::endl;
  }

  // Program linking
  ID = glCreateProgram();

  glAttachShader(ID, vertexShader);
  glAttachShader(ID, fragmentShader);
  glLinkProgram(ID);

  // Program Error handleing
  glGetProgramiv(ID, GL_LINK_STATUS, &sucess);
  if (!sucess) {
    glGetProgramInfoLog(ID, 512, NULL, infoLog);
    std::cout << "ERROR::PROGRAM::COMPILATION_FAILED" << infoLog << std::endl;
  }

  // Once the shader are bind to the program they are no longer useful and can
  // be deleted
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

void Shader::Activate() { glUseProgram(ID); }

void Shader::Delete() { glDeleteProgram(ID); }

void Shader::SetVec2(const std::string &name, float x, float y) {
  glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void Shader::SetInt(const std::string &name, int val) {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), val);
}

void Shader::SetFloat(const std::string &name, float val) {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), val);
}
