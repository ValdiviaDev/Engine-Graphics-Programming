#version 330 core

layout(location=0) in vec3 position;

out vec2 vTexCoords;

void main(void)
{
    vTexCoords = (position.xy * 0.5)+vec2(0.5);
    gl_Position = /*projectionMatrix * worldViewMatrix **/ vec4(position, 1);
}
