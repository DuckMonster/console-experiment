#pragma once
#define MAX_NODES 256

typedef struct Node Node;
typedef struct Node
{
	bool valid;
	i32 x;
	i32 y;

	Node* connections[4];
	u32 num_connections;
} Node;

extern Node* nodes;

Node* node_get(i32 x, i32 y);
Node* node_place(i32 x, i32 y);
void node_connect(Node* a, Node* b);

typedef struct
{
	i32 cursor_x;
	i32 cursor_y;
} Board;
extern Board board;

void board_init();
void board_draw();

void board_place_node();
void cursor_move(i32 dx, i32 dy);
