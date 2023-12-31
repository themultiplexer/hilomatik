#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout (location = 0) in vec3 inPosition;
layout (location = 0) in vec2 uvs;

uniform vec2 iResolution;
uniform float vol;
out float volume;
out vec2 vuv;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    gl_PointSize = 5.0;
    vuv = uvs;
    volume = vol;
}