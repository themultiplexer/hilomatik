#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

in vec3 inPosition;

uniform vec2 iResolution;
out float volume;

void main() {
    vec2  pos = inPosition.xy;
    float AR = iResolution.y / iResolution.x;
    //pos.x *= AR;


    gl_Position = proj * view * model * vec4(pos, inPosition.z, 1.0);
    //gl_PointSize = volume * 5.0 + 5.0;
    gl_PointSize = 5.0;
}