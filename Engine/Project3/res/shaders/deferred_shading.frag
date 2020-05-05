#version 330 core

// Matrices
uniform mat4 worldViewMatrix;
uniform mat3 normalMatrix;
uniform vec3 viewPos;

// Material
uniform vec4 albedo;
uniform vec4 specular;
uniform vec4 emissive;
uniform float smoothness;
uniform float bumpiness;
uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D emissiveTexture;
uniform sampler2D normalTexture;
uniform sampler2D bumpTexture;

// Lights
#define MAX_LIGHTS 8
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform int lightCount;

in vec4 vPosition;
in vec4 vNormal;
in vec2 vTexCoords;
in vec4 albedoSpecular;

out vec4 outColor;

void main(void){

    //Data from the G buffer
    //TODO: Do we need to do this?

    //Calculate lighting
    vec3 lighting = albedoSpecular.rgb * 0.1;
   // vec3 viewDir = normalize(viewPos - vPosition.xyz);

    for(int i = 0; i < lightCount; ++i)
    {
      // Diffuse
      vec3 lightDir = normalize(lightPosition[i] - vPosition.xyz);
      vec3 diffuse = max(dot(vNormal.xyz, lightDir), 0.0) * albedoSpecular.rgb * lightColor[i];
      lighting += diffuse;
    }


    outColor = vec4(lighting, 1.0);
}
