#version 330 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Lights
#define MAX_LIGHTS 8
uniform int lightType/*[MAX_LIGHTS]*/;
uniform vec3 lightPosition/*[MAX_LIGHTS]*/;
uniform vec3 lightDirection/*[MAX_LIGHTS]*/;
uniform vec3 lightColor/*[MAX_LIGHTS]*/;
//uniform int lightCount;

in vec2 vTexCoords;

layout(location=0) out vec4 outColor;

void main(void)
{
   //Calculate lighting
   vec3 lighting = texture(gAlbedoSpec, vTexCoords).rgb * 0.1;
  // vec3 viewDir = normalize(viewPos - vPosition.xyz);

   //for(int i = 0; i < lightCount; ++i)
   //{
     // Diffuse
     vec3 lightDir = normalize(lightPosition/*[i]*/ - texture(gPosition, vTexCoords).xyz);
     vec3 diffuse = max(dot(texture(gNormal, vTexCoords).xyz, lightDir), 0.0) * texture(gAlbedoSpec, vTexCoords).rgb * lightColor/*[i]*/;
     lighting += diffuse;
   //}


    outColor = vec4(lighting, 1.0);
}
