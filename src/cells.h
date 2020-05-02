#pragma once

#define CLR_BLACK 0x0;
#define CLR_WHITE 0x1;
#define CLR_RED_0 (0x8 + 0x0);
#define CLR_RED_1 (0x8 + 0x1);
#define CLR_ORNG_0 (0x8 + 0x6);
#define CLR_ORNG_1 (0x8 + 0x7);

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
inline Cell* cell_get(Point pos) { return &cells[pos.x + pos.y * CELL_COLS]; }