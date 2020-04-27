#include "cells.h"
#include "gl_bind.h"
#include "import.h"
#include <stdlib.h>

GLuint vao;
GLuint vert_vbo;
GLuint cell_vbo;
GLuint program;
GLuint texture;

typedef struct 
{
	f32 x;
	f32 y;
	f32 u;
	f32 v;
} Cell_Vert;
Cell_Vert* cell_verts = NULL;
Cell* cells = NULL;

inline void vert_set(Cell_Vert* vert, f32 x, f32 y, f32 u, f32 v)
{
	vert->x = x;
	vert->y = y;
	vert->u = u;
	vert->v = v;
}

inline void cell_set(Cell* cell, i32 tile_x, i32 tile_y)
{
	cell->tile_x = tile_x;
	cell->tile_y = tile_y;
}

void cells_init()
{
	// Setup vertex objects
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	u32 cellverts_size = sizeof(Cell_Vert) * CELL_COLS * CELL_ROWS * 6;

	// Create cell vert buffer
	glGenBuffers(1, &vert_vbo);
	cell_verts = (Cell_Vert*)malloc(cellverts_size);

	Cell_Vert* vert_ptr = cell_verts;
	for(u32 y=0; y<CELL_ROWS; ++y)
	{
		for(u32 x=0; x<CELL_COLS; ++x)
		{
			vert_set(vert_ptr++, x, y, 0.f, 0.f);
			vert_set(vert_ptr++, x + 1.f, y, 1.f, 0.f);
			vert_set(vert_ptr++, x, y + 1.f, 0.f, 1.f);

			vert_set(vert_ptr++, x + 1.f, y, 1.f, 0.f);
			vert_set(vert_ptr++, x, y + 1.f, 0.f, 1.f);
			vert_set(vert_ptr++, x + 1.f, y + 1.f, 1.f, 1.f);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, vert_vbo);
	glBufferData(GL_ARRAY_BUFFER, cellverts_size, cell_verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(f32), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));

	// Create cell buffer
	glGenBuffers(1, &cell_vbo);

	u32 cells_size = sizeof(Cell) * CELL_COLS * CELL_ROWS * 6;
	cells = (Cell*)malloc(cells_size);
	mem_zero(cells, cells_size);

	glBindBuffer(GL_ARRAY_BUFFER, cell_vbo);
	glBufferData(GL_ARRAY_BUFFER, cells_size, cells, GL_STREAM_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 2, GL_INT, 2 * sizeof(u32), 0);

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
	Tga_File tga;
	tga_load(&tga, "res/font.tga");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tga.width, tga.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, tga.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Set cell uniforms
	GLint u_CellmapSize = glGetUniformLocation(program, "u_CellmapSize");
	GLint u_CellSize = glGetUniformLocation(program, "u_CellSize");
	GLint u_TilesetSize = glGetUniformLocation(program, "u_TilesetSize");
	glUniform2i(u_CellmapSize, CELL_COLS, CELL_ROWS);
	glUniform2i(u_CellSize, CELL_WIDTH, CELL_HEIGHT);
	glUniform2i(u_TilesetSize, tga.width, tga.height);

	tga_free(&tga);

	// Set some stuff
	cell_set_string(0, 0, "Hello, World! How are you doing?");
}

void cells_render()
{
	// Update the cell vao
	u32 cells_size = sizeof(Cell) * CELL_COLS * CELL_ROWS * 6;

	glBindBuffer(GL_ARRAY_BUFFER, cell_vbo);
	glBufferData(GL_ARRAY_BUFFER, cells_size, cells, GL_STREAM_DRAW);

	glBindVertexArray(vao);
	glUseProgram(program);
	glDrawArrays(GL_TRIANGLES, 0, CELL_COLS * CELL_ROWS * 6);
}

void cell_set_string(u32 x, u32 y, const char* str)
{
	u32 len = (u32)strlen(str);
	u32 cell_index = (x + y * (CELL_COLS)) * 6;

	Cell* cell_ptr = &cells[cell_index];
	for(u32 i=0; i<len; ++i)
	{
		u32 tile_x = str[i] % TILESET_COLS;
		u32 tile_y = str[i] / TILESET_COLS;

		for(u32 j=0; j<6; ++j)
			cell_set(cell_ptr++, tile_x, tile_y);
	}
}

void cell_set_char(u32 x, u32 y, char character)
{
	u32 tile_x = character % TILESET_COLS;
	u32 tile_y = character / TILESET_COLS;
	u32 cell_index = (x + y * (CELL_COLS)) * 6;

	Cell* cell_ptr = &cells[cell_index];
	for(u32 i=0; i<6; ++i)
		cell_set(cell_ptr++, tile_x, tile_y);
}