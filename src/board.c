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

void node_connect(Node* a, Node* b)
{
	a->connections[a->num_connections] = b,
	a->num_connections++;
	assert(a->num_connections <= 4);

	b->connections[b->num_connections] = a;
	b->num_connections++;
	assert(b->num_connections <= 4);
}

void draw_connection(i32 x1, i32 y1, i32 x2, i32 y2)
{
	if (x1 == x2)
	{
		i32 min_y = min(y1, y2);
		i32 max_y = max(y1, y2);

		for(i32 y=min_y+1; y<max_y; ++y)
		{
			Cell* cell = cell_get(x1, y);
			cell->glyph = '|';
			cell->fg_color = CLR_RED_0;
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
			cell->fg_color = CLR_RED_0;
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

		// This node is selected
		if (node == connect_node)
		{
			cell->bg_color = CLR_RED_1;
			cell->fg_color = CLR_WHITE;
		}
		else
		{
			cell->fg_color = CLR_RED_0;
		}

		for(u32 j=0; j<node->num_connections; ++j)
		{
			Node* other = node->connections[j];
			draw_connection(
				node->x, node->y,
				other->x, other->y
			);
		}
	}

	Cell* cursor_cell = cell_get(board.cursor_x, board.cursor_y);
	cursor_cell->bg_color = CLR_WHITE;
	cursor_cell->fg_color = CLR_BLACK;
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
		connect_node = other_node;
	}
}

void cursor_move(i32 dx, i32 dy)
{
	board.cursor_x = min(max(board.cursor_x + dx, 0), CELL_COLS - 1);
	board.cursor_y = min(max(board.cursor_y + dy, 0), CELL_ROWS - 1);
}