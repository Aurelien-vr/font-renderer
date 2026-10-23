#include <string>
#include <sys/types.h>
#define GLFW_INCLUDE_NONE
#include "../lib/GLFW/glfw3.h"
#include "../lib/glad.h"
#include "Shader.h"
#include "auFontRenderer.h"
#include <cmath>
#include <cstdio>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

int main() {
  // GLFW INIT
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_FLOATING, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(800, 600, "Cosmos", NULL, NULL);

  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  // Remove VSync that cap fps to the max monitor refresh rate
  glfwSwapInterval(0);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glViewport(0, 0, 800, 600);

  double fpsTimer = glfwGetTime();
  int frameCount = 0;
  float fps = 0.0f;

  auFontRenderer font = auFontRenderer(window, {100, 300}, 64, {1, 1, 1});
  font.loadFont();

  while (!glfwWindowShouldClose(window)) {
    // 1. clear first
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // 2. then draw
    static std::string fpsText = "FPS: 0";

    frameCount++;

    double currentTime = glfwGetTime();
    double elapsed = currentTime - fpsTimer;

    if (elapsed >= 1.0) {
      fps = static_cast<float>(frameCount / elapsed);

      fpsText = std::to_string(static_cast<int>(std::lround(fps)));

      frameCount = 0;
      fpsTimer = currentTime;
    }

    font.auDraw(fpsText);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  font.shaders.Delete();
  glfwTerminate();
  return 0;
}
