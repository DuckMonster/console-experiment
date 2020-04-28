#include "board.h"
#include "cells.h"
#include "circuit.h"
#include <stdlib.h>

Node* connect_node = NULL;
Comment* edit_comment = NULL;

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
	// Set all inverter states...
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (!inv->valid)
			continue;

		Node* src_node = node_get(circ, inv->x - 1, inv->y);
		if (src_node == NULL)
		{
			inv->state = true;
		}
		else
		{
			inv->state = !src_node->state;
		}
	}

	// Deactivate all nodes...
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		circ->nodes[i].state = false;
	}

	// Reactivate all nodes
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (!inv->valid || !inv->state)
			continue;

		Node* tar_node = node_get(circ, inv->x + 1, inv->y);
		if (tar_node != NULL)
			node_activate(tar_node);
	}
}

void board_tic()
{
	tic_num++;
	tic_circuit(board_get_edit_circuit());
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
	// Draw nodes
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Node* node = &circ->nodes[i];
		if (!node->valid)
			continue;

		u8 dir_total = 0;
		for(u32 j=0; j<node->num_connections; ++j)
		{
			Node* other = node->connections[j];
			draw_connection(
				node->x, node->y,
				other->x, other->y,
				node->state
			);

			dir_total |= get_direction(node->x, node->y, other->x, other->y);
		}

		Cell* cell = cell_get(node->x, node->y);
		cell->glyph = GLPH_NODE + dir_total;
		if (node->state)
		{
			cell->fg_color = CLR_RED_0;
		}
		else
		{
			cell->fg_color = CLR_RED_1;
		}
	}

	// Draw inverters
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (!inv->valid)
			continue;

		Cell* cell = cell_get(inv->x, inv->y);
		cell->glyph = '>';
		if (inv->state)
		{
			cell->fg_color = CLR_RED_0;
		}
		else
		{
			cell->fg_color = CLR_RED_1;
		}
	}

	// If we're placing a node, draw a template connection
	if (connect_node != NULL)
	{
		Cell* cell = cell_get(connect_node->x, connect_node->y);
		cell->bg_color = CLR_RED_0;
		cell->fg_color = CLR_WHITE;

		draw_connection(connect_node->x, connect_node->y, board.cursor_x, board.cursor_y, connect_node->state);
	}

	// Draw comments
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Comment* comment = &circ->comments[i];
		if (!comment->valid)
			continue;

		Cell* cell = cell_get(comment->x, comment->y);
		cell->glyph = '#';
		cell->fg_color = CLR_ORNG_0;
		cell->bg_color = CLR_ORNG_1;
		cell++;

		cell->glyph = ' ';
		cell->fg_color = CLR_ORNG_0;
		cell->bg_color = CLR_ORNG_1;
		cell++;

		for(u32 s=0; s<comment->msg_len; ++s)
		{
			cell->glyph = comment->msg[s];
			cell->fg_color = CLR_ORNG_0;
			cell->bg_color = CLR_ORNG_1;
			cell++;
		}
	}

	// Draw chips
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Chip* chip = &circ->chips[i];
		if (!chip->valid)
			continue;

		for(i32 y=0; y<chip->height; ++y)
		{
			for(i32 x=0; x<chip->width; ++x)
			{
				Cell* cell = cell_get(chip->x + x, chip->y + y);
				cell->bg_color = CLR_RED_1;
				cell->fg_color = CLR_RED_0;
				cell->glyph = ' ';

				if (x == 0 || y == 0 || x == chip->width - 1 || y == chip->height - 1)
					cell->glyph = '#';
			}
		}
	}
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
	Circuit* circ = board_get_edit_circuit();

	if (!board.visual)
	{
		Thing thing = thing_get(circ, board.cursor_x, board.cursor_y);
		if (!thing.ptr)
			return;

		switch(thing.type)
		{
			case THING_Node:
			{
				if (connect_node == thing.ptr)
					connect_node = NULL;

				node_delete(thing.ptr);
				break;
			}
			case THING_Inverter: inverter_delete(thing.ptr); break;
		}
	}
	else
	{
		static Thing thing_arr[64];
		u32 num_things = things_get(circ, board.vis_x, board.vis_y, board.cursor_x, board.cursor_y, thing_arr, 64);

		for(u32 i=0; i<num_things; ++i)
		{
			switch(thing_arr[i].type)
			{
				case THING_Node: node_delete(thing_arr[i].ptr); break;
				case THING_Inverter: inverter_delete(thing_arr[i].ptr); break;
			}
		}

		board.visual = false;
	}
}

void board_place_node()
{
	Circuit* circ = board_get_edit_circuit();

	if (connect_node == NULL)
	{
		// Select or create a node to connect
		connect_node = node_get(circ, board.cursor_x, board.cursor_y);
		if (connect_node == NULL)
			connect_node = node_place(circ, board.cursor_x, board.cursor_y);
	}
	else
	{
		Node* target_node = NULL;

		// Find or create the target node
		Thing thing = thing_get(circ, board.cursor_x, board.cursor_y);
		if (thing.type == THING_NULL)
		{
			target_node = node_place(circ, board.cursor_x, board.cursor_y);
		}
		else
		{
			// Something else is placed here... abort!
			if (thing.type != THING_Node)
				return;

			target_node = thing.ptr;
		}

		// If we selected the same node, stop trying to connect it
		if (target_node == connect_node)
		{
			connect_node = NULL;
			return;
		}

		node_connect(connect_node, target_node);
		connect_node = NULL;
	}
}

void board_place_inverter()
{
	inverter_place(board_get_edit_circuit(), board.cursor_x, board.cursor_y);
}

void board_place_comment()
{
	Circuit* circ = board_get_edit_circuit();

	Comment* comment = comment_place(circ, board.cursor_x, board.cursor_y);
	edit_comment = comment;
	board.cursor_x += 2;
}

void board_place_chip()
{
	Circuit* circ = board_get_edit_circuit();
	chip_place(circ, board.cursor_x, board.cursor_y);
}

void board_comment_write(char chr)
{
	if (chr >= ' ' && chr <= '~')
	{
		if (edit_comment->msg_len == COMMENT_MAX_LEN)
			edit_comment->msg_len--;

		edit_comment->msg[edit_comment->msg_len] = chr;
		edit_comment->msg_len++;

		board.cursor_x = edit_comment->x + edit_comment->msg_len + 2;
		board.cursor_y = edit_comment->y;
	}
	else
	{
		switch(chr)
		{
			case 0xD:
			case 0xA:
			case 0x1B:
			{
				board.cursor_x = edit_comment->x;
				board.cursor_y = edit_comment->y;
				edit_comment = NULL;
				break;
			}

			case 0x8:
			{
				edit_comment->msg_len--;
				break;
			}
		}
	}
}

void cursor_move(i32 dx, i32 dy)
{
	board.cursor_x = min(max(board.cursor_x + dx, 0), CELL_COLS - 1);
	board.cursor_y = min(max(board.cursor_y + dy, 0), CELL_ROWS - 1);
}

void edit_stack_step_in()
{
	if (board.edit_index >= EDIT_STACK_SIZE - 1)
		return;

	Chip* chip = chip_get(board_get_edit_circuit(), board.cursor_x, board.cursor_y);
	if (chip == NULL)
		return;

	board.edit_stack[board.edit_index++] = chip->link_circuit;
}

void edit_stack_step_out()
{
	if (board.edit_index == 0)
		return;

	board.edit_index--;
}

bool board_key_event(u32 code, char chr)
{
	// Writing comments
	if (edit_comment)
	{
		board_comment_write(chr);
		return true;
	}

	switch(code)
	{
		case KEY_PLACE_NODE: board_place_node(); break;
		case KEY_PLACE_INVERTER: board_place_inverter(); break;
		case KEY_PLACE_COMMENT: board_place_comment(); break;
		case KEY_PLACE_CHIP: board_place_chip(); break;
		case KEY_DELETE: board_delete(); break;
		case KEY_CANCEL: 
		{
			// Exit visual mode
			if (board.visual)
			{
				board.visual = false;
				break;
			}

			if (connect_node != NULL)
			{
				connect_node = NULL;
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