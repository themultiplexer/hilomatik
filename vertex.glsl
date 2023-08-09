precision mediump float;

varying   vec2 pointPos;
uniform   vec2 uResolution; // = (window-width, window-height)
uniform   mat4 Projection;
uniform   mat4 ModelView;

in vec4 position;
out varying vec2 uv;

void main()
{
    if(false) {
        gl_Position = position;
        uv = (position.xy + 1.0) / 2.0;
        gl_Color = vec4(1.0,1.0,1.0, 1.0);
        gl_PointSize = 100.0f;
    } else {
        gl_Position  = Projection * ModelView * position;
        gl_PointSize = 100.0;

        vec2 ndcPos = gl_Position.xy / gl_Position.w;
        pointPos    = uResolution * (ndcPos*0.5 + 0.5);
    }
}