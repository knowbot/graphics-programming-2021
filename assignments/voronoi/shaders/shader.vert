#version 330 core
// VERTEX SHADER

// TODO voronoi 1.3
// Receives position in 'vec3', the position variable has attribute 'location = 0'
layout(location = 0) in vec3 pos;
// You have to declare an 'out float' to send the z-coordinate of the position
// to the fragment shader (voronoi 1.4 and 1.5)
out float z_depth;
// You have to set an 'uniform vec2' to receive the position offset of the object
uniform vec2 offset;

void main()
{
    // TODO voronoi 1.3
    // Set the vertex->fragment shader 'out' variable
    z_depth = pos.z;
    // Set the 'gl_Position' built-in variable using a 'vec4(vec3 position you compute, 1.0)',
    vec3 new_pos = pos + vec3(offset, 0.0);
    gl_Position = vec4(new_pos, 1.0);
    // Remeber to use the 'uniform vec2' to move the vertex before you set 'gl_Position'.
}