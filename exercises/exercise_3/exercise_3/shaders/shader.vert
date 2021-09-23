#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
out vec4 vtxColor;

uniform mat4 model;

// TODO 3.1 create a mat4 uniform named 'model', you should set it for each part of the plane

void main()
{
   vec4 vPos = model * vec4(pos, 1.0);
   gl_Position = vPos;
   vtxColor = color;
}