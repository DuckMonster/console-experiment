#include "board.h"
#include "cells.h"
#include <stdlib.h>

Board board;
void board_init()
{
	board.cursor_x = 0;
	board.cursor_y = 0;
}

void board_render()
{
	Cell* cell_ptr = cells;
	for(u32 y=0; y<CELL_ROWS; ++y)
	{
		for(u32 x=0; x<CELL_COLS; ++x)
		{
			if (!(x % 10) && !(y % 10))
				cell_ptr->glyph = '+';
			else if (!(x % 2) && !(y % 10))
				cell_ptr->glyph = '-';
			else if (!(y % 2) && !(x % 10))
				cell_ptr->glyph = '|';
			else
				cell_ptr->glyph = ' ';

			cell_ptr->bg_color = 8 + 5;
			cell_ptr->fg_color = 8 + 4;
			cell_ptr++;
		}
	}

	Cell* cursor_cell = cell_get(board.cursor_x, board.cursor_y);
	cursor_cell->bg_color = 1;
	cursor_cell->fg_color = 0;
}

void cursor_move(i32 dx, i32 dy)
{
	board.cursor_x = min(max(board.cursor_x + dx, 0), CELL_COLS - 1);
	board.cursor_y = min(max(board.cursor_y + dy, 0), CELL_ROWS - 1);
}