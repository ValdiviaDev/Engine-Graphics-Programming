#version 330

uniform int code;

out vec4 outColor;

void main()
{
    outColor = vec4(code/255.0, 0, 0, 0);
}
