#version 330 core

uniform sampler2D u_Sampler;
in vec2 f_UV;

out vec4 o_Color;

void main()
{
	float color = texture(u_Sampler, f_UV).r;
	o_Color = vec4(vec3(color), 1.0);
}