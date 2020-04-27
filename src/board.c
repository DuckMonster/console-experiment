#include "board.h"
#include "cells.h"
#include <stdlib.h>

Node* nodes = NULL;
Node* connect_node = NULL;

Node* node_get(i32 x, i32 y)
{
	for(u32 i=0; i<MAX_NODES; ++i)
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
	for(u32 i=0; i<MAX_NODES; ++i)
	{
		if (nodes[i].valid)
			continue;

		node = &nodes[i];
	}

	assert(node != NULL);
	node->valid = true;
	node->x = x;
	node->y = y;

	return node;
}

void node_delete(Node* node)
{
	// Remove the connection to other nodes
	for(u32 i=0; i<node->num_connections; ++i)
		node_remove_connection(node->connections[i], node);

	mem_zero(node, sizeof(Node));
}

void node_connect(Node* a, Node* b)
{
	a->connections[a->num_connections] = b,
	a->num_connections++;
	assert(a->num_connections <= 4);

	b->connections[b->num_connections] = a;
	b->num_connections++;
	assert(b->num_connections <= 4);
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

	assert(found);
	node->num_connections--;
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
			cell->glyph = '|';
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
			cell->glyph = '-';
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
void board_init()
{
	board.cursor_x = 0;
	board.cursor_y = 0;

	nodes = (Node*)malloc(sizeof(Node) * MAX_NODES);
	mem_zero(nodes, sizeof(Node) * MAX_NODES);
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
	for(u32 i=0; i<MAX_NODES; ++i)
	{
		Node* node = &nodes[i];
		if (!node->valid)
			continue;

		Cell* cell = cell_get(node->x, node->y);
		cell->glyph = 'o';
		cell->fg_color = CLR_RED_1;

		for(u32 j=0; j<node->num_connections; ++j)
		{
			Node* other = node->connections[j];
			draw_connection(
				node->x, node->y,
				other->x, other->y,
				node->state
			);
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

	Cell* cursor_cell = cell_get(board.cursor_x, board.cursor_y);
	cursor_cell->bg_color = CLR_WHITE;
	cursor_cell->fg_color = CLR_BLACK;
}

void board_delete_node()
{
	Node* node = node_get(board.cursor_x, board.cursor_y);
	if (node != NULL)
		node_delete(node);
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
		// Either get an existing node to connect to, or create a new one
		Node* other_node = node_get(board.cursor_x, board.cursor_y);
		if (other_node == NULL)
			other_node = node_place(board.cursor_x, board.cursor_y);

		// If we selected the same node, stop trying to connect it
		if (other_node == connect_node)
		{
			connect_node = NULL;
			return;
		}

		node_connect(connect_node, other_node);
		connect_node = NULL;
	}
}

void cursor_move(i32 dx, i32 dy)
{
	board.cursor_x = min(max(board.cursor_x + dx, 0), CELL_COLS - 1);
	board.cursor_y = min(max(board.cursor_y + dy, 0), CELL_ROWS - 1);
}

bool board_key_event(u32 code, char chr)
{
	switch(code)
	{
		case KEY_PLACE_NODE: board_place_node(); break;
		case KEY_DELETE: board_delete_node(); break;
		case KEY_CANCEL: 
		{
			// Tried to cancel, but we aren't connecting anything
			if (connect_node == NULL) return false;

			connect_node = NULL;
			break;
		}
		case KEY_MOVE_LEFT: cursor_move(-1, 0); break;
		case KEY_MOVE_DOWN: cursor_move(0, 1); break;
		case KEY_MOVE_UP: cursor_move(0, -1); break;
		case KEY_MOVE_RIGHT: cursor_move(1, 0); break;
		default: return false;
	}

	return true;
}