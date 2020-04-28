#include "board.h"
#include "cells.h"
#include <stdlib.h>

u8 get_direction(i32 x1, i32 y1, i32 x2, i32 y2)
{
	i32 dx = x2 - x1;
	i32 dy = y2 - y1;

	if (dy == 0)
	{
		if (dx > 0) return DIR_East;
		if (dx < 0) return DIR_West;
	}
	else
	{
		if (dy > 0) return DIR_South;
		if (dy < 0) return DIR_North;
	}

	return DIR_None;
}

/* NODES */
Node nodes[MAX_THINGS];
Node* connect_node = NULL;

Node* node_get(i32 x, i32 y)
{
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Node* node = &nodes[i];
		if (node->valid && node->x == x && node->y == y)
			return node;
	}

	return NULL;
}

Node* node_place(i32 x, i32 y)
{
	Node* node = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (nodes[i].valid)
			continue;

		node = &nodes[i];
	}

	assert(node != NULL);
	node->valid = true;
	node->x = x;
	node->y = y;

	// Should we intercept and split any connections?
	Connection conn = connection_get(x, y);
	if (conn.first)
	{
		node_remove_connection(conn.first, conn.second);
		node_remove_connection(conn.second, conn.first);
		node_connect(conn.first, node);
		node_connect(conn.second, node);
	}

	return node;
}

void node_delete(Node* node)
{
	// Remove the connection to other nodes
	for(u32 i=0; i<node->num_connections; ++i)
		node_remove_connection(node->connections[i], node);

	if (node == connect_node)
		connect_node = NULL;

	mem_zero(node, sizeof(Node));
}

void node_connect(Node* a, Node* b)
{
	bool a_con_b = false;
	bool b_con_a = false;

	// Check if they're already connected...
	for(u32 i=0; i<a->num_connections; ++i)
	{
		if (a->connections[i] == b)
		{
			a_con_b = true;
			break;
		}
	}
	for(u32 i=0; i<b->num_connections; ++i)
	{
		if (b->connections[i] == a)
		{
			b_con_a = true;
			break;
		}
	}

	assert(a_con_b == b_con_a);

	if (!a_con_b)
	{
		a->connections[a->num_connections] = b,
		a->num_connections++;
		assert(a->num_connections <= 4);
	}

	if (!b_con_a)
	{
		b->connections[b->num_connections] = a;
		b->num_connections++;
		assert(b->num_connections <= 4);
	}
}

void node_remove_connection(Node* node, Node* other)
{
	bool found = false;
	for(u32 i=0; i<node->num_connections; ++i)
	{
		// If we found the entry, move everything backwards
		if (found)
		{
			node->connections[i - 1] = node->connections[i];
		}
		// otherwise, look until we find the entry
		else
		{
			if (node->connections[i] == other)
				found = true;
		}
	}

	if (found)
		node->num_connections--;
}

void node_activate(Node* node)
{
	node->state = true;
	node->tic = tic_num;

	for(u32 i=0; i<node->num_connections; ++i)
	{
		Node* other = node->connections[i];
		if (other->tic != tic_num)
			node_activate(other);
	}
}

Connection connection_get(i32 x, i32 y)
{
	Connection conn;
	mem_zero(&conn, sizeof(conn));

	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Node* first = &nodes[i];
		if (!first->valid)
			continue;

		for(u32 c=0; c<first->num_connections; ++c)
		{
			Node* second = first->connections[c];
			if (first->x == second->x && first->x == x)
			{
				i32 min_y = min(first->y, second->y);
				i32 max_y = max(first->y, second->y);
				if (min_y < y && max_y > y)
				{
					conn.first = first;
					conn.second = second;
					break;
				}
			}
			else if (first->y == y)
			{
				i32 min_x = min(first->x, second->x);
				i32 max_x = max(first->x, second->x);
				if (min_x < x && max_x > x)
				{
					conn.first = first;
					conn.second = second;
					break;
				}
			}

		}
	}

	return conn;
}

/* INVERTERS */
Inverter inverters[MAX_THINGS];
Inverter* inverter_get(i32 x, i32 y)
{
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (inverters[i].valid && inverters[i].x == x && inverters[i].y == y)
			return &inverters[i];
	}

	return NULL;
}

