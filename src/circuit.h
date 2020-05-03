#pragma once
#define MAX_NODES 256
#define MAX_LINKS 8

typedef struct Circuit Circuit;
typedef struct
{
	u16 generation;
	u16 index;
} Thing_Id;
inline bool id_eq(Thing_Id a, Thing_Id b) { return memcmp(&a, &b, sizeof(Thing_Id)) == 0; }
bool id_null(Circuit* circ, Thing_Id id);

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
typedef struct
{
	u16 generation;

	bool valid;
	bool state;

	Point pos;
	u32 tic;

	Thing_Id connections[4];
} Node;

Node* node_find(Circuit* circ, Point pos);
Node* node_get(Circuit* circ, Thing_Id id);
Node* node_create(Circuit* circ, Point pos);
void node_delete(Circuit* circ, Node* node);
Thing_Id node_id(Circuit* circ, Node* node);

void node_connect(Circuit* circ, Node* a, Node* b);

/* THINGS */
enum Thing_Type
{
	THING_Null,
	THING_Node,
};

typedef struct
{
	u8 type;
	void* ptr;
} Thing;

Thing thing_find(Circuit* circ, Point pos);
u32 things_find(Circuit* circ, Rect rect, Thing* out_arr, u32 arr_size);

/* CIRCUIT */
typedef struct Circuit
{
	char name[20];
	u16 gen_num;

	Node nodes[MAX_NODES];
	u32 node_num;
} Circuit;

Circuit* circuit_make(const char* name);
void circuit_free(Circuit* circ);
void circuit_merge(Circuit* circ, Circuit* other);
void circuit_copy(Circuit* circ, Circuit* other);
void circuit_shift(Circuit* circ, Point amount);

void circuit_save(Circuit* circ, const char* path);
void circuit_load(Circuit* circ, const char* path);