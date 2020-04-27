#version 330 core

uniform sampler2D u_Sampler;
uniform sampler2D u_ColorSampler;
uniform ivec2 u_ColorMapSize;

in vec2 f_UV;
flat in ivec2 f_ColorIndex;

out vec4 o_Color;

vec3 get_color(int color_index)
{
	int color_x = color_index % u_ColorMapSize.x;
	int color_y = color_index / u_ColorMapSize.y;

	vec2 color_uv = vec2(color_x + 0.5, color_y + 0.5) / u_ColorMapSize;
	return texture(u_ColorSampler, color_uv).xyz;
}

void main()
{
	vec3 fg_color = get_color(f_ColorIndex[0]);
	vec3 bg_color = get_color(f_ColorIndex[1]);

	float font = texture(u_Sampler, f_UV).r;

	vec3 color = mix(bg_color, fg_color, font);
	o_Color = vec4(color, 1.0);
}