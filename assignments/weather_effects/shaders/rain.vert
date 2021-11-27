#version 330 core
layout (location = 0) in vec3 pos;
const int boxSize = 50;

uniform mat4 viewProj;
uniform mat4 viewProjPrev;
uniform vec3 cameraPos;
uniform vec3 forwardOffset;
uniform vec3 offsets;
uniform float maxSize;
uniform float velocity;
uniform float heightScale;
const float minSize = 0.0f;

void main()
{
    vec3 newPos = mod(pos + offsets, boxSize) + cameraPos + forwardOffset - boxSize/2;
//    vec3 posPrev = pos + velocity * heightScale;
//    vec4 bot = viewProj * vec4(pos, 1.0);
//    vec4 top = viewProjPrev * vec4(posPrev, 1.0);
//    gl_Position = mix(top, bot, mod(gl_VertexID, 2));
    gl_Position = viewProj * vec4(newPos, 1.0);
    gl_PointSize = max(maxSize / gl_Position.w, minSize);
}