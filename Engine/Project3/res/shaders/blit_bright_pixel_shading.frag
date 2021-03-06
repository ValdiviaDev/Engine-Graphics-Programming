#version 330 core

uniform sampler2D colorTexture;

in vec2 vTexCoords;

out vec4 outColor;

void main(void)
{
    vec4 color = texture(colorTexture, vTexCoords);
    float intensity = dot(color.rgb, vec3(0.21, 0.71, 0.08));
    float threshold = 0.5;
    float threshold1 = threshold;
    float threshold2 = threshold + 0.1;
    outColor = color * smoothstep(threshold1, threshold2, intensity);
}
