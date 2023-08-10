#version 330 core

in vec2 inPosition;
uniform vec2 uResolution; // = (window-width, window-height)

void main() {
    vec2  uv = inPosition;
    float AR = uResolution.y / uResolution.x;
    
    uv.x *= AR;

    gl_Position = vec4(uv, 0.0, 1.0);
    gl_FrontColor = vec4(1.0,1.0,1.0, 1.0);
    gl_PointSize = inPosition.y * 50.0;
}