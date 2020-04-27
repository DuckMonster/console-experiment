#version 330 core

uniform ivec2 u_CellmapSize;
uniform ivec2 u_CellSize;
uniform ivec2 u_TilesetSize;

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in ivec2 a_Tile;

out vec2 f_UV;

void main()
{
	vec2 position = a_Position;
	position /= u_CellmapSize;
	position.y = 1.0 - position.y;
	position = position * 2.0 - 1.0;

	gl_Position = vec4(position, 0.0, 1.0);

	vec2 uv;
	uv.x = (float(u_CellSize.x) / u_TilesetSize.x) * (a_Tile.x + a_UV.x) + (1.0 / u_TilesetSize.x) * a_Tile.x;
	uv.y = (float(u_CellSize.y) / u_TilesetSize.y) * (a_Tile.y + a_UV.y) + (1.0 / u_TilesetSize.y) * a_Tile.y;

	f_UV = uv;
}
