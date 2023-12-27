#version 330

in vec3 in_position;
in vec2 in_texcoord;

out vec2 texcoord;
out vec2 poscoord;

uniform mat3 transform;
uniform mat3 projection;
uniform float uv_x;
uniform float uv_y;
uniform float nx_frames;
uniform float ny_frames;

void main()
{
	texcoord = (vec2(uv_x,uv_y) + in_texcoord) / vec2(nx_frames,ny_frames);
	poscoord = in_position.xy;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}