Inverter* inverter_place(i32 x, i32 y)
{
	Inverter* inverter = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (!inverters[i].valid)
		{
			inverter = &inverters[i];
			break;
		}
	}

	assert(inverter != NULL);
	inverter->valid = true;
	inverter->x = x;
	inverter->y = y;

	// If we're placing on a connection, intercept the connection by placing two new nodes
	Connection conn = connection_get(x, y);
	if (conn.first)
	{
		Node* left;
		Node* right;

		left = node_get(x - 1, y);
		if (!left)
			left = node_place(x - 1, y);

		right = node_get(x + 1, y);
		if (!right)
			right = node_place(x + 1, y);

		node_remove_connection(left, right);
		node_remove_connection(right, left);
	}

	return inverter;
}

void inverter_delete(Inverter* inv)
{
	mem_zero(inv, sizeof(Inverter));
}

/* COMMENTS */
Comment comments[MAX_THINGS];
Comment* edit_comment = NULL;

Comment* comment_place(i32 x, i32 y)
{
	Comment* comment = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (!comments[i].valid)
		{
			comment = &comments[i];
			break;
		}
	}

	assert(comment);
	comment->valid = true;
	comment->x = x;
	comment->y = y;
	return comment;
}

/* BOARD */
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

Board board;
u32 tic_num = 0;

Thing thing_get(i32 x, i32 y)
{
	Thing thing;
	mem_zero(&thing, sizeof(thing));

	// Nodes
	Node* node = node_get(x, y);
	if (node)
	{
		thing.type = THING_Node;
		thing.ptr = node;
		return thing;
	}

	// Inverters
	Inverter* inv = inverter_get(x, y);
	if (inv)
	{
		thing.type = THING_Inverter;
		thing.ptr = inv;
		return thing;
	}

	return thing;
}

u32 things_get(i32 x1, i32 y1, i32 x2, i32 y2, Thing* out_things, u32 arr_size)
{
	i32 min_x = min(x1, x2);
	i32 max_x = max(x1, x2);
	i32 min_y = min(y1, y2);
	i32 max_y = max(y1, y2);

	u32 index = 0;
	for(i32 y=min_y; y<=max_y; ++y)
	{
		for(i32 x=min_x; x<=max_x; ++x)
		{
			Thing thing = thing_get(x, y);
			if (thing.type == THING_NULL)
				continue;

			out_things[index++] = thing;
			if (index >= arr_size)
				break;
		}
	}

	return index;
}

void board_init()
{
	board.cursor_x = 0;
	board.cursor_y = 0;

	mem_zero(nodes, sizeof(nodes));
	mem_zero(inverters, sizeof(inverters));
	mem_zero(comments, sizeof(comments));
}

void board_tic()
{
	tic_num++;

	// Set all inverter states...
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Inverter* inv = &inverters[i];
		if (!inv->valid)
			continue;

		Node* src_node = node_get(inv->x - 1, inv->y);
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
		nodes[i].state = false;
	}

	// Reactivate all nodes
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Inverter* inv = &inverters[i];
		if (!inv->valid || !inv->state)
			continue;

		Node* tar_node = node_get(inv->x + 1, inv->y);
		if (tar_node != NULL)
			node_activate(tar_node);
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

	// Draw nodes
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Node* node = &nodes[i];
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
		Inverter* inv = &inverters[i];
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
		Comment* comment = &comments[i];
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
}

void board_delete()
{
	if (!board.visual)
	{
		Thing thing = thing_get(board.cursor_x, board.cursor_y);
		if (!thing.ptr)
			return;

		switch(thing.type)
		{
			case THING_Node: node_delete(thing.ptr); break;
			case THING_Inverter: inverter_delete(thing.ptr); break;
		}
	}
	else
	{
		static Thing thing_arr[64];
		u32 num_things = things_get(board.vis_x, board.vis_y, board.cursor_x, board.cursor_y, thing_arr, 64);

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
	if (connect_node == NULL)
	{
		// Select or create a node to connect
		connect_node = node_get(board.cursor_x, board.cursor_y);
		if (connect_node == NULL)
			connect_node = node_place(board.cursor_x, board.cursor_y);
	}
	else
	{
		Node* target_node = NULL;

		// Find or create the target node
		Thing thing = thing_get(board.cursor_x, board.cursor_y);
		if (thing.type == THING_NULL)
		{
			target_node = node_place(board.cursor_x, board.cursor_y);
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
	inverter_place(board.cursor_x, board.cursor_y);
}

void board_place_comment()
{
	Comment* comment = comment_place(board.cursor_x, board.cursor_y);
	edit_comment = comment;
	board.cursor_x++;
	board.cursor_x++;
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

		case KEY_TIC: board_tic(); break;

		default: return false;
	}

	return true;
}