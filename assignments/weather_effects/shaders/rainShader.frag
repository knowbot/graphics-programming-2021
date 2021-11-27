#version 330 core
in float lenColorScale;

out vec4 fragColor;

void main()
{
    vec3 color = vec3(0.7,0.7,0.8);

    fragColor = vec4(color,lenColorScale);

}