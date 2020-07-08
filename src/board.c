#include "board.h"
#include "cells.h"
#include "circuit.h"
#include "context.h"
#include "prompt.h"
#include <stdlib.h>

Circuit* clipboard;
Thing_Id connect_node;
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
	if (!board.debug)
		circuit_tic(board.edit_stack[0]);
}

void cell_draw_off(Point pnt, i32 glyph, i32 fg_color, i32 bg_color)
{
	cell_set(point_sub(pnt, board.offset), glyph, fg_color, bg_color);
}

void cell_write_str_off(Point pnt, const char* str, i32 fg_color, i32 bg_color)
{
	cell_write_str(point_sub(pnt, board.offset), str, fg_color, bg_color);
}

void cell_or(Point pnt, i32 or_glyph)
{
	Cell* cell = cell_get(pnt);
	if (!cell)
		return;

	cell->glyph |= or_glyph;
}

void cell_or_offset(Point pnt, i32 or_glyph)
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

void draw_dirty_stack(Dirty_Stack* stack, Point offset, bool is_tic)
{
	Circuit* circ = board_get_edit_circuit();

	cell_write_str(point_add(point(0, CELL_ROWS - 2), offset), "----------", CLR_WHITE, (is_tic ? CLR_ORNG_0 : CLR_BLACK));

	static char debug_buff[50];
	for(u32 i=0; i<stack->count; ++i)
	{
		bool top = (i == stack->count - 1);
		Thing* thing = thing_get(circ, stack->list[i]);

		if (thing)
			sprintf(debug_buff, "%s (%d, %d)", thing_get_name(thing), thing->pos.x, thing->pos.y);
		else
			sprintf(debug_buff, "%s", "NULL");

		Point pos = point(0, CELL_ROWS - 3 - i);
		cell_write_str(point_add(pos, offset), debug_buff, CLR_WHITE, ((top && is_tic) ? CLR_BLUE_0 : CLR_BLACK));
	}
}

void draw_dirty_stack_overlay(Dirty_Stack* stack)
{
	Circuit* circ = board_get_edit_circuit();

	static char digit_buff[3];
	for(u32 i=0; i<stack->count; ++i)
	{
		u32 stack_index = stack->count - i - 1;
		Thing* thing = thing_get(circ, stack->list[stack_index]);
		if (!thing)
			continue;

		sprintf(digit_buff, "%d", i);
		cell_write_str_off(thing->pos, digit_buff, CLR_BLACK, CLR_WHITE);
	}
}

