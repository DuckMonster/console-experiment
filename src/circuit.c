#include "circuit.h"
#include <stdlib.h>
#include <stdio.h>

bool id_null(Circuit* circ, Thing_Id id)
{
	if (!id.generation)
		return true;

	if (circ->nodes[id.index].generation != id.generation)
		return true;

	return false;
}

/* NODES */
Node* node_find(Circuit* circ, Point pos)
{
	Node* node = NULL;
	for(u32 i=0; i < circ->node_num; i++)
	{
		if (circ->nodes[i].valid && point_eq(circ->nodes[i].pos, pos))
		{
			node = &circ->nodes[i];
			break;
		}
	}

	return node;
}

Node* node_get(Circuit* circ, Thing_Id id)
{
	if (id_null(circ, id))
		return NULL;

	return &circ->nodes[id.index];
}

Node* node_create(Circuit* circ, Point pos)
{
	assert(!node_find(circ, pos));

	Node* node = NULL;
	for(u32 i=0; i < MAX_NODES; ++i)
	{
		if (circ->nodes[i].valid)
			continue;

		node = &circ->nodes[i];
		break;
	}

	assert(node);
	node->generation = ++circ->gen_num;
	node->valid = true;
	node->pos = pos;

	u32 index = node - circ->nodes;
	if (index >= circ->node_num)
		circ->node_num = index + 1;

	return node;
}

void node_delete(Circuit* circ, Node* node)
{
	mem_zero(node, sizeof(Node));
}

Thing_Id node_id(Circuit* circ, Node* node)
{
	// ID's are offset by 1, so that ID 0 is considered NULL
	Thing_Id id;
	id.index = node - circ->nodes;
	id.generation = node->generation;
	return id;
}

void node_connect(Circuit* circ, Node* a, Node* b)
{
	assert(a != b);
	Thing_Id a_id = node_id(circ, a);
	Thing_Id b_id = node_id(circ, b);

	// First check if there already is a connection in some direction...
	bool a_conn_b = false;
	bool b_conn_a = false;
	for(u32 i=0; i<4; ++i)
	{
		a_conn_b |= id_eq(a->connections[i], b_id);
		b_conn_a |= id_eq(b->connections[i], a_id);
	}

	if (!a_conn_b)
	{
		// Connect a to b
		for(u32 i=0; i<4; ++i)
		{
			if (id_null(circ, a->connections[i]))
			{
				a->connections[i] = b_id;
				break;
			}
		}
	}

	if (!b_conn_a)
	{
		// Connect b to a
		for(u32 i=0; i<4; ++i)
		{
			if (id_null(circ, b->connections[i]))
			{
				b->connections[i] = a_id;
				break;
			}
		}
	}
}

/* THINGS */
Thing thing_find(Circuit* circ, Point pos)
{
	Thing thing;
	mem_zero(&thing, sizeof(thing));

	Node* node = node_find(circ, pos);
	if (node)
	{
		thing.type = THING_Node;
		thing.ptr = node;
		return thing;
	}

	return thing;
}

u32 things_find(Circuit* circ, Rect rect, Thing* out_arr, u32 arr_size)
{
	u32 index = 0;

	// Collect nodes
	for(u32 i=0; i<circ->node_num && index < arr_size; ++i)
	{
		Node* node = &circ->nodes[i];
		if (node->valid && point_in_rect(node->pos, rect))
		{
			out_arr[index].type = THING_Node;
			out_arr[index].ptr = node;
			index++;
		}
	}

	return index;
}

/* CIRCUIT */
Circuit* circuit_make(const char* name)
{
	Circuit* circ = malloc(sizeof(Circuit));
	mem_zero(circ, sizeof(Circuit));

	strcpy(circ->name, name);
	return circ;
}

void circuit_free(Circuit* circ)
{
	free(circ);
}

void circuit_merge(Circuit* circ, Circuit* other)
{
	// Make sure they actually fit..
	assert((circ->node_num) + (other->node_num) < MAX_NODES);

	// We will put all of the nodes after the last node of the target
	// (+1 since node_last is the index of the last node, not the count)
	memcpy(circ->nodes + circ->node_num, other->nodes, sizeof(Node) * (other->node_num));

	// After that we have to update all of the connection ID's, since the indecies have been shifted
	for(u32 i=circ->node_num; i<circ->node_num + other->node_num; ++i)
	{
		// We can do this for all connections, since NULL connections have 0 generation anyways
		for(u32 c=0; c<4; ++c)
			circ->nodes[i].connections[c].index += circ->node_num;
	}

	// Avoid repeat generation
	circ->gen_num = max(circ->gen_num, other->gen_num);
	circ->node_num += other->node_num;
}

void circuit_copy(Circuit* circ, Circuit* other)
{
	memcpy(circ, other, sizeof(Circuit));
}

void circuit_shift(Circuit* circ, Point amount)
{
	// Shift all nodes
	for(u32 i=0; i<circ->node_num; ++i)
		point_add(&circ->nodes[i].pos, amount);
}

void circuit_save(Circuit* circ, const char* path)
{
	FILE* file = fopen(path, "wb");
	assert(file != NULL);

	u32 bytes_written = (u32)fwrite(circ, 1, sizeof(Circuit), file);
	fclose(file);

	log("Saved to '%s'; %dB written", path, bytes_written);
}

void circuit_load(Circuit* circ, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file)
	{
		msg_box("Failed to load circuit '%s'; file not found", path);
		return;
	}

	mem_zero(circ, sizeof(Circuit));

	u32 bytes_read = (u32)fread(circ, 1, sizeof(Circuit), file);
	fclose(file);

	log("Loaded '%s'; %d bytes read", path, bytes_read);
}