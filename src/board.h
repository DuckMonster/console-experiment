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
#define KEY_VISUAL_MODE 0x2F
#define KEY_TIC 0x34

enum Direction
{
	DIR_None	= 0x0,
	DIR_East	= 0x1,
	DIR_North	= 0x2,
	DIR_West	= 0x4,
	DIR_South	= 0x8,
};
u8 get_direction(i32 x1, i32 y1, i32 x2, i32 y2);

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

typedef struct
{
	Node* first;
	Node* second;
} Connection;

Connection connection_get(i32 x, i32 y);

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
	bool visual;
	i32 vis_x;
	i32 vis_y;

	i32 cursor_x;
	i32 cursor_y;
} Board;
extern Board board;

/* THINGS */
enum Thing_Type
{
	THING_NULL,
	THING_Node,
	THING_Inverter,
	THING_Comment,
};

typedef struct
{
	u8 type;
	void* ptr;
} Thing;
Thing thing_get(i32 x, i32 y);
u32 things_get(i32 x1, i32 y1, i32 x2, i32 y2, Thing* out_things, u32 arr_size);

void board_init();
void board_tic();
void board_draw();

void board_place_node();
bool board_key_event(u32 code, char chr);
void cursor_move(i32 dx, i32 dy);
