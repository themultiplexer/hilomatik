#version 330 core

uniform vec3 circleColor;

void main() {
    gl_FragColor = vec4(circleColor, 1.0);
}