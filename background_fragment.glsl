#version 330 core
#define M_PI 3.1415926535897932384626433832795
#define P0  0.01
#define P1  -0.1
#define EDGE   0.001
#define SMOOTH 0.001


uniform float vol;
in vec2 vuv;
uniform vec2 iResolution;
uniform float time;
uniform int mode;

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

float ellipse(vec2 uv, vec2 size, float blur,vec2 pos){
	return smoothstep(1.0,blur-0.01,length((uv/size)-pos));
}

vec4 square(vec2 uv, float size, vec2 pos, vec3 color) {
    float x = uv.x - pos.x;
    float y = uv.y - pos.y;
    float d = max(abs(x), abs(y)) - size;
    return d > 0. ? vec4(0.) : vec4(color, 1.0);
}

vec4 c(float col){
	return vec4(vec3(col), col);
}

vec4 ca(float col){
	return vec4(vec3(col), 1.0);
}

float Spiral(in vec2 uv, float angle2)
{
    float angle = atan(uv.y, uv.x);
    angle2 = sin(length(uv) * 1.0f) * angle2;
    angle += (0.5 - STEP_SIZE * floor(length(uv) / STEP_SIZE)) * angle2;
    return smoothstep(0.5, 0.5, cos(angle * STRIPES));
}

vec4 test2(vec2 uv, vec3 color, float radius, int ncirc, float xbegin, float every, float angle_offset, bool inner, float lw, bool adaptive_line_width) {
    float xx = length(uv);
    float yy = mod(atan(uv.y,uv.x) + angle_offset, every) - (every/2.0);
    
    
    float kArc = 0.5;
    float angle = yy;


    xx = xx * cos(yy);
    yy = xx * sin(yy);

    vec2 center = vec2(xbegin, 0.0);
    
    float ring_dist = sqrt(pow(xx - center.x, 2.0) + pow(yy - center.y, 2.0)) - (radius / 2.0);
    float center_ring = length(uv) - center.x + radius;
    
    float lineWidth = lw;
    if (adaptive_line_width) {
        lineWidth = lineWidth - clamp(lineWidth * (length(uv) - xbegin) / (ncirc * radius * 2.0), 0.0, lineWidth);
    }
    float res = 0.0;

    float a = 1.0;
    float b = 0.0;

    for (int i = 0; i < ncirc; i++) {
        vec2 point = center + vec2( i * 2.0 * radius, 0.0);
        float dist = sqrt(pow(xx - point.x, 2.0) + pow(yy - point.y, 2.0)) - radius;
        float segment = step( angle + (i % 2) * 0.5, kArc ) * step( 0.0, angle + (i % 2) * 0.5 );
        res += smoothstep(a, b, abs(dist) / fwidth(dist) / lineWidth) * mix( segment, 1.0, step( 1.0, kArc ) );
    }

    
    if(inner){
        res += smoothstep(a, b, abs(ring_dist) / fwidth(ring_dist) / lineWidth);
        res += smoothstep(a, b, abs(center_ring) / fwidth(center_ring) / lineWidth);
    } else if (angle_offset == 0.0) {
        res += smoothstep(a,b, abs(center_ring) / fwidth(center_ring) / lineWidth);
    }


    return mix(vec4(0), vec4(color, res),  res);
}

void main() {
    gl_FragColor = vec4(0.1, 0.1, 0.1, 1.0);
    //gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	
	vec2 uv = 4.0 * vuv - 2.0;
    
    
    if(mode == 0) {
        uv = rotateUV(uv, time / 4.0f);
        float angle = 0.5 * sin(time);
        float value = Spiral(uv, angle);
        gl_FragColor = vec4(1.0) - vec4(vec3(value), 1.0) - circle(uv, 0.1, 1.0, vec2(0.0,0.0));
        gl_FragColor /= 5.0f;
    } else if(mode == 1) {
        uv = rotateUV(uv, time);
        gl_FragColor = vec4(vec3(0.5), 1.0);
        gl_FragColor -= test2(uv, vec3(1.0), 0.075, 3, 0.375, 0.7, 0.0, false, 4.0, true);
        gl_FragColor -= test2(uv, vec3(1.0), 0.075, 3, 0.375, 0.7, 0.35, false, 4.0, true);
    } else {
        vec2 uv1 = rotateUV(uv, time / 1.5);
        gl_FragColor += test2(uv1,vec3(1.0), 0.075, 3, 0.375, 0.575, 0.0, false, 4.0,true);
        gl_FragColor += test2(uv1,vec3(1.0, 0.0, 0.0), 0.075, 3, 0.375, 0.575, 0.2875, false, 4.0,true);

        vec2 uv2 = rotateUV(uv, -time / 2.0);
        gl_FragColor -= test2(uv2,vec3(1.0), 0.04, 2, 0.25 + vol, 0.7, 0.0, true, 6.0, false);
        gl_FragColor += test2(uv2,vec3(4.0), 0.04, 2, 0.25 + vol, 0.7, 0.0, true, 2.0, true);
        gl_FragColor -= test2(uv2,vec3(1.0), 0.04, 2, 0.25 + vol, 0.7, 0.35, true, 6.0, false);
        gl_FragColor += test2(uv2,vec3(4.0), 0.04, 2, 0.25 + vol, 0.7, 0.35, true, 2.0, true);

        gl_FragColor = clamp(gl_FragColor, 0.0, 1.0);

        vec2 pupil_location = vec2(0.0,0.1 * sin(time));
        vec2 uva = uv * 6.0;

        vec2 uvb = rotateUV(uva, time / 1.5);
        gl_FragColor += square(uvb, 1.2, vec2(0.0, 0.0), vec3(1));
        gl_FragColor -= square(uvb, 1.19, vec2(0.0, 0.0), vec3(2));
        gl_FragColor += square(uvb, 1.0, vec2(0.0, 0.0), vec3(2));

        float col0 = circle(uva, 0.5, 1.0, vec2(0.0,0.0));
        float col2 = circle(uva, 1.05,0.99, vec2(0.0,0.0)) ;
        float col3 = circle(uva, 0.45,1.0, vec2(0.0,0.0)) ;
        float col4 = circle(uva, 0.39,1.0,vec2(0.0,0.0)) ;
        float col5 = ellipse(uva, vec2(0.225,0.45),0.99, vec2(0.0,0.0)) ;
        float col6 = ellipse(uva, vec2(0.15,0.39),0.98,vec2(0.0,0.0)) ;
        float col7 = circle(uva, 0.12,1.0, pupil_location);
        float col8 = circle(uva, 0.05,1.0, pupil_location);

        gl_FragColor -= c(col2);
        gl_FragColor += (c(col3) - c(col4)) + (c(col5) - c(col6)) + (c(col7) - c(col8));
        gl_FragColor = clamp(gl_FragColor * 2.0, 0.0, 1.0);



        
    }
}