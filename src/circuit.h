#pragma once
#define MAX_THINGS 256
typedef struct Circuit Circuit;

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

extern u32 tic_num;

Node* node_get(Circuit* circ, i32 x, i32 y);
Node* node_place(Circuit* circ, i32 x, i32 y);
void node_delete(Node* node);
void node_connect(Node* a, Node* b);
void node_remove_connection(Node* node, Node* other);
void node_activate(Node* node);

typedef struct
{
	Circuit* circuit;

	Node* first;
	Node* second;
} Connection;

Connection connection_get(Circuit* circ, i32 x, i32 y);

/* INVERTERS */
typedef struct
{
	Circuit* circuit;

	bool valid;
	bool state;
	i32 x;
	i32 y;
	u32 tic;
} Inverter;

Inverter* inverter_get(Circuit* circ, i32 x, i32 y);
Inverter* inverter_place(Circuit* circ, i32 x, i32 y);
void inverter_delete(Inverter* inv);

/* COMMENTS */
#define COMMENT_MAX_LEN 20
typedef struct
{
	Circuit* circuit;

	bool valid;
	i32 x;
	i32 y;
	char msg[COMMENT_MAX_LEN];
	u32 msg_len;
} Comment;

Comment* comment_place(Circuit* circ, i32 x, i32 y);

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

Thing thing_get(Circuit* circ, i32 x, i32 y);
u32 things_get(Circuit* circ, i32 x1, i32 y1, i32 x2, i32 y2, Thing* out_things, u32 arr_size);

/* CIRCUIT */
typedef struct Circuit
{
	const char* name;

	Node nodes[MAX_THINGS];
	Inverter inverters[MAX_THINGS];
	Comment comments[MAX_THINGS];
} Circuit;