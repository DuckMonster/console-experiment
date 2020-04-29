#include "board.h"
#include "cells.h"
#include "circuit.h"
#include <stdlib.h>

Board board;
u32 tic_num = 0;

void board_init()
{
	board.cursor_x = 0;
	board.cursor_y = 0;

	// Make the base circuit
	board.edit_stack[0] = circuit_make("BASE");
}

void tic_circuit(Circuit* circ)
{
}

void board_tic()
{
	tic_num++;

	// Tick the top entry on the stack first, let it trickle down through chips
	tic_circuit(board.edit_stack[0]);
}

void draw_edit_stack()
{
	Cell* cell = cell_get(0, 0);
	for(i32 i=0; i<=board.edit_index; ++i)
	{
		Circuit* circ = board.edit_stack[i];
		u32 name_len = (u32)strlen(circ->name);

		for(u32 c=0; c<name_len; ++c)
		{
			cell->bg_color = CLR_BLACK;
			cell->fg_color = CLR_WHITE;
			cell->glyph = circ->name[c];
			cell++;
		}

		if (i != board.edit_index)
		{
			cell->bg_color = CLR_BLACK;
			cell->fg_color = CLR_WHITE;
			cell->glyph = ' ';
			cell++;
			cell->bg_color = CLR_BLACK;
			cell->fg_color = CLR_WHITE;
			cell->glyph = '>';
			cell++;
			cell->bg_color = CLR_BLACK;
			cell->fg_color = CLR_WHITE;
			cell->glyph = ' ';
			cell++;
		}
	}
}

void draw_connection(i32 x1, i32 y1, i32 x2, i32 y2, bool state)
{
	if (x1 == x2)
	{
		i32 min_y = min(y1, y2);
		i32 max_y = max(y1, y2);

		for(i32 y=min_y+1; y<max_y; ++y)
		{
			Cell* cell = cell_get(x1, y);
			// Clear the cell if the glyph is not a wire
			if (cell->glyph & ~GLPH_WIRE_X)
				cell->glyph = 0;

			cell->glyph |= GLPH_WIRE_V;

			if (state)
			{
				cell->fg_color = CLR_RED_0;
			}
			else
			{
				cell->fg_color = CLR_RED_1;
			}
		}
	}
	else
	{
		i32 min_x = min(x1, x2);
		i32 max_x = max(x1, x2);

		for(i32 x=min_x+1; x<max_x; ++x)
		{
			Cell* cell = cell_get(x, y1);
			// Clear the cell if the glyph is not a wire
			if (cell->glyph & ~GLPH_WIRE_X)
				cell->glyph = 0;

			cell->glyph |= GLPH_WIRE_H;
			if (state)
			{
				cell->fg_color = CLR_RED_0;
			}
			else
			{
				cell->fg_color = CLR_RED_1;
			}
		}
	}
}

void draw_circuit(Circuit* circ)
{
}

void board_draw()
{
	// Draw background!
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
	}

	draw_circuit(board_get_edit_circuit());

	if (!board.visual)
	{
		Cell* cursor_cell = cell_get(board.cursor_x, board.cursor_y);
		cursor_cell->bg_color = CLR_WHITE;
		cursor_cell->fg_color = CLR_BLACK;
	}
	else
	{
		// Draw visual selection box
		i32 min_x = min(board.vis_x, board.cursor_x);
		i32 max_x = max(board.vis_x, board.cursor_x);
		i32 min_y = min(board.vis_y, board.cursor_y);
		i32 max_y = max(board.vis_y, board.cursor_y);
		for(i32 y=min_y; y<=max_y; ++y)
		{
			for(i32 x=min_x; x<=max_x; ++x)
			{
				Cell* cell = cell_get(x, y);
				cell->bg_color = CLR_WHITE;
				cell->fg_color = CLR_BLACK;
			}
		}

		// Draw cursor
		Cell* cursor_cell = cell_get(board.cursor_x, board.cursor_y);
		cursor_cell->bg_color = CLR_ORNG_0;
		cursor_cell->fg_color = CLR_ORNG_1;
	}

	draw_edit_stack();
}

void board_delete()
{
}

void board_place_node()
{
}

void board_place_inverter()
{
}

void board_place_comment()
{
}

void board_place_chip()
{
}

void board_toggle_link()
{
}

void board_comment_write(char chr)
{
}

void cursor_move(i32 dx, i32 dy)
{
	board.cursor_x = min(max(board.cursor_x + dx, 0), CELL_COLS - 1);
	board.cursor_y = min(max(board.cursor_y + dy, 0), CELL_ROWS - 1);
}

void edit_stack_step_in()
{
}

void edit_stack_step_out()
{
}

bool board_key_event(u32 code, char chr)
{
	switch(code)
	{
		case KEY_PLACE_NODE: board_place_node(); break;
		case KEY_PLACE_INVERTER: board_place_inverter(); break;
		case KEY_PLACE_COMMENT: board_place_comment(); break;
		case KEY_PLACE_CHIP: board_place_chip(); break;

		case KEY_TOGGLE_LINK: board_toggle_link(); break;

		case KEY_DELETE: board_delete(); break;
		case KEY_CANCEL: 
		{
			// Exit visual mode
			if (board.visual)
			{
				board.visual = false;
				break;
			}

			return false;
		}
		case KEY_MOVE_LEFT: cursor_move(-1, 0); break;
		case KEY_MOVE_DOWN: cursor_move(0, 1); break;
		case KEY_MOVE_UP: cursor_move(0, -1); break;
		case KEY_MOVE_RIGHT: cursor_move(1, 0); break;

		case KEY_VISUAL_MODE:
		{
			board.visual = !board.visual;
			board.vis_x = board.cursor_x;
			board.vis_y = board.cursor_y;

			break;
		}

		case KEY_EDIT_STEP_IN: edit_stack_step_in(); break;
		case KEY_EDIT_STEP_OUT: edit_stack_step_out(); break;

		case KEY_TIC: board_tic(); break;

		default: return false;
	}

	return true;
}

Circuit* board_get_edit_circuit()
{
	return board.edit_stack[board.edit_index];
}