#version 330 core
layout (location = 0) in vec3 pos;   // the position variable has attribute position 0
uniform mat4 viewProjection; // current viewProjection matrix
uniform mat4 prevViewProjection; // previous viewProjection matrix
uniform float boxSize;
uniform vec3 offsets;
uniform vec3 camPosition;
uniform vec3 camForward;
uniform vec3 instanceVelocity;
out float lenColorScale;


void main()
{
    vec3 position = mod(pos + offsets, boxSize);
    position += camPosition + camForward - boxSize/2;

    // == Code additions for drawing precipitation as lines ==
    vec3 worldPos = position;
    vec3 worldPosPrev = position + instanceVelocity *  0.005;

    vec4 bottom = viewProjection*vec4(worldPos,1.0);
    vec4 top = viewProjection*vec4(worldPosPrev,1.0);
    vec4 topPrev = prevViewProjection*vec4(worldPosPrev, 1.0);

    vec4 projPos = mix(topPrev, bottom, mod(gl_VertexID,2));

    gl_Position = projPos;

    vec2 dir = (top.xy/top.w) - (bottom.xy/bottom.w);
    vec2 dirPrev = (topPrev.xy/topPrev.w) - (bottom.xy/bottom.w);

    float len = length(dir);
    float lenPrev = length(dirPrev);
    lenColorScale = clamp(len/lenPrev, 0.0, 1.0);

    // =======================================================

    // For points
    //gl_Position = viewProjection * vec4(position, 1.0); // position for points
    //gl_PointSize = 3.0;
}