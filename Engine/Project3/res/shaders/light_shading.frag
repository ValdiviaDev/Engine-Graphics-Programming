#version 330 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Lights
uniform int lightType;
uniform vec3 lightPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor;

in vec2 vTexCoords;

layout(location=0) out vec4 outColor;

void main(void)
{
   //Calculate lighting
   vec3 lighting = texture(gAlbedoSpec, vTexCoords).rgb * 0.1;
   vec3 lightDir = normalize(lightPosition - texture(gPosition, vTexCoords).xyz);
   vec3 diffuse = max(dot(texture(gNormal, vTexCoords).xyz, lightDir), 0.0) * texture(gAlbedoSpec, vTexCoords).rgb * lightColor;
   lighting += diffuse;

    outColor = vec4(lighting, 1.0);
}
