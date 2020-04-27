#pragma once
#define MAX_THINGS 256

#define KEY_CANCEL 0x01
#define KEY_PLACE_NODE 0x11
#define KEY_PLACE_INVERTER 0x17
#define KEY_PLACE_COMMENT 0x2E
#define KEY_DELETE 0x2D
#define KEY_MOVE_LEFT 0x23
#define KEY_MOVE_DOWN 0x24
#define KEY_MOVE_UP 0x25
#define KEY_MOVE_RIGHT 0x26

enum Thing_Type
{
	THING_NULL,
	THING_Node,
	THING_Inverter,
	THING_Comment,
};

/* NODES */
typedef struct Node Node;
typedef struct Node
{
	bool valid;
	bool state;

	i32 x;
	i32 y;
	u32 tic;

	Node* connections[4];
	u32 num_connections;
} Node;

extern Node nodes[MAX_THINGS];
extern u32 tic_num;

Node* node_get(i32 x, i32 y);
Node* node_place(i32 x, i32 y);
void node_delete(Node* node);
void node_connect(Node* a, Node* b);
void node_remove_connection(Node* node, Node* other);
void node_activate(Node* node);

/* INVERTERS */
typedef struct
{
	bool valid;
	bool state;
	i32 x;
	i32 y;
	u32 tic;
} Inverter;

extern Inverter inverters[MAX_THINGS];
Inverter* inverter_get(i32 x, i32 y);
Inverter* inverter_place(i32 x, i32 y);
void inverter_delete(Inverter* inv);

/* COMMENTS */
#define COMMENT_MAX_LEN 20
typedef struct
{
	bool valid;
	i32 x;
	i32 y;
	char msg[COMMENT_MAX_LEN];
	u32 msg_len;
} Comment;

extern Comment comments[MAX_THINGS];
Comment* comment_place(i32 x, i32 y);

/* BOARD */
typedef struct
{
	i32 cursor_x;
	i32 cursor_y;
} Board;
extern Board board;

bool thing_get(i32 x, i32 y, void** out_ptr, u8* out_type);

void board_init();
void board_tic();
void board_draw();

void board_place_node();
bool board_key_event(u32 code, char chr);
void cursor_move(i32 dx, i32 dy);
