#version 330 core
#define M_PI 3.1415926535897932384626433832795

in vec2 vuv;

uniform sampler2D texture;
uniform vec2 iResolution;
uniform float time;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    //gl_FragColor = vec4(1.0, 1.0, 1.0, 0.1);
    gl_FragColor = texture2D(texture, vuv);
    gl_FragColor.a /= 2.0;
}