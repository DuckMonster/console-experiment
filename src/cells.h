#pragma once

typedef struct
{
	i32 tile_x;
	i32 tile_y;
} Cell;

#define CELL_COLS 25
#define CELL_ROWS 2

#define CELL_WIDTH 6
#define CELL_HEIGHT 9

#define TILESET_COLS 18

void cells_init();
void cells_render();
void cell_set_string(u32 x, u32 y, const char* str);
void cell_set_char(u32 x, u32 y, char character);