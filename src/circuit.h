#pragma once
#define MAX_NODES 256
#define MAX_LINKS 8
typedef struct Circuit Circuit;
typedef u32 thing_id;

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
	bool valid;
	bool state;

	i32 x;
	i32 y;
	u32 tic;
} Node;


/* CIRCUIT */
typedef struct Circuit
{
	const char* name;

	u32 last_node = 0;
	Node nodes[MAX_NODES];
} Circuit;

Circuit* circuit_make(const char* name);
void circuit_free(Circuit* circ);