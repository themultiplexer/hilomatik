#version 330 core
#define M_PI 3.1415926535897932384626433832795

in float volume;
in vec2 vuv;
uniform vec2 iResolution;
uniform float time;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

vec4 sines(vec2 uv, float offset){
    float xx = sqrt(uv.x * uv.x + uv.y * uv.y);
    float yy = (mod(atan(uv.y,uv.x) + offset, 0.19625) - 0.1);
    xx = xx * cos(yy);
    yy = xx * sin(yy);
    
    float x1 = 0.5;
    float x2 = 1.0;
    float t = -sin(xx * (6.3f/(x2-x1))) * 0.04;
    float thickness = 0.015 * xx;
    float dist = t - yy;
    float fw = fwidth(dist);
    float lineWidth = 5.0;
    

    if (xx > x1 && xx < x2 && yy >= t - thickness && yy <= t + thickness) {
        return vec4(smoothstep(1.0,0.0,abs(dist) / fw / lineWidth));
    	//return vec4(vec3(1.0),1.0);
    }
    return vec4(0.0);
}

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

vec4 c(float col){
	return vec4(vec3(col), col);
}

vec4 ca(float col){
	return vec4(vec3(col), 1.0);
}

void main() {
    vec2 uv = vuv;

    uv = rotateUV(uv, -0.3 * (time / 2.0));
	
	vec2 pupil_location = vec2(0.0,0.1 * sin(time));
	float col0 = circle(uv, 1.5 + volume, 1.0, vec2(0.0,0.0));

    float col1 = circle(uv, 1.01 + volume,1.0, vec2(0.0,0.0)) ;
    float col2 = circle(uv, 0.975+ volume,0.99, vec2(0.0,0.0)) ;
    float col3 = circle(uv, 0.45,1.0, vec2(0.0,0.0)) ;
    float col4 = circle(uv, 0.39,1.0,vec2(0.0,0.0)) ;
    float col5 = ellipse(uv, vec2(0.225,0.45),1.0, vec2(0.0,0.0)) ;
    float col6 = ellipse(uv, vec2(0.15,0.39),1.0,vec2(0.0,0.0)) ;
	float col7 = circle(uv, 0.12,1.0, pupil_location) ;
    float col8 = circle(uv, 0.05,1.0,pupil_location) ;

    gl_FragColor = (ca(1.0 - col0) - c(1.0 - col0))   +  (c(col1) - c(col2)) + (c(col3) - c(col4)) + (c(col5) - c(col6)) + (c(col7) - c(col8));
    gl_FragColor += sines(uv, 0.0) + sines(uv, 0.05);
}