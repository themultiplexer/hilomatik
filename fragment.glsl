#version 130
in vec2 uv;
out vec4 color;

void main() {
  color = vec4(uv, 0.0, 1.0);
}