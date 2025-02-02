#version 330 core
// FRAGMENT SHADER

// TODO voronoi 1.4
out vec4 fragColor;
// Create an 'in float' variable to receive the depth value from the vertex shader,
// the variable must have the same name as the 'out variable' in the vertex shader.
in float z_depth;

void main()
{
    // Use the interpolated z-coordinate to draw the distance of this fragment
    // relative to the center of the cone.
    // Make sure that the z-coordinate is sin the [0, 1] range (if it is not, place it in that range).
    // You can use non-linear transformations of the z-coordinate, such as the 'pow' or 'sqrt' functions,
    // to make the change in grey tone more evident.
    float scaled_depth = (z_depth + 1.0) / 2.0;
    float final_depth = 1 - pow(scaled_depth, 2);
    fragColor = vec4(vec3(final_depth), 1.0); // CODE HERE
}