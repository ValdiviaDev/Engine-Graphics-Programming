#version 330 core

layout(location=0) in vec3 position;
//layout(location=1) in vec3 normal;
//layout(location=2) in vec2 texCoords;
//layout(location=3) in vec3 tangent;
//(location=4) in vec3 bitangent;

//uniform mat4 projectionMatrix;
//uniform mat4 worldViewMatrix;

out vec2 vTexCoords;

void main(void)
{
    vTexCoords = (position.xy * 0.5)+vec2(0.5);
    gl_Position = /*projectionMatrix * worldViewMatrix **/ vec4(position, 1);
}