void draw_debug()
{
	static char debug_buff[50];
	Circuit* circ = board_get_edit_circuit();

	sprintf(debug_buff, "DEBUG TIC[%d]", tic);
	cell_write_str(point(0, CELL_ROWS - 1), debug_buff, CLR_WHITE, CLR_BLACK);

	// Draw the next thing to be ticked
	{
		Dirty_Stack* tic_stack = &circ->dirty_stacks[circ->stack_index];
		Thing* thing = dirty_stack_peek(tic_stack, circ);
		if (thing)
			cell_draw_off(thing->pos, -1, CLR_WHITE, CLR_BLUE_0);
	}

	draw_dirty_stack(&circ->dirty_stacks[0], point(0, 0), circ->stack_index == 0);
	draw_dirty_stack(&circ->dirty_stacks[1], point(20, 0), circ->stack_index == 1);

	if (board.debug_overlay)
		draw_dirty_stack_overlay(&circ->dirty_stacks[circ->stack_index]);
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
	THINGS_FOREACH(circ, THING_All)
	{
		switch(it->type)
		{
			// DRAW NODE
			case THING_Node:
			{
				Node* node = (Node*)it;
				cell_draw_off(node->pos, GLPH_NODE, CLR_RED_1, -1);

				if (thing_active(node))
					cell_draw_off(node->pos, -1, CLR_RED_0, -1);
				if (node->link_type == LINK_Public)
					cell_draw_off(node->pos, -1, -1, CLR_ORNG_1);
				if (node->link_type == LINK_Chip)
					cell_draw_off(node->pos, -1, -1, CLR_BLUE_0);

				for(u32 i=0; i<4; ++i)
				{
					Node* other = node_get(circ, node->connections[i]);
					if (!other)
						continue;

					u8 direction = get_direction(node->pos, other->pos);
					cell_or_offset(node->pos, direction);

					draw_connection(rect(node->pos, other->pos), node->flags & other->flags & FLAG_Active);
				}
				break;
			}

			// DRAW INVERTER
			case THING_Inverter:
			{
				Inverter* inv = (Inverter*)it;

				i32 color = CLR_RED_1;
				if (thing_active(inv))
					color = CLR_RED_0;

				cell_draw_off(inv->pos, '>', color, -1);
				break;
			}

			// DRAW CHIP
			case THING_Chip:
			{
				Chip* chip = (Chip*)it;
				Point pos = chip->pos;
				Point size = chip->size;

				for(i32 y=pos.y; y<pos.y + size.y; ++y)
					for(i32 x=pos.x; x<pos.x + size.x; ++x)
						cell_draw_off(point(x, y), ' ', CLR_WHITE, CLR_BLACK);

				break;
			}

			// DRAW DELAY
			case THING_Delay:
			{
				Delay* delay = (Delay*)it;
				i32 color = thing_active(delay) ? CLR_RED_0 : CLR_RED_1;

				cell_draw_off(delay->pos, 'o', color, -1);
				break;
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
	draw_edit_stack();

	if (board.debug)
		draw_debug();

	Node* connect_node_ptr = node_get(board_get_edit_circuit(), connect_node);
	if (connect_node_ptr)
	{
		cell_draw_off(connect_node_ptr->pos, -1, CLR_RED_1, CLR_RED_0);
	}

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
}

void delete_things(Circuit* circ, Thing** thing_arr, u32 count)
{
	for(u32 i=0; i<count; ++i)
	{
		thing_delete(circ, thing_arr[i]);
	}
}

void board_delete()
{
	Circuit* circ = board_get_edit_circuit();
	if (board.visual)
	{
		Rect vis_rect = rect(board.cursor, board.vis_origin);
		THINGS_FOREACH(circ, THING_All)
		{
			if (rect_rect_intersect(thing_get_bbox(it), vis_rect))
				thing_delete(circ, it);
		}

		board.visual = false;
	}
	else
	{
		Thing* thing = thing_find(circ, board.cursor, THING_All);
		if (thing)
			thing_delete(circ, thing);
	}
}

void board_place_node()
{
	Circuit* circ = board_get_edit_circuit();

	// Check if we're blocked...
	Thing* thing = thing_find(circ, board.cursor, THING_All);
	if (thing && thing->type != THING_Node)
		return;

	Node* node = (Node*)thing;

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

	Node* connect_node_ptr = node_get(circ, connect_node);

	if (connect_node_ptr)
	{
		// Re-selected the same node, stop connecting
		if (node == connect_node_ptr)
		{
			connect_node = NULL_ID;
			return;
		}

		node_connect(circ, connect_node_ptr, node);
		connect_node = NULL_ID;
	}
	else
	{
		connect_node = thing_id(circ, (Thing*)node);
	}
}

void board_split_connection(Connection conn, Point pos)
{
	Circuit* circ = board_get_edit_circuit();
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

void board_place_inverter()
{
	Circuit* circ = board_get_edit_circuit();
	Point pos = board.cursor;

	// We're blocked...
	if (thing_find(circ, pos, THING_All))
		return;

	// Split connections if there are any
	Connection conn = connection_find(circ, board.cursor);
	if (conn.a)
		board_split_connection(conn, board.cursor);

	inverter_create(circ, pos);
}

void board_place_comment()
{
}

void board_place_chip()
{
	Circuit* circ = board_get_edit_circuit();
	Point pos = board.cursor;

	// We're blocked...
	if (thing_find(circ, pos, THING_All))
		return;

	chip_create(board_get_edit_circuit(), board.cursor);
}

void board_place_delay()
{
	Circuit* circ = board_get_edit_circuit();
	Point pos = board.cursor;

	// We're blocked...
	if (thing_find(circ, pos, THING_All))
		return;

	// Split connections if there are any
	Connection conn = connection_find(circ, board.cursor);
	if (conn.a)
		board_split_connection(conn, pos);

	delay_create(board_get_edit_circuit(), board.cursor);
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
	circuit_save(board.edit_stack[0], "res/test.circ");
}

void board_load()
{
	circuit_load(board.edit_stack[0], "res/test.circ");
	board.edit_index = 0;
}

void board_yank()
{
	if (board.visual)
	{
		Rect v_rect = rect(board.vis_origin, board.cursor);
		circuit_copy_rect(clipboard, board_get_edit_circuit(), v_rect);

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
			case KEY_PLACE_DELAY: board_place_delay(); break;

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

			case KEY_SUBTIC: circuit_subtic(board.edit_stack[0]); break;
			case KEY_TIC: circuit_tic(board.edit_stack[0]); break;

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

			case KEY_TIC: board.debug = !board.debug; break;
			case KEY_SUBTIC: board.debug_overlay = !board.debug_overlay; break;
		}
	}

	return true;
}

Circuit* board_get_edit_circuit()
{
	return board.edit_stack[board.edit_index];
}