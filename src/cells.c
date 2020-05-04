#include "cells.h"
#include "gl_bind.h"
#include "import.h"
#include <stdlib.h>

GLuint vao;
GLuint quad_vbo;
GLuint offset_vbo;
GLuint cell_vbo;
GLuint program;

GLuint font_texture;
GLuint color_texture;

typedef struct 
{
	f32 x;
	f32 y;
	f32 u;
	f32 v;
} Cell_Vert;
Cell_Vert* cell_verts = NULL;
Cell* cells = NULL;

void cells_init()
{
	// Setup vertex objects
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	/* CELL QUAD VBO */
	float quad_data[] = {
		0.f, 0.f,
		1.f, 0.f,
		0.f, 1.f,

		1.f, 0.f,
		0.f, 1.f,
		1.f, 1.f,
	};

	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);

	/* CELL OFFSETS VBO */
	{
		typedef struct
		{
			i32 x;
			i32 y;
		} Cell_Offset;

		u32 offsets_size = sizeof(Cell_Offset) * 2 * CELL_COLS * CELL_ROWS;
		Cell_Offset* offsets = (Cell_Offset*)malloc(offsets_size);
		Cell_Offset* ptr = offsets;
		for(i32 y = 0; y < CELL_ROWS; ++y)
		{
			for(i32 x = 0; x < CELL_COLS; ++x)
			{
				ptr->x = x;
				ptr->y = y;
				ptr++;
			}
		}

		glGenBuffers(1, &offset_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, offset_vbo);
		glBufferData(GL_ARRAY_BUFFER, offsets_size, offsets, GL_STATIC_DRAW);

		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(1, 2, GL_INT, 0, 0);
		glVertexAttribDivisor(1, 1);
	}

	/* CELL DATA VBO */
	{
		u32 cells_size = sizeof(Cell) * CELL_COLS * CELL_ROWS;
		cells = (Cell*)malloc(cells_size);
		mem_zero(cells, cells_size);

		glGenBuffers(1, &cell_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, cell_vbo);
		glBufferData(GL_ARRAY_BUFFER, cells_size, cells, GL_STREAM_DRAW);

		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(2, 1, GL_INT, 3 * sizeof(i32), 0);
		glVertexAttribIPointer(3, 2, GL_INT, 3 * sizeof(i32), (void*)(1 * sizeof(i32)));
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
	}

	// Setup the shaders
	u32 vert_len;
	u32 frag_len;
	char* vert_src = file_read_all("res\\tiles.vert", &vert_len);
	char* frag_src = file_read_all("res\\tiles.frag", &frag_len);

	GLuint vert_shdr = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shdr, 1, &vert_src, &vert_len);
	glCompileShader(vert_shdr);

	GLuint frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shdr, 1, &frag_src, &frag_len);
	glCompileShader(frag_shdr);

	program = glCreateProgram();
	glAttachShader(program, vert_shdr);
	glAttachShader(program, frag_shdr);
	glLinkProgram(program);
	glUseProgram(program);

	char INFO_BUFFER[256];
	glGetProgramInfoLog(program, 256, NULL, INFO_BUFFER);
	log(INFO_BUFFER);

	// Load the font
	Tga_File font_tga;
	tga_load(&font_tga, "res/font.tga");

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &font_texture);
	glBindTexture(GL_TEXTURE_2D, font_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_tga.width, font_tga.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, font_tga.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	Tga_File color_tga;
	tga_load(&color_tga, "res/colors.tga");

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_2D, color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, color_tga.width, color_tga.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, color_tga.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Set cell uniforms
	GLint u_CellmapSize = glGetUniformLocation(program, "u_CellmapSize");
	GLint u_CellSize = glGetUniformLocation(program, "u_CellSize");
	GLint u_TilesetSize = glGetUniformLocation(program, "u_TilesetSize");
	GLint u_TilesetCols = glGetUniformLocation(program, "u_TilesetCols");
	GLint u_ColorMapSize = glGetUniformLocation(program, "u_ColorMapSize");
	GLint u_ColorSampler = glGetUniformLocation(program, "u_ColorSampler");
	glUniform2i(u_CellmapSize, CELL_COLS, CELL_ROWS);
	glUniform2i(u_CellSize, CELL_WIDTH, CELL_HEIGHT);
	glUniform2i(u_TilesetSize, font_tga.width, font_tga.height);
	glUniform1i(u_TilesetCols, TILESET_COLS);
	glUniform2i(u_ColorMapSize, color_tga.width, color_tga.height);
	glUniform1i(u_ColorSampler, 1);

	tga_free(&font_tga);
	tga_free(&color_tga);
}

void cells_render()
{
	// Update the cell vao
	u32 cells_size = sizeof(Cell) * CELL_COLS * CELL_ROWS;

	glBindBuffer(GL_ARRAY_BUFFER, cell_vbo);
	glBufferData(GL_ARRAY_BUFFER, cells_size, cells, GL_STREAM_DRAW);

	glBindVertexArray(vao);
	glUseProgram(program);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, CELL_COLS * CELL_ROWS);
}

void cell_set(Point pos, i32 glyph, i32 fg_color, i32 bg_color)
{
	Cell* cell = cell_get(pos);
	if (!cell)
		return;

	if (glyph >= 0)
		cell->glyph = glyph;
	if (fg_color >= 0)
		cell->fg_color = fg_color;
	if (bg_color >= 0)
		cell->bg_color = bg_color;
}

Point cell_write_str(Point pos, const char* str, i32 fg_color, i32 bg_color)
{
	u32 len = (u32)strlen(str);
	for(u32 i=0; i<len; ++i)
	{
		cell_set(pos, str[i], fg_color, bg_color);
		pos.x++;
	}

	return pos;
}