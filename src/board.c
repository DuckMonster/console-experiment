#include "board.h"
#include "cells.h"
#include "circuit.h"
#include "context.h"
#include "prompt.h"
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

void cell_draw_off(Point pnt, i32 glyph, i32 fg_color, i32 bg_color)
{
	cell_set(point_sub(pnt, board.offset), glyph, fg_color, bg_color);
}

void cell_or(Point pnt, i32 or_glyph)
{
	Cell* cell = cell_get(pnt);
	if (!cell)
		return;

	cell->glyph |= or_glyph;
}

void cell_or_off(Point pnt, i32 or_glyph)
{
	cell_or(point_sub(pnt, board.offset), or_glyph);
}

void draw_edit_stack()
{
	Cell* cell = cells;
	Point pos = point(0, 0);

	for(i32 i=0; i<=board.edit_index; ++i)
	{
		Circuit* circ = board.edit_stack[i];
		pos = cell_write_str(pos, circ->name, CLR_WHITE, CLR_BLACK);
		if (i < board.edit_index)
		{
			pos = cell_write_str(pos, " >> ", CLR_WHITE, CLR_BLACK);
		}
	}
}

void draw_connection(Rect rect, bool state)
{
	if (rect.min.x == rect.max.x)
	{
		i32 x = rect.min.x;
		for(i32 y=rect.min.y + 1; y<rect.max.y; ++y)
		{
			i32 glyph = cell_glyph_get(point_sub(point(x, y), board.offset));

			// Clear the cell if the glyph is not a wire
			if (glyph & ~GLPH_WIRE_X)
				glyph = 0;

			glyph |= GLPH_WIRE_V;

			i32 color = CLR_RED_1;
			if (state)
				color = CLR_RED_0;

			cell_draw_off(point(x, y), glyph, color, -1);
		}
	}
	else
	{
		i32 y = rect.min.y;

		for(i32 x=rect.min.x + 1; x<rect.max.x; ++x)
		{
			i32 glyph = cell_glyph_get(point_sub(point(x, y), board.offset));

			// Clear the cell if the glyph is not a wire
			if (glyph & ~GLPH_WIRE_X)
				glyph = 0;

			glyph |= GLPH_WIRE_H;

			i32 color = CLR_RED_1;
			if (state)
				color = CLR_RED_0;

			cell_draw_off(point(x, y), glyph, color, -1);
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

		cell_draw_off(node->pos, GLPH_NODE, CLR_RED_1, -1);
		if (connect_node == node)
		{
			cell_draw_off(node->pos, -1, CLR_RED_1, CLR_RED_0);
		}
		else
		{
			if (node->state)
				cell_draw_off(node->pos, -1, CLR_RED_0, -1);
			if (node->link_type == LINK_Public)
				cell_draw_off(node->pos, -1, -1, CLR_ORNG_1);
			if (node->link_type == LINK_Chip)
				cell_draw_off(node->pos, -1, -1, CLR_BLUE_0);
		}

		for(u32 i=0; i<4; ++i)
		{
			Node* other = node_get(circ, node->connections[i]);
			if (!other)
				continue;

			u8 direction = get_direction(node->pos, other->pos);
			cell_or_off(node->pos, direction);

			draw_connection(rect(node->pos, other->pos), node->state);
		}
	}

	// Draw inverters
	for(u32 i=0; i < circ->inv_num; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (!inv->valid)
			continue;

		i32 color = CLR_RED_1;
		if (inv->active)
			color = CLR_RED_0;

		cell_draw_off(inv->pos, '>', color, -1);
	}

	// Draw chips
	for(u32 i=0; i<circ->chip_num; ++i)
	{
		Chip* chip = &circ->chips[i];
		if (!chip->valid)
			continue;

		for(i32 y=chip->pos.y; y<=chip->pos.y + 5; ++y)
		{
			for(i32 x=chip->pos.x; x<=chip->pos.x + 5; ++x)
			{
				cell_draw_off(point(x, y), ' ', CLR_WHITE, CLR_BLACK);
			}
		}
	}
}

void board_draw()
{
	// Draw background!
	{
		Cell* cell_ptr = cells;
		for(i32 y=board.offset.y; y<board.offset.y + CELL_ROWS; ++y)
		{
			for(i32 x=board.offset.x; x<board.offset.x + CELL_COLS; ++x)
			{
				if (!(x % 10) && !(y % 10))
					cell_ptr->glyph = '+';
				else if (!(x % 2) && !(y % 10))
					cell_ptr->glyph = '-';
				else if (!(y % 2) && !(x % 10))
					cell_ptr->glyph = '|';
				else
					cell_ptr->glyph = ' ';

				cell_ptr->bg_color = CLR_BLUE_1;
				cell_ptr->fg_color = CLR_BLUE_0;
				cell_ptr++;
			}
		}
	}

	draw_circuit(board_get_edit_circuit());

	if (!board.visual)
	{
		Cell* cursor_cell = cell_get(point_sub(board.cursor, board.offset));
		cursor_cell->bg_color = CLR_WHITE;
		cursor_cell->fg_color = CLR_BLACK;
	}
	else
	{
		// Draw visual selection box
		Rect vis_rect = rect(board.vis_origin, board.cursor);
		vis_rect.min = point_sub(vis_rect.min, board.offset);
		vis_rect.max = point_sub(vis_rect.max, board.offset);
		for(i32 y=vis_rect.min.y; y<=vis_rect.max.y; ++y)
		{
			for(i32 x=vis_rect.min.x; x<=vis_rect.max.x; ++x)
			{
				Cell* cell = cell_get(point(x, y));
				if (!cell)
					continue;

				cell->bg_color = CLR_WHITE;
				cell->fg_color = CLR_BLACK;
			}
		}

		// Draw cursor
		Cell* cursor_cell = cell_get(point_sub(board.cursor, board.offset));
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

			case THING_Chip:
			{
				chip_delete(circ, thing_arr[i].ptr);
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
	chip_create(board_get_edit_circuit(), board.cursor);
}

void board_toggle_public()
{
	Circuit* circ = board_get_edit_circuit();
	Node* node = node_find(circ, board.cursor);
	if (!node)
		return;

	node_toggle_public(circ, node);
}

void board_comment_write(char chr)
{
}

void cursor_move(i32 dx, i32 dy)
{
	board.cursor.x = board.cursor.x + dx;
	board.cursor.y = board.cursor.y + dy;

	if (board.cursor.x < board.offset.x)
		board.offset.x = board.cursor.x;
	if (board.cursor.y < board.offset.y)
		board.offset.y = board.cursor.y;
	if (board.cursor.x >= board.offset.x + CELL_COLS)
		board.offset.x = board.cursor.x - CELL_COLS + 1;
	if (board.cursor.y >= board.offset.y + CELL_ROWS)
		board.offset.y = board.cursor.y - CELL_ROWS + 1;
}

void edit_stack_step_in()
{
	Chip* chip = chip_find(board_get_edit_circuit(), board.cursor);
	if (chip)
		board.edit_stack[++board.edit_index] = chip->circuit;
}

void edit_stack_step_out()
{
	if (board.edit_index == 0)
		return;

	board.edit_index--;
}

void board_save()
{
	circuit_save(board_get_edit_circuit(), "res/test.circ");
}

void board_load()
{
	circuit_load(board_get_edit_circuit(), "res/test.circ");
	board.edit_index = 0;
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

			for(u32 i=0; i<clipboard->inv_num; ++i)
			{
				Inverter* inv= &clipboard->inverters[i];
				if (!point_in_rect(inv->pos, v_rect))
					inverter_delete(clipboard, inv);
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

bool board_key_event(u32 code, char chr, u32 mods)
{
	if (!mods)
	{
		switch(code)
		{
			case KEY_PLACE_NODE: board_place_node(); break;
			case KEY_PLACE_INVERTER: board_place_inverter(); break;
			//case KEY_PLACE_COMMENT: board_place_comment(); break;
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

			case KEY_YANK: board_yank(); break;
			case KEY_PUT: board_put(); break;

			case KEY_TIC: board_tic(); break;

			default: return false;
		}
	}
	if (mods == MODK_CTRL)
	{
		switch(code)
		{
			case KEY_MOVE_RIGHT: edit_stack_step_in(); break;
			case KEY_MOVE_LEFT: edit_stack_step_out(); break;

			case KEY_TOGGLE_PUBLIC: board_toggle_public(); break;

			case KEY_SAVE: board_save(); break;
			case KEY_LOAD: board_load(); break;

			case KEY_DELETE: prompt_msg("Error", "This is an error"); break;
		}
	}

	return true;
}

Circuit* board_get_edit_circuit()
{
	return board.edit_stack[board.edit_index];
}