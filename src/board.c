#include "board.h"
#include "cells.h"
#include "circuit.h"
#include <stdlib.h>

Circuit* clipboard;
Node* connect_node = NULL;
Board board;

void board_init()
{
	// Make the base circuit
	board.edit_stack[0] = circuit_make("BASE");

	clipboard = circuit_make("CLIPBOARD");
}

void board_tic()
{
	// Tick the top entry on the stack first, let it trickle down through chips
	circuit_tic(board.edit_stack[0]);
}

void draw_edit_stack()
{
	Cell* cell = cells;
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
			Cell* cell = cell_get(point(x1, y));
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
			Cell* cell = cell_get(point(x, y1));
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
	for(u32 i=0; i < circ->node_num; i++)
	{
		Node* node = &circ->nodes[i];
		if (!node->valid)
			continue;

		Cell* cell = cell_get(node->pos);
		cell->glyph = GLPH_NODE;

		if (connect_node == node)
		{
			cell->fg_color = CLR_RED_1;
			cell->bg_color = CLR_RED_0;
		}
		else
		{
			if (node->power)
			{
				cell->fg_color = CLR_RED_0;
			}
			else
			{
				cell->fg_color = CLR_RED_1;
			}
		}

		for(u32 i=0; i<4; ++i)
		{
			Node* other = node_get(circ, node->connections[i]);
			if (!other)
				continue;

			draw_connection(node->pos.x, node->pos.y, other->pos.x, other->pos.y, node->power);
		}
	}

	// Draw inverters
	for(u32 i=0; i < circ->inv_num; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (!inv->valid)
			continue;

		Cell* cell = cell_get(inv->pos);
		cell->glyph = '>';

		if (inv->active)
		{
			cell->fg_color = CLR_RED_0;
		}
		else
		{
			cell->fg_color = CLR_RED_1;
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
		Cell* cursor_cell = cell_get(board.cursor);
		cursor_cell->bg_color = CLR_WHITE;
		cursor_cell->fg_color = CLR_BLACK;
	}
	else
	{
		// Draw visual selection box
		Rect vis_rect = rect(board.vis_origin, board.cursor);
		for(i32 y=vis_rect.min.y; y<=vis_rect.max.y; ++y)
		{
			for(i32 x=vis_rect.min.x; x<=vis_rect.max.x; ++x)
			{
				Cell* cell = cell_get(point(x, y));
				cell->bg_color = CLR_WHITE;
				cell->fg_color = CLR_BLACK;
			}
		}

		// Draw cursor
		Cell* cursor_cell = cell_get(board.cursor);
		cursor_cell->bg_color = CLR_ORNG_0;
		cursor_cell->fg_color = CLR_ORNG_1;
	}

	draw_edit_stack();
}

void delete_things(Circuit* circ, Thing* thing_arr, u32 count)
{
	for(u32 i=0; i<count; ++i)
	{
		switch(thing_arr[i].type)
		{
			case THING_Node:
			{
				node_delete(circ, thing_arr[i].ptr);
				if (connect_node == thing_arr[i].ptr)
					connect_node = NULL;
				break;
			}

			case THING_Inverter:
			{
				inverter_delete(circ, thing_arr[i].ptr);
				break;
			}
		}
	}
}

void board_delete()
{
	Circuit* circ = board_get_edit_circuit();
	if (board.visual)
	{
		static Thing thing_buf[64];
		u32 num_things = things_find(circ, rect(board.vis_origin, board.cursor), thing_buf, 64);

		delete_things(circ, thing_buf, num_things);
		board.visual = false;
	}
	else
	{
		Thing thing = thing_find(circ, board.cursor);
		delete_things(circ, &thing, 1);
	}
}

void board_place_node()
{
	Circuit* circ = board_get_edit_circuit();
	Node* node = node_find(circ, board.cursor);

	// No node, create one
	if (!node)
	{
		node = node_create(board_get_edit_circuit(), board.cursor);

		// Split connections if there are any
		Connection conn = connection_find(circ, board.cursor);
		if (conn.a)
		{
			node_disconnect(circ, conn.a, conn.b);
			node_connect(circ, conn.a, node);
			node_connect(circ, conn.b, node);
		}
	}

	if (connect_node)
	{
		// Re-selected the same node, stop connecting
		if (node == connect_node)
		{
			connect_node = NULL;
			return;
		}

		node_connect(circ, connect_node, node);
		connect_node = NULL;
	}
	else
	{
		connect_node = node;
	}
}

void board_place_inverter()
{
	Circuit* circ = board_get_edit_circuit();
	Point pos = board.cursor;

	// Split connections if there are any
	Connection conn = connection_find(circ, board.cursor);
	if (conn.a)
	{
		node_disconnect(circ, conn.a, conn.b);

		// For a horizontal connection, add in-betweeny nodes
		if (conn.a->pos.x != conn.b->pos.x)
		{
			Node* left_src;
			Node* right_src;
			if (conn.a->pos.x < conn.b->pos.x)
			{
				left_src = conn.a;
				right_src = conn.b;
			}
			else
			{
				left_src = conn.b;
				right_src = conn.a;
			}

			// Left in-betweeny
			if (!node_find(circ, point_add(pos, point(-1, 0))))
			{
				Node* node = node_create(circ, point_add(pos, point(-1, 0)));
				node_connect(circ, node, left_src);
			}
			// Righy betweeny
			if (!node_find(circ, point_add(pos, point(1, 0))))
			{
				Node* node = node_create(circ, point_add(pos, point(1, 0)));
				node_connect(circ, node, right_src);
			}
		}
	}

	// We're blocked...
	if (thing_find(circ, pos).type != THING_Null)
		return;

	inverter_create(circ, pos);
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
	board.cursor.x = min(max(board.cursor.x + dx, 0), CELL_COLS - 1);
	board.cursor.y = min(max(board.cursor.y + dy, 0), CELL_ROWS - 1);
}

void edit_stack_step_in()
{
}

void edit_stack_step_out()
{
}

void board_save()
{
	circuit_save(board_get_edit_circuit(), "res/test.circ");
}

void board_load()
{
	Circuit* loaded_circ = circuit_make("TEMP");
	circuit_load(loaded_circ, "res/test.circ");
	circuit_merge(board_get_edit_circuit(), loaded_circ);

	circuit_free(loaded_circ);
}

void board_yank()
{
	if (board.visual)
	{
		Rect v_rect = rect(board.vis_origin, board.cursor);
		circuit_copy(clipboard, board_get_edit_circuit());

		// Cull everything outside of the vis-rect
		{
			for(u32 i=0; i<clipboard->node_num; ++i)
			{
				Node* node = &clipboard->nodes[i];
				if (!point_in_rect(node->pos, v_rect))
					node_delete(clipboard, node);
			}
		}

		circuit_shift(clipboard, point_inv(v_rect.min));
		board.visual = false;
	}
}

void board_put()
{
	Point shift = board.cursor;
	circuit_shift(clipboard, shift);
	circuit_merge(board_get_edit_circuit(), clipboard);
	circuit_shift(clipboard, point_inv(shift));
}

bool board_key_event(u32 code, char chr)
{
	switch(code)
	{
		case KEY_PLACE_NODE: board_place_node(); break;
		case KEY_PLACE_INVERTER: board_place_inverter(); break;
		case KEY_PLACE_COMMENT: board_place_comment(); break;
		//case KEY_PLACE_CHIP: board_place_chip(); break;

		//case KEY_TOGGLE_LINK: board_toggle_link(); break;

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
			board.vis_origin = board.cursor;

			break;
		}

		case KEY_EDIT_STEP_IN: edit_stack_step_in(); break;
		case KEY_EDIT_STEP_OUT: edit_stack_step_out(); break;

		case KEY_YANK: board_yank(); break;
		case KEY_PUT: board_put(); break;

		case KEY_SAVE: board_save(); break;
		case KEY_LOAD: board_load(); break;

		case KEY_TIC: board_tic(); break;

		default: return false;
	}

	return true;
}

Circuit* board_get_edit_circuit()
{
	return board.edit_stack[board.edit_index];
}