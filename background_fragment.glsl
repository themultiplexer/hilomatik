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
    float angle = atan(uv.y, uv.x);
    angle2 = sin(length(uv) * 1.0f) * angle2;
    angle += (0.5 - STEP_SIZE * floor(length(uv) / STEP_SIZE)) * angle2;
    return smoothstep(0.5, 0.5, cos(angle * STRIPES));
}

vec4 test2(vec2 uv) {
    float every = 0.45;
    float xx = length(uv);
    float yy = (mod(atan(uv.y,uv.x), every) - (every/2.0));
    float radius = 0.075;
    
    vec2 center = vec2(0.4, 0.0);
    vec2 ocenter = vec2(0.4 + 2.0 * radius, 0.0);
    float kArc = 0.5;
    float angle = yy;
    
    xx = xx * cos(yy);
    yy = xx * sin(yy);
    
    float dist = sqrt(pow(xx - center.x, 2.0) + pow(yy - center.y, 2.0)) - radius;
    float dist2 = sqrt(pow(xx - center.x, 2.0) + pow(yy - center.y, 2.0)) - 0.05;
    float dist3 = sqrt(pow(xx - ocenter.x, 2.0) + pow(yy - ocenter.y, 2.0)) - radius;
    float dist4 = length(uv) - 0.4 + radius;
    
    float segment = step( angle, kArc ) * step( 0.0, angle );
    float segment2 = step( angle + 0.5, kArc ) * step( 0.0, angle + 0.5 );
	
    float lineWidth = 2.0;
    float res = 2.0 * smoothstep(1.0, 0.0, abs(dist) / fwidth(dist) / lineWidth) * mix( segment, 1.0, step( 1.0, kArc ) );
    //res += 2.0 * smoothstep(1.0, 0.0, abs(dist2) / fwidth(dist2) / lineWidth);
    res += 2.0 * smoothstep(1.0, 0.0, abs(dist3) / fwidth(dist3) / lineWidth) * mix( segment2, 1.0, step( 1.0, kArc ) );
    res += 2.0 * 2.0 * smoothstep(1.0, 0.0, abs(dist4) / fwidth(dist4) / lineWidth);
    return vec4(vec3(res), 1.0);
}

void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.9);
	
	vec2 uv = 4.0 * vuv - 2.0;
    
    uv = rotateUV(uv, time / 4.0f);
    float angle = 0.5 * sin(time);
    if(true) {
        float value = Spiral(uv, angle);
        gl_FragColor = vec4(1.0) - vec4(vec3(value), 1.0) - circle(uv, 0.1, 1.0, vec2(0.0,0.0));
        gl_FragColor /= 5.0f;
    } else {
        gl_FragColor = test2(uv);
    }
}