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
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }
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
  // Pace rendering to the display refresh rate instead of busy-looping.
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  glViewport(0, 0, 800, 600);

  double currentTimeStamp = glfwGetTime();
  float fps = 0;

  {
    auFontRenderer font = auFontRenderer(window, {100, 300}, 64, {1, 1, 1});
    font.loadFont();

    while (!glfwWindowShouldClose(window)) {
      // 1. clear first
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      // 2. then draw
      double currentTime = glfwGetTime();
      double seconds = currentTime - currentTimeStamp;
      fps = 1 / seconds;
      currentTimeStamp = currentTime;

      static int lastSecond = -1;
      static std::string fpsText;

      if (static_cast<int>(currentTime) != lastSecond) {
        lastSecond = static_cast<int>(currentTime);
        fpsText = std::to_string(static_cast<int>(std::lround(fps)));
      }

      font.auDraw(fpsText);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
