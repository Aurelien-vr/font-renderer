#include <optional>
#include <string>
#define GLFW_INCLUDE_NONE
#include "../lib/GLFW/glfw3.h"
#include "../lib/glad.h"
#include "Shader.h"
#include "auFontRendering.h"
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
  // NOTE: Remove VSync that cap fps to the max monitor refresh rate
  glfwSwapInterval(0);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glViewport(0, 0, 800, 600);

  double elapseTime = 1;
  std::optional<double> currentTimeStamp;
  float fps = 0;

  auFontRendering font = auFontRendering(window, {0, 0}, 98, {1, 1, 1});
  font.loadFont();

  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // while (!glfwWindowShouldClose(window)) {
  //
  //   // fps conter
  //   double currentTime = glfwGetTime();
  //   if (currentTimeStamp.has_value()) {
  //     double seconds = currentTime - currentTimeStamp.value();
  //     fps = 1 / seconds;
  //   }
  //   currentTimeStamp = currentTime;
  //
  //   font.auDraw();
  //
  //   glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  //   glClear(GL_COLOR_BUFFER_BIT);
  //
  //   font.shaders.Activate();
  //
  //   glfwSwapBuffers(window);
  //   glfwPollEvents();
  // }
  //

  while (!glfwWindowShouldClose(window)) {
    // 1. clear first
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 2. then draw
    font.auDraw("x");

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  font.shaders.Delete();
  return 0;
}
