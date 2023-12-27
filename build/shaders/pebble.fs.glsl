#version 330

in vec3 vcolor;

uniform vec3 fcolor;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(fcolor * vcolor, 1.0);
}