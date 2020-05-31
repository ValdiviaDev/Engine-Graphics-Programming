#version 330 core

// Matrices
uniform mat4 worldViewMatrix;
uniform mat3 normalMatrix;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Lights
#define MAX_LIGHTS 8
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform int lightCount;

in vec2 vTexCoords;

out vec4 outColor;

void main(void)
{
    //Calculate lighting
    vec3 lighting = gAlbedoSpec.rgb * 0.1;
   // vec3 viewDir = normalize(viewPos - vPosition.xyz);

    for(int i = 0; i < lightCount; ++i)
    {
      // Diffuse
      vec3 lightDir = normalize(lightPosition[i] - gPosition.xyz);
      vec3 diffuse = max(dot(gNormal.xyz, lightDir), 0.0) * gAlbedoSpec.rgb * lightColor[i];
      lighting += diffuse;
    }


    outColor = vec4(lighting, 1.0);
}
