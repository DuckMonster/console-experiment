#pragma once

#define CLR_BLACK 0x0
#define CLR_WHITE 0x1
#define CLR_RED_0 (0x8 + 0x0)
#define CLR_RED_1 (0x8 + 0x1)
#define CLR_BLUE_0 (0x8 + 0x4)
#define CLR_BLUE_1 (0x8 + 0x5)
#define CLR_ORNG_0 (0x8 + 0x6)
#define CLR_ORNG_1 (0x8 + 0x7)

#define GLPH_NODE (0x90)
#define GLPH_WIRE (0xA0)
#define GLPH_WIRE_H (GLPH_WIRE | 0x01)
#define GLPH_WIRE_V (GLPH_WIRE | 0x02)
#define GLPH_WIRE_X (GLPH_WIRE_H | GLPH_WIRE_V)
#define GLPH_BORDER (0xB0)

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

#define TILESET_COLS 0x10

void cells_init();
void cells_render();
void cell_set(Point pos, i32 glyph, i32 fg_color, i32 bg_color);
Point cell_write_str(Point pos, const char* str, i32 fg_color, i32 bg_color);

inline i32 cell_glyph_get(Point pos)
{
	if (pos.x < 0 || pos.y < 0 || pos.x >= CELL_COLS || pos.y >= CELL_ROWS)
		return -1;

	return cells[pos.x + pos.y * CELL_COLS].glyph;
}

inline Cell* cell_get(Point pos)
{
	if (pos.x < 0 || pos.y < 0 || pos.x >= CELL_COLS || pos.y >= CELL_ROWS)
		return NULL;

	return &cells[pos.x + pos.y * CELL_COLS];
}