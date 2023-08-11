#version 330 core

in float volume;
uniform sampler2D depthTexture;
uniform vec2 uResolution;
in vec3 v_bc;

float line_segment(in vec2 p, in vec2 a, in vec2 b) {
	vec2 ba = b - a;
	vec2 pa = p - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
	return length(pa - h * ba);
}

vec4 mainImage(vec2 fragCoord) {
	vec2 pos = (fragCoord - uResolution.xy * .5) / uResolution.y;
	float zoom = 2.5;
	pos *= zoom;

	vec2 v1 = vec2(0.,5.);
	vec2 v2 = vec2(0.,5.) + 3.1;
	float thickness = 10.0;

	float d = line_segment(pos, v1, v2) - thickness;

	vec3 color = vec3(1.) - sign(d) * vec3(0., 0., 0.);
	color *= 1.5 - exp(.5 * abs(d));
	color *= .5 + .3 * cos(120. * d);
	color = mix(color, vec3(1.), 1. - smoothstep(.0, .015, abs(d)));

	return vec4(color, 1.);
}

void main() {
    //gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	float lineWidth = 0.005;
	float f_closest_edge = min(v_bc.x, min(v_bc.y, v_bc.z)); // see to which edge this pixel is the closest
	float f_width = fwidth(f_closest_edge); // calculate derivative (divide lineWidth by this to have the line width constant in screen-space)
	float f_alpha = smoothstep(lineWidth, lineWidth + f_width, f_closest_edge); // calculate alpha
	gl_FragColor = vec4(vec3(1.0 - f_alpha), 1.0);
	//gl_FragColor = vec4(color.rgb, 1.0 - f_alpha);
	//gl_FragColor = vec4(vec3(0.9), 1.0) * (1.0 - f_alpha) * height  * color.a + color;
}