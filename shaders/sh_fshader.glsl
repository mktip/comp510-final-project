#version 330 core
in vec4 FragPos;

uniform vec4 LightPosition;
uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - LightPosition.xyz);

    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / far_plane;

    // write this as modified depth
    gl_FragDepth = lightDistance;
}