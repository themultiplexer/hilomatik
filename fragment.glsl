precision highp float;

varying vec2  pointPos;
uniform float aRadius;
uniform vec4  aColor;
in vec2 uv;
const float threshold = 0.1;


void main()
{
    if(false) {
        gl_FragColor = vec4(uv, 0.0, 1.0);
    } else {
        float dist = distance(pointPos, gl_FragCoord.xy);
        gl_FragColor = vec4(gl_FragCoord.y, gl_FragCoord.x, 0.0, 1.0);
        /*
        if (dist > aRadius)
            discard;

        float d = dist / aRadius;
        vec3 color = mix(aColor.rgb, vec3(0.0), step(1.0-threshold, d));

        gl_FragColor = vec4(color, 1.0);
        */
    }
}