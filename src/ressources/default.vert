#version 460 core
layout(location = 0) in vec2 aPos;

uniform int uEM_size;
uniform vec2 uScreenSize;
uniform float uPtSize;

const float DPI = 144.0f; //96 * 1.5

void main() {
  float scale = (uPtSize * DPI / 72.0f) / float(uEM_size);

  float ndc_x = ((aPos.x * scale) / uScreenSize.x) * 2.0f - 1.0f;
  float ndc_y = (1.0f - ((aPos.y * scale) / uScreenSize.y) * 2.0f) * -1;

  gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);
}
