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

vec4 test2(vec2 uv) {
    float every = 0.45;
    float xx = length(uv);
    float yy = (mod(atan(uv.y,uv.x), every) - (every/2.0));
    float radius = 0.1;
    vec2 ocenter = vec2(0.8, 0.0);
    vec2 center = vec2(0.6, 0.0);
    float kArc = 0.5;
    float angle = yy;
    
    xx = xx * cos(yy);
    yy = xx * sin(yy);
    
    float dist = sqrt(pow(xx - center.x, 2.0) + pow(yy - center.y, 2.0)) - radius;
    float dist2 = sqrt(pow(xx - center.x, 2.0) + pow(yy - center.y, 2.0)) - 0.05;
    float dist3 = sqrt(pow(xx - ocenter.x, 2.0) + pow(yy - ocenter.y, 2.0)) - radius;
    float dist4 = length(uv) - 0.5;
    
    float segment = step( angle, kArc ) * step( 0.0, angle );
    float segment2 = step( angle + 0.5, kArc ) * step( 0.0, angle + 0.5 );
	
    float lineWidth = 2.0;
    float res = smoothstep(1.0, 0.0, abs(dist) / fwidth(dist) / lineWidth) * mix( segment, 1.0, step( 1.0, kArc ) );
    res += smoothstep(1.0, 0.0, abs(dist2) / fwidth(dist2) / lineWidth);
    res += smoothstep(1.0, 0.0, abs(dist3) / fwidth(dist3) / lineWidth) * mix( segment2, 1.0, step( 1.0, kArc ) );
    res += 2.0 * smoothstep(1.0, 0.0, abs(dist4) / fwidth(dist4) / lineWidth);
    return vec4(vec3(res), res);
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

    uv = rotateUV(uv, -0.3 * (time / 1.0));
	
	vec2 pupil_location = vec2(0.0,0.1 * sin(time));
	float col0 = circle(uv, 1.5 + volume, 1.0, vec2(0.0,0.0));

    float col1 = circle(uv, 1.01 + volume,1.0, vec2(0.0,0.0)) ;
    float col2 = circle(uv, 0.975+ volume,0.99, vec2(0.0,0.0)) ;
    float col3 = circle(uv, 0.45,1.0, vec2(0.0,0.0)) ;
    float col4 = circle(uv, 0.39,1.0,vec2(0.0,0.0)) ;
    float col5 = ellipse(uv, vec2(0.225,0.45),0.99, vec2(0.0,0.0)) ;
    float col6 = ellipse(uv, vec2(0.15,0.39),0.98,vec2(0.0,0.0)) ;
	float col7 = circle(uv, 0.12,1.0, pupil_location) ;
    float col8 = circle(uv, 0.05,1.0,pupil_location) ;

    gl_FragColor = (ca(1.0 - col0) - c(1.0 - col0))   +  (c(col1) - c(col2)) + (c(col3) - c(col4)) + (c(col5) - c(col6)) + (c(col7) - c(col8));
    
    if(sin(time * 0.2) > 0.0){
        gl_FragColor += sines(uv, 0.0) + sines(uv, 0.05);
    } else {
        gl_FragColor += test2(uv);
    }
}