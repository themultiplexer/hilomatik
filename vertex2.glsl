#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec3 v_bary;

uniform vec2 uResolution; // = (window-width, window-height)
out float volume;

out vec3 v_bc;

void main() {
    vec2  pos = inPosition.xy;
    float AR = uResolution.y / uResolution.x;
    //pos.x *= AR;

    volume = inPosition.w;

    gl_Position = proj * view * model * vec4(pos, inPosition.z, 1.0);
    //gl_PointSize = volume * 5.0 + 5.0;
    gl_PointSize = 5.0;
    v_bc = v_bary;
}