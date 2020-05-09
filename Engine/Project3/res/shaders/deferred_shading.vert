#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoords;
layout(location=3) in vec3 tangent;
layout(location=4) in vec3 bitangent;

uniform mat4 projectionMatrix;
uniform mat4 worldViewMatrix;
uniform mat4 worldMatrix;
uniform mat4 viewMatrix;

uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;

out vec4 vPosition;
out vec4 vNormal;
out vec2 vTexCoords;

void main(void)
{
    vPosition = projectionMatrix * worldMatrix * vec4(position, 1);

    //TODO: Is this way of doing it ok? Should we normalize?
    //vNormal =  inverse(worldMatrix) * vec4(normal, 1);
    vNormal =  projectionMatrix * worldMatrix * vec4(normal, 1);
    vTexCoords = texCoords;

    gl_Position = projectionMatrix * worldViewMatrix * vec4(position, 1);

}
