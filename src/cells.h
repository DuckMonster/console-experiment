#pragma once

#define CLR_BLACK 0x0;
#define CLR_WHITE 0x1;
#define CLR_RED_0 (0x8 + 0x0);
#define CLR_RED_1 (0x8 + 0x1);
#define CLR_ORNG_0 (0x8 + 0x6);
#define CLR_ORNG_1 (0x8 + 0x7);

typedef struct
{
	i32 glyph;
	i32 fg_color;
	i32 bg_color;
} Cell;
extern Cell* cells;

#define CELL_COLS 60
#define CELL_ROWS 30

#define CELL_WIDTH 6
#define CELL_HEIGHT 9

#define TILESET_COLS 18

void cells_init();
void cells_render();
void cell_set_string(u32 x, u32 y, const char* str);
void cell_set_char(u32 x, u32 y, char character);
inline Cell* cell_get(u32 x, u32 y) { return &cells[x + y * CELL_COLS]; }