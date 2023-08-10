#version 330 core

in float volume;

void main() {
    gl_FragColor = vec4(volume, 0.2, 0.2, 1.0);
}