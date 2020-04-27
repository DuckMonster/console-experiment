#pragma once
#define MAX_NODES 256

#define KEY_CANCEL 0x01
#define KEY_PLACE_NODE 0x11
#define KEY_DELETE 0x2D
#define KEY_MOVE_LEFT 0x23
#define KEY_MOVE_DOWN 0x24
#define KEY_MOVE_UP 0x25
#define KEY_MOVE_RIGHT 0x26
#define KEY_FLIP_STATE 0x21

typedef struct Node Node;
typedef struct Node
{
	bool valid;
	bool state;
	i32 x;
	i32 y;

	Node* connections[4];
	u32 num_connections;
} Node;

extern Node* nodes;

Node* node_get(i32 x, i32 y);
Node* node_place(i32 x, i32 y);
void node_delete(Node* node);
void node_connect(Node* a, Node* b);
void node_remove_connection(Node* node, Node* other);

typedef struct
{
	i32 cursor_x;
	i32 cursor_y;
} Board;
extern Board board;

void board_init();
void board_draw();

void board_place_node();
bool board_key_event(u32 code, char chr);
void cursor_move(i32 dx, i32 dy);
