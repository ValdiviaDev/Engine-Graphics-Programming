#version 330 core

uniform sampler2D ssaoInput;

in vec2 vTexCoords;
out float outColor;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, vTexCoords + offset).r;
        }
    }
    outColor = result / (4.0 * 4.0);
}
