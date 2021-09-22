#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 velocity;
layout (location = 2) in float timeOfBirth;

uniform float currentTime;

out float age;

void main()
{
    // this is the output position and and point size (this time we are rendering points, instad of triangles!)
    if (timeOfBirth == 0) {
        gl_Position = vec4(-2.0, -2.0, -2.0, 0.0);
        return;
    }
    age = currentTime - timeOfBirth;
    if(age > 10.0)
        age = 10.0;
    vec2 newPos = pos + velocity * age;
    gl_Position = vec4(newPos, 0.0, 1.0);
    gl_PointSize = mix(0.1, 20, age);
}