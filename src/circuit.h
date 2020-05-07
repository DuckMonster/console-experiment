#pragma once
#define MAX_THINGS 256
#define MAX_NODES 256
#define MAX_INVERTERS 256
#define MAX_CHIPS 8
#define MAX_PUBLIC_NODES 32

typedef struct Circuit Circuit;
typedef struct
{
	u16 generation;
	u16 index;
} Thing_Id;
inline bool id_eq(Thing_Id a, Thing_Id b) { return memcmp(&a, &b, sizeof(Thing_Id)) == 0; }
bool id_null(Thing_Id id);

enum Direction
{
	DIR_None	= 0x0,
	DIR_East	= 0x1,
	DIR_North	= 0x2,
	DIR_West	= 0x4,
	DIR_South	= 0x8,
};
u8 get_direction(Point from, Point to);

/* THINGS */
enum Thing_Type
{
	THING_Null = 0,
	THING_Node = 1 << 0,
	THING_Inverter = 1 << 1,
	THING_Chip = 1 << 2,
	THING_All = ~0
};

#define THING_IMPL()\
u32 generation;\
u8 type;\
bool valid;\
\
Point pos;\
Point size\

typedef struct
{
	THING_IMPL();

	// Padding data used by the different types of things
	u8 data[40];
} Thing;

Thing* thing_create(Circuit* circ, u8 type, Point pos);
Thing* thing_find(Circuit* circ, Point pos, u8 type_mask);
Thing* thing_get(Circuit* circ, Thing_Id id);
Thing_Id thing_id(Circuit* circ, Thing* thing);
u32 things_find(Circuit* circ, Rect rect, Thing** out_arr, u32 arr_size);
bool _thing_it_inc(Circuit* circ, Thing** thing, u8 type_mask);

#define THINGS_FOREACH(circ, type_mask) for(Thing* it = circ->things; _thing_it_inc(circ, &it, (type_mask)); it++)

/* NODES */
enum Node_Link_Type
{
	LINK_None,
	LINK_Public,
	LINK_Chip,
};

enum Node_State
{
	STATE_Off,
	STATE_On,
	STATE_Powered,
};

typedef struct
{
	THING_IMPL();

	u8 state;
	i32 recurse_id;

	u8 link_type;
	Thing_Id link_chip;
	u32 link_index;

	Thing_Id connections[4];
} Node;

Node* node_find(Circuit* circ, Point pos);
Node* node_get(Circuit* circ, Thing_Id id);
Node* node_create(Circuit* circ, Point pos);
void node_delete(Circuit* circ, Node* node);

void node_update_state(Circuit* circ, Node* node);
void node_toggle_public(Circuit* circ, Node* node);

void node_connect(Circuit* circ, Node* a, Node* b);
void node_disconnect(Circuit* circ, Node* a, Node* b);

/* CONNECTIONS */
typedef struct
{
	Node* a;
	Node* b;
} Connection;

Connection connection_find(Circuit* circ, Point pos);

/* INVERTERS */
typedef struct
{
	u16 generation;

	bool valid;
	bool active;
	bool dirty;

	Point pos;

	u32 tic;
} Inverter;

Inverter* inverter_find(Circuit* circ, Point pos);
Inverter* inverter_create(Circuit* circ, Point pos);
void inverter_delete(Circuit* circ, Inverter* inv);
void inverter_make_dirty(Circuit* circ, Inverter* inv);
void inverter_invalidate(Circuit* circ, Inverter* inv);
bool inverter_clean_up(Circuit* circ, Inverter* inv);

/* CHIP */
typedef struct 
{
	u16 generation;

	bool valid;
	Point pos;

	Circuit* circuit;
	Thing_Id link_nodes[MAX_PUBLIC_NODES];
} Chip;

Chip* chip_find(Circuit* circ, Point pos);
Chip* chip_get(Circuit* circ, Thing_Id id);
Chip* chip_create(Circuit* circ, Point pos);
Thing_Id chip_id(Circuit* circ, Chip* chip);
void chip_delete(Circuit* circ, Chip* chip);

void chip_update(Circuit* circ, Chip* chip);

/* CIRCUIT */
typedef struct Circuit
{
	char name[20];
	u16 gen_num;

	u32 tic_num;
	u32 dirty_inverters;

	Thing things[MAX_THINGS];
	u32 thing_num;

	Inverter inverters[MAX_INVERTERS];
	u32 inv_num;

	Chip chips[MAX_CHIPS];
	u32 chip_num;

	Thing_Id public_nodes[MAX_PUBLIC_NODES];
} Circuit;

Circuit* circuit_make(const char* name);
void circuit_free(Circuit* circ);

void circuit_tic(Circuit* circ);

void circuit_merge(Circuit* circ, Circuit* other);
void circuit_copy(Circuit* circ, Circuit* other);
void circuit_shift(Circuit* circ, Point amount);

void circuit_save(Circuit* circ, const char* path);
void circuit_load(Circuit* circ, const char* path);

bool circuit_get_public_state(Circuit* circ, u32 index);
void circuit_set_public_state(Circuit* circ, u32 index, bool state);