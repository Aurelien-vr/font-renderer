#version 460 core
out vec4 FragColor;

vec3 idToColor(int id) {
  float r = fract(sin(float(id) * 12.9898) * 43758.5453);
  float g = fract(sin(float(id) * 78.233) * 43758.5453);
  float b = fract(sin(float(id) * 37.719) * 43758.5453);
  return vec3(r, g, b);
}

void main()
{
  FragColor = vec4(idToColor(gl_PrimitiveID), 1.0);
  // FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
