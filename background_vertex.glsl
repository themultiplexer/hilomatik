#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout (location = 0) in vec3 inPosition;
layout (location = 0) in vec2 uvs;
out vec2 vuv;

uniform vec2 iResolution;
out float volume;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    vuv = uvs;
}