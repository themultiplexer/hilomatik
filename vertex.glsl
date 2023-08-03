#version 130
in vec4 position;
out vec2 uv;

void main() {
  gl_Position = position;
  uv = (position.xy + 1.0) / 2.0;
}