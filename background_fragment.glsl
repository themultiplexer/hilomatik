#version 330 core
#define M_PI 3.1415926535897932384626433832795
#define P0  0.01
#define P1  -0.1
#define EDGE   0.001
#define SMOOTH 0.001


in float volume;
in vec2 vuv;
uniform vec2 iResolution;
uniform float time;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;


#define STRIPES 30.0
#define STEP_SIZE 0.001

vec2 rotateUV(vec2 uv, float rotation)
{
    float mid = 0.0;
    return vec2(
        cos(rotation) * (uv.x - mid) + sin(rotation) * (uv.y - mid) + mid,
        cos(rotation) * (uv.y - mid) - sin(rotation) * (uv.x - mid) + mid
    );
}

float circle(vec2 uv,float rad,float blur,vec2 pos){
	return smoothstep(rad,rad*(blur-0.01),length(uv-pos));
}

float Spiral(in vec2 uv, float angle2)
{
    // Calculate and adjust angle of this vector
    float angle = atan(uv.y, uv.x);

    angle2 = sin(length(uv) * 3.0f) * angle2;
    
    angle += (0.5 - STEP_SIZE * floor(length(uv) / STEP_SIZE)) * angle2;
    //angle += (STEP_SIZE * floor(length(uv) / STEP_SIZE));

    // Return the color of the stripe at the adjusted angle
    return smoothstep(0.5, 0.5, cos(angle * STRIPES));
}

void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.9);
	
	vec2 uv = vuv;
    
    uv = rotateUV(uv, time / 4.0f);
    float angle = 0.05 * sin(time / 4.0);
    //angle = sin(time)
    float value = Spiral(uv, angle);
    gl_FragColor = vec4(1.0) - vec4(vec3(value), 1.0) - circle(uv, 0.1, 1.0, vec2(0.0,0.0));
    gl_FragColor /= 5.0f;
}