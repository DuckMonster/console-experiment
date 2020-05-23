#pragma once
#include "types.h"
#include <stdio.h>
typedef struct Circuit Circuit;

typedef struct
{
	u16 generation;
	u16 index;
} Thing_Id;
inline bool id_eq(Thing_Id a, Thing_Id b) { return memcmp(&a, &b, sizeof(Thing_Id)) == 0; }
bool id_null(Thing_Id id);
extern Thing_Id NULL_ID;

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
	THING_Delay = 1 << 3,
	THING_All = ~0
};

enum Thing_Flags
{
	FLAG_Active = 1 << 0,
	FLAG_Powered = 1 << 1,
};

#define THING_IMPL()\
u32 generation;\
u8 type;\
bool valid;\
bool dirty;\
u8 flags;\
u32 tic;\
\
Point pos;\
Point size\

typedef struct Thing
{
	THING_IMPL();

	// Padding data used by the different types of things
	u8 data[64];
} Thing;

void things_reserve(Circuit* circ, u32 num);
Thing* thing_create(Circuit* circ, u8 type, Point pos);
void thing_delete(Circuit* circ, Thing* thing);
Thing* thing_find(Circuit* circ, Point pos, u8 type_mask);
Thing* thing_get(Circuit* circ, Thing_Id id);
Thing_Id thing_id(Circuit* circ, Thing* thing);
u32 things_find(Circuit* circ, Rect rect, Thing** out_arr, u32 arr_size);
const char* thing_get_name(Thing* thing);
Rect thing_get_bbox(Thing* thing);

bool thing_flag_get(Thing* thing, u8 flag);
void thing_flag_set(Thing* thing, u8 flag, bool value);
inline bool thing_active(void* thing) { return thing_flag_get(thing, FLAG_Active); }
inline void thing_set_active(void* thing, bool active) { thing_flag_set(thing, FLAG_Active, active); }
inline bool thing_powered(void* thing) { return thing_flag_get(thing, FLAG_Powered); }
inline void thing_set_powered(void* thing, bool powered) { thing_flag_set(thing, FLAG_Powered, powered); }

bool _thing_it_inc(Circuit* circ, Thing** thing, u8 type_mask);

#define THINGS_FOREACH(circ, type_mask) for(Thing* it = circ->things; _thing_it_inc(circ, &it, (type_mask)); it++)

// Thing data
typedef void (*Thing_Delete_Proc)(Circuit* circ, void* thing);
typedef void (*Thing_Save_Proc)(Circuit* circ, void* thing, FILE* file);
typedef void (*Thing_Load_Proc)(Circuit* circ, void* thing, FILE* file);
typedef void (*Thing_Copy_Proc)(Circuit* circ, void* thing, void* other);
typedef void (*Thing_Merge_Proc)(Circuit* circ, void* thing, void* other);
typedef void (*Thing_Dirty_Proc)(Circuit* circ, void* thing);
typedef void (*Thing_Clean_Proc)(Circuit* circ, void* thing);

enum Thing_Push_Type
{
	PUSH_Top,
	PUSH_Bottom
};

typedef struct
{
	const char* name;
	Thing_Delete_Proc on_delete;
	Thing_Save_Proc on_save;
	Thing_Load_Proc on_load;
	Thing_Copy_Proc on_copy;
	Thing_Merge_Proc on_merge;
	Thing_Dirty_Proc on_dirty;
	Thing_Clean_Proc on_clean;
	u8 push_type;
} Thing_Type_Data;

Thing_Type_Data* thing_type_data(Thing* thing);

/* NODES */
enum Node_Link_Type
{
	LINK_None,
	LINK_Public,
	LINK_Chip,
};

typedef struct
{
	THING_IMPL();

	i32 recurse_id;

	u8 link_type;
	Thing_Id link_node;
	Thing_Id link_chip;

	Thing_Id connections[4];
} Node;

Node* node_find(Circuit* circ, Point pos);
Node* node_get(Circuit* circ, Thing_Id id);
Node* node_create(Circuit* circ, Point pos);
void node_on_deleted(Circuit* circ, Node* node);
void node_on_merge(Circuit* circ, Node* node, Node* other);
void node_on_copy(Circuit* circ, Node* node, Node* other);

void node_set_powered(Circuit* circ, Node* node, bool powered);

void node_update_state(Circuit* circ, Node* node);
void node_toggle_public(Circuit* circ, Node* node);

void node_connect(Circuit* circ, Node* a, Node* b);
void node_disconnect(Circuit* circ, Node* a, Node* b);

void node_on_clean(Circuit* circ, Node* node);

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
	THING_IMPL();
} Inverter;

Inverter* inverter_find(Circuit* circ, Point pos);
Inverter* inverter_create(Circuit* circ, Point pos);
void inverter_on_deleted(Circuit* circ, Inverter* inv);

void inverter_on_clean(Circuit* circ, Inverter* inv);

/* CHIP */
typedef struct 
{
	THING_IMPL();

	Circuit* circuit;
	Thing_Id* link_nodes;
} Chip;

Chip* chip_find(Circuit* circ, Point pos);
Chip* chip_get(Circuit* circ, Thing_Id id);
Chip* chip_create(Circuit* circ, Point pos);
void chip_on_deleted(Circuit* circ, Chip* chip);
void chip_on_save(Circuit* circ, Chip* chip, FILE* file);
void chip_on_load(Circuit* circ, Chip* chip, FILE* file);
void chip_on_copy(Circuit* circ, Chip* chip, Chip* other);
Thing_Id chip_id(Circuit* circ, Chip* chip);
void chip_delete(Circuit* circ, Chip* chip);

void chip_update(Circuit* circ, Chip* chip);

void chip_make_dirty(Circuit* circ, Chip* chip);
bool chip_clean_up(Circuit* circ, Chip* chip);

/* DELAY */
typedef struct
{
	THING_IMPL();
} Delay;

Delay* delay_find(Circuit* circ, Point pos);
Delay* delay_create(Circuit* circ, Point pos);
void delay_on_deleted(Circuit* circ, Delay* delay);
void delay_on_clean(Circuit* circ, Delay* delay);