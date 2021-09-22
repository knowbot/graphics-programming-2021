#version 330 core

out vec4 fragColor;

in float age;

void main()
{
    float alpha;
    float dist = distance(gl_PointCoord, vec2(0.5, 0.5));
    if(dist > 0.5)
        alpha = 0;
    else {
        alpha = (1 - 2*dist);
    }
    vec3 rgb;
    if(age <= 5.0)
        rgb = vec3(1.0, mix(1.0, 0.5, age), mix(0.05, 0.01, age));
    else
        rgb = vec3(mix(1.0, 0.0, age), mix(0.5, 0.0, age), mix(0.01, 0.0, age));
    alpha = mix(alpha, 0, age);
    fragColor = vec4(rgb, alpha);

}