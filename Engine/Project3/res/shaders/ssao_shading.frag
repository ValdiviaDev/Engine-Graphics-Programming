#version 330 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;

in vec2 vTexCoords;
out float outColor;

// tile noise texture over screen, based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0); // screen = 800x600

void main()
{
    vec3 fragPos   = texture(gPosition, vTexCoords).xyz;
    vec3 normal    = texture(gNormal, vTexCoords).rgb;
    vec3 randomVec = texture(texNoise, vTexCoords * noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i)
    {
        // get sample position
        vec3 sample = TBN * samples[i]; // from tangent to view-space
        sample = fragPos + sample * 0.5;
        vec4 offset = vec4(sample, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        float sampleDepth = texture(gPosition, offset.xy).z;
        occlusion += (sampleDepth >= sample.z + 0.025 ? 1.0 : 0.0);
        float rangeCheck = smoothstep(0.0, 1.0, 0.5 / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= sample.z + 0.025 ? 1.0 : 0.0 ) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / 64);
    outColor = occlusion;

}
