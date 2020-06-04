#version 330 core

uniform sampler2D gPosition;
//uniform sampler2D gDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;

uniform bool use_ssao;

// Lights
uniform int lightType;
uniform vec3 lightPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;

in vec2 vTexCoords;

layout(location=0) out vec4 outColor;

void main(void)
{
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, vTexCoords).rgb;
    vec3 Normal = texture(gNormal, vTexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, vTexCoords).rgb;
    float AmbientOcclusion = 0.0f;
    if(use_ssao == true){
        AmbientOcclusion = texture(ssao, vTexCoords).r;
    }

    // blinn-phong (in view-space)
    vec3 ambient;
    if(use_ssao == true){
        ambient = vec3(0.3 * Diffuse * AmbientOcclusion); // here we add occlusion factor
    }
    else{
        ambient = vec3(0.3 * Diffuse); // here we add occlusion factor
    }
    vec3 lighting  = ambient;
    vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0) in view-space
    // diffuse
    vec3 lightDir = normalize(lightPosition - FragPos);
    //if(lightType == 1){
    //    lightDir = lightDirection;
    //}
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lightColor;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = lightColor * spec;
    // attenuation
    float dist = length(lightPosition - FragPos);
    //float attenuation = lightIntensity / (lightIntensity + light.Linear * dist + light.Quadratic * dist * dist);
    //diffuse  *= attenuation;
    //specular *= attenuation;
    lighting += diffuse + specular;

    outColor = vec4(lighting, 1.0);
}
