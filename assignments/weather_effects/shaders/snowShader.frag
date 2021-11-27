#version 330 core
in float lenColorScale;

out vec4 fragColor;

void main()
{
    vec3 color = vec3(1.0,1.0,1.0);

    fragColor = vec4(color,lenColorScale);

}