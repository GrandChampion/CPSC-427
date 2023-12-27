#version 330

in vec2 texcoord;
in vec2 poscoord;

uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform vec3 bcolor;
uniform float bwidth;

layout(location = 0) out  vec4 color;

void main()
{
    color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
    float r = dot(poscoord, poscoord);
    if (r > pow(0.5 - bwidth, 2) && r < 0.25) {
        color = vec4(bcolor, 1.0);
    }
}
