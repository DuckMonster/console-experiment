#pragma once

typedef struct
{
	i32 glyph;
	i32 fg_color;
	i32 bg_color;
} Cell;
extern Cell* cells;

#define CELL_COLS 80
#define CELL_ROWS 40

#define CELL_WIDTH 6
#define CELL_HEIGHT 9

#define TILESET_COLS 18

void cells_init();
void cells_render();
void cell_set_string(u32 x, u32 y, const char* str);
void cell_set_char(u32 x, u32 y, char character);
inline Cell* cell_get(u32 x, u32 y) { return &cells[x + y * CELL_COLS]; }