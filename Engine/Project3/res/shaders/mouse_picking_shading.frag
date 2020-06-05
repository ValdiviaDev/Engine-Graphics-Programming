#version 330

uniform float code;

out vec4 outColor;

void main()
{
    outColor = vec4(vec3(code),1.0);

}
