#version 330 core

in float volume;
uniform sampler2D depthTexture;
uniform vec2 iResolution;
in vec3 v_bc;

void main() {
    float lineWidth = 0.05;
	float f_closest_edge = min(v_bc.x, min(v_bc.x, v_bc.z * (1.0 - v_bc.x))); // see to which edge this pixel is the closest
	float f_width = fwidth(f_closest_edge); // calculate derivative (divide lineWidth by this to have the line width constant in screen-space)
	float f_alpha = smoothstep(lineWidth, lineWidth + f_width, f_closest_edge); // calculate alpha
	gl_FragColor = vec4(vec3(1.0 - f_alpha), 1.0);
}