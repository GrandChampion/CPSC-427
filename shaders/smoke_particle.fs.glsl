#version 330

in vec2 vpos;

uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float timing;

layout (location = 0) out vec4 color;

void main()
{
    float opacity_change = abs(sin(timing))+0.8;
    float green_value = 0.4*abs(sin(timing));
    color = vec4(1.0, green_value, 0.0, opacity_change);

}