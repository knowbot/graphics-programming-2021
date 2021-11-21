#version 330 core
layout (location = 0) in vec3 pos;
const int boxSize = 50;
uniform mat4 viewMat;
uniform vec3 cameraPos;
uniform vec3 forwardOffset;
uniform vec3 offsets;
const float minSize = 0.f, maxSize = 10.0f;

void main()
{
    vec3 position = mod(pos + offsets, boxSize) + cameraPos + forwardOffset - boxSize/2;
    gl_Position = viewMat * vec4(position, 1.0);
//    float dist = distance(position, cameraPos);
    gl_PointSize = max(maxSize / gl_Position.w, minSize);
}