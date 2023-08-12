#version 330 core
#define M_PI 3.1415926535897932384626433832795
#define P0  0.1
#define P1  -0.1


#define EDGE   0.005
#define SMOOTH 0.0025


in float volume;
uniform vec2 iResolution;
uniform float time;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;


float N_i_1 (in float t, in float i)
{
    // return 1 if i < t < i+1, else return 0
    return step(i, t) * step(t,i+1.0);
}

float N_i_2 (in float t, in float i)
{
    return
        N_i_1(t, i)       * (t - i) +
        N_i_1(t, i + 1.0) * (i + 2.0 - t);
}

float N_i_3 (in float t, in float i)
{
    return
        N_i_2(t, i)       * (t - i) / 2.0 +
        N_i_2(t, i + 1.0) * (i + 3.0 - t) / 2.0;
}

float SplineValue(in float t)
{
    return
        P0 * N_i_3(t, 0.0) +
        P1 * N_i_3(t, 1.0);  
}

// F(x,y) = F(x) - y
float F ( in vec2 coords )
{
    // time in this curve goes from 0.0 to 10.0 but values
    // are only valid between 2.0 and 8.0
    float T = coords.x*6.0 + 2.0;
    return SplineValue(T) - coords.y;
}

// signed distance function for F(x,y)
float SDF( in vec2 coords )
{
    float v = F(coords);
    float slope = dFdx(v) / dFdx(coords.x);
    return abs(v)/length(vec2(slope, -1.0));
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

float udSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 ba = b-a;
    vec2 pa = p-a;
    float h =clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length(pa-h*ba);
}

void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.9);
	//center origin point
	
	vec4 fragCoord = view * model * gl_FragCoord;

	vec2 uv = ( fragCoord.xy -.5* iResolution.xy) / iResolution.y;
	
	vec2 pupil_location = vec2(0.0,0.04 * sin(time));
	float col0  = circle(uv, 0.35, 1.0, vec2(0.0,0.0));

    float col1  = circle(uv, 0.245,1.0, vec2(0.0,0.0)) ;
    float col2 = circle(uv, 0.235,1.0, vec2(0.0,0.0)) ;
    float col3 = circle(uv, 0.12,1.0, vec2(0.0,0.0)) ;
    float col4 = circle(uv, 0.10,1.0,vec2(0.0,0.0)) ;
    float col5 = ellipse(uv, vec2(0.06,0.12),1.0, vec2(0.0,0.0)) ;
    float col6 = ellipse(uv, vec2(0.04,0.1),1.0,vec2(0.0,0.0)) ;
	float col7 = circle(uv, 0.035,1.0, pupil_location) ;
    float col8 = circle(uv, 0.015,0.98,pupil_location) ;

    gl_FragColor = (ca(1.0 - col0) - c(1.0 - col0)) + (c(col1) - c(col2)) + (c(col3) - c(col4)) + (c(col5) - c(col6)) + (c(col7) - c(col8));

	int elements = 12;
	for(int i = 0; i < elements; i++) {
		float theta = 2.0f * M_PI * i / elements;

		vec2 v1 = vec2(0.125 * cos(theta), 0.125 * sin(theta));
		vec2 v2 = vec2(0.23 * cos(theta), 0.23 * sin(theta));
		float d = udSegment( uv, v1, v2 ) - 0.001;
		vec4 col = vec4(1.0) - sign(d)*vec4(1.0,1.0,1.0, 1.0);
		gl_FragColor += col;
	}
}