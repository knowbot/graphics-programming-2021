#version 330 core

out vec4 fragColor;

in float age;

const float ageMax = 10.0;
const vec3 rgbStart = vec3(1.0, 1.0, 0.05);
const vec3 rgbHalf = vec3(1.0, 0.5, 0.01);
const vec3 rgbEnd = vec3(0.0, 0.0, 0.0);

void main()
{
    float alpha;
    float dist = distance(gl_PointCoord, vec2(0.5, 0.5));
    alpha = (1 - 2*dist);
    if(dist > 0.5)
        alpha = 0;
    vec3 rgb = mix(rgbHalf, rgbEnd, age / ageMax);
    if(age <= 5.0)
        rgb = mix(rgbStart, rgbHalf, age / (ageMax * 0.5));
    alpha = mix(alpha, 0, age / ageMax);
    fragColor = vec4(rgb, alpha);

}