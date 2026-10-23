#pragma once
#include "glad.h"
#include <string>

std::string readShaderFile(const std::string &filePath);

class Shader {
public:
  unsigned int ID;

  Shader(std::string vertPath, std::string fracPath);
  void Activate();
  void Delete();
  void SetVec2(const std::string &name, float x, float y);
  void SetInt(const std::string &name, int val);
  void SetFloat(const std::string &name, float val);
};
