#version 330 core

in vec2 inPosition;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    gl_FrontColor = vec4(1.0,1.0,1.0, 1.0);
    gl_PointSize = 8.0f;
}