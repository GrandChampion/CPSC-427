#version 330

in vec2 texcoord;

uniform sampler2D sampler0;
uniform vec3 fcolor;

layout(location = 0) out  vec4 color;

void main()
{
    color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
}
