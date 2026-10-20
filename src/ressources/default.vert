#version 460 core

layout(location = 0) in vec2 aPos;

uniform int uEM_size;
uniform vec2 uScreenSize;
uniform float uPtSize;
uniform vec2 offset;
uniform vec2 position;

const float DPI = 144.0;

void main() {
  float scale =
    (uPtSize * DPI / 72.0) / float(uEM_size);

  // Font units -> pixels.
  vec2 glyphPixels = (aPos + offset) * scale;

  // Add screen-space position after scaling.
  vec2 pixelPosition = glyphPixels + position;

  float ndc_x =
    (pixelPosition.x / uScreenSize.x) * 2.0 - 1.0;

  float ndc_y =
    1.0 - (pixelPosition.y / uScreenSize.y) * 2.0;

  gl_Position = vec4(ndc_x, ndc_y * -1, 0.0, 1.0);
}
