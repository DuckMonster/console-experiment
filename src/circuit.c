#include "circuit.h"
#include <stdlib.h>
#include <stdio.h>

bool id_null(Thing_Id id)
{
	if (!id.generation)
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
	if (id_null(id))
		return NULL;

	if (circ->nodes[id.index].generation != id.generation)
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

	// Invalidate right away in case inverters are close-by
	//node_make_dirty(circ, node);

	return node;
}

void node_delete(Circuit* circ, Node* node)
{
	// Pre-invalidate for the coming updates...
	node->valid = false;

	// Deleting node will dirty its connections!
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_invalidate(circ, other);
	}

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
			if (!node_get(circ, a->connections[i]))
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
			if (!node_get(circ, b->connections[i]))
			{
				b->connections[i] = a_id;
				break;
			}
		}
	}

	// After a connection is made, the batch is invalidated
	// (since a and b are now connected, they will both be made dirty)
	node_invalidate(circ, a);
}

void node_disconnect(Circuit* circ, Node* a, Node* b)
{
	assert(a != b);
	Thing_Id a_id = node_id(circ, a);
	Thing_Id b_id = node_id(circ, b);

	for(u32 i=0; i<4; ++i)
	{
		if (id_eq(a->connections[i], b_id))
			zero_t(a->connections[i]);
		if (id_eq(b->connections[i], a_id))
			zero_t(b->connections[i]);
	}
}

void node_add_power(Circuit* circ, Node* node)
{
	// Already ticked...
	if (node->tic == circ->subtic_num)
		return;

	// If our state will change, make affected inverters dirty
	if (!node->power)
	{
		Inverter* inv = inverter_find(circ, point_add(node->pos, point(1, 0)));
		if (inv)
			inverter_make_dirty(circ, inv);
	}

	node->power++;
	node->tic = circ->subtic_num;

	log("Power++ (%d)", node->power);

	// Spread to connections
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_add_power(circ, other);
	}
}

void node_remove_power(Circuit* circ, Node* node)
{
	// Already ticked...
	if (node->tic == circ->subtic_num)
		return;

	// Cant decrease power below 0, somethings up...
	assert(node->power > 0);

	// If this will change the state, make affected inverters dirty
	if (node->power == 1)
	{
		Inverter* inv = inverter_find(circ, point_add(node->pos, point(1, 0)));
		if (inv)
			inverter_make_dirty(circ, inv);
	}

	node->power--;
	node->tic = circ->subtic_num;

	log("Power-- (%d)", node->power);

	// Spread to connections
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_remove_power(circ, other);
	}
}

void node_invalidate(Circuit* circ, Node* node)
{
	// Invalidating means the state of this node-network is no longer certain
	// Its triggered when adding/removing nodes, or connections
	if (node->tic >= circ->subtic_num)
		return;

	node->power = 0;
	node->tic = circ->subtic_num;

	// Spread to connections
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_invalidate(circ, other);
	}

	// The power of invalidated nodes is uncertain... tell source inverters to re-apply their power to the network
	Inverter* inv = inverter_find(circ, point_add(node->pos, point(-1, 0)));
	if (inv)
		inverter_invalidate(circ, inv);

	// Target inverters are simply made dirty
	inv = inverter_find(circ, point_add(node->pos, point(1, 0)));
	if (inv)
		inverter_make_dirty(circ, inv);
}

/* CONNECTIONS */
Connection connection_find(Circuit* circ, Point pos)
{
	Connection conn;
	mem_zero(&conn, sizeof(conn));

	for(u32 i=0; i<circ->node_num; ++i)
	{
		Node* node = &circ->nodes[i];
		if (!node->valid)
			continue;

		for(u32 c=0; c<4; ++c)
		{
			Node* other = node_get(circ, node->connections[c]);
			if (!other)
				continue;

			Rect con_rect = rect(node->pos, other->pos);
			if (point_in_rect(pos, con_rect))
			{
				conn.a = node;
				conn.b = other;
				return conn;
			}
		}
	}

	return conn;
}

/* INVERTERS */
Inverter* inverter_find(Circuit* circ, Point pos)
{
	for(u32 i=0; i<circ->inv_num; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (inv->valid && point_eq(inv->pos, pos))
			return inv;
	}

	return NULL;
}

Inverter* inverter_get(Circuit* circ, Thing_Id id)
{
	if (id_null(id))
		return NULL;

	if (circ->inverters[id.index].generation != id.generation)
		return NULL;

	return &circ->inverters[id.index];
}

Inverter* inverter_create(Circuit* circ, Point pos)
{
	Inverter* inv = NULL;
	for(u32 i=0; i<MAX_INVERTERS; ++i)
	{
		if (!circ->inverters[i].valid)
		{
			inv = &circ->inverters[i];
			break;
		}
	}

	if (inv == NULL)
	{
		msg_box("Ran out of inverters");
		return NULL;
	}

	inv->generation = ++circ->gen_num;
	inv->valid = true;
	inv->pos = pos;

	u32 index = inv - circ->inverters;
	if (index >= circ->inv_num)
		circ->inv_num = index + 1;

	// Inverters start out dirty!
	inverter_make_dirty(circ, inv);

	return inv;
}

void inverter_delete(Circuit* circ, Inverter* inv)
{
	// Pre-invalidate for consequence calculations..
	inv->valid = false;

	// Invalidate affected if we're powering it
	if (inv->active)
	{
		Node* tar_node = node_find(circ, point_add(inv->pos, point(1, 0)));
		if (tar_node)
			node_remove_power(circ, tar_node);
	}

	// Manually dec dirty counter if removing a dirty inverter
	if (inv->dirty)
	{
		circ->dirty_inverters--;
	}

	mem_zero(inv, sizeof(Inverter));
}

void inverter_make_dirty(Circuit* circ, Inverter* inv)
{
	if (inv->dirty)
		return;

	inv->dirty = true;
	circ->dirty_inverters++;
}

void inverter_invalidate(Circuit* circ, Inverter* inv)
{
	inverter_make_dirty(circ, inv);
	inv->active = false;
}

bool inverter_clean_up(Circuit* circ, Inverter* inv)
{
	if (!inv->dirty)
		return false;

	// Already cleaned this tic
	if (inv->tic == circ->tic_num)
		return false;

	bool prev_active = inv->active;

	inv->tic = circ->tic_num;
	inv->dirty = false;
	circ->dirty_inverters--;

	// Update state
	Node* src_node = node_find(circ, point_add(inv->pos, point(-1, 0)));
	if (src_node)
	{
		inv->active = !src_node->power;
	}
	else
	{
		inv->active = true;
	}

	// Did our state change?
	if (inv->active != prev_active)
	{
		// In that case, update nodes
		Node* tar_node = node_find(circ, point_add(inv->pos, point(1, 0)));
		if (tar_node)
		{
			circ->subtic_num++;
			if (inv->active)
			{
				node_add_power(circ, tar_node);
			}
			else
			{
				node_remove_power(circ, tar_node);
			}
		}
	}

	return true;
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

	Inverter* inv = inverter_find(circ, pos);
	if (inv)
	{
		thing.type = THING_Inverter;
		thing.ptr = inv;
		return thing;
	}

	return thing;
}

u32 things_find(Circuit* circ, Rect rect, Thing* out_arr, u32 arr_size)
{
	u32 index = 0;

	for(i32 y=rect.min.y; y<=rect.max.y; ++y)
	{
		for(i32 x=rect.min.x; x<=rect.max.x; ++x)
		{
			Thing thing = thing_find(circ, point(x, y));
			if (thing.type != THING_Null)
			{
				out_arr[index++] = thing;
				if (index == arr_size)
					break;
			}
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

void circuit_tic(Circuit* circ)
{
	circ->tic_num++;
	u32 iterations = 0;

	// Update all inverters while there are dirty ones
	while(circ->dirty_inverters)
	{
		iterations++;

		bool was_cleaned = false;
		for(u32 i=0; i<circ->inv_num; ++i)
		{
			Inverter* inv = &circ->inverters[i];
			if (!inv->valid)
				continue;

			circ->subtic_num++;
			was_cleaned |= inverter_clean_up(circ, inv);
		}

		// All dirty inverters might have already ticked, but remain dirty
		// Continue cleaning next tic instead
		if (!was_cleaned)
			break;
	}

	if (iterations > 0)
	{
		log("TIC[%d]", iterations);
	}

	// Allow for changes after the tic, during the construction next frame before tic
	circ->tic_num++;
	circ->subtic_num++;
}

void circuit_merge(Circuit* circ, Circuit* other)
{
	// Make sure they actually fit..
	assert((circ->node_num) + (other->node_num) < MAX_NODES);
	assert((circ->inv_num) + (other->inv_num) < MAX_INVERTERS);

	// We will put all of the nodes after the last node of the target
	// (+1 since node_last is the index of the last node, not the count)
	memcpy(circ->nodes + circ->node_num, other->nodes, sizeof(Node) * (other->node_num));
	memcpy(circ->inverters + circ->inv_num, other->inverters, sizeof(Inverter) * (other->inv_num));

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
	circ->inv_num += other->inv_num;
}

void circuit_copy(Circuit* circ, Circuit* other)
{
	memcpy(circ, other, sizeof(Circuit));
}

void circuit_shift(Circuit* circ, Point amount)
{
	// Shift all nodes
	for(u32 i=0; i<circ->node_num; ++i)
		circ->nodes[i].pos = point_add(circ->nodes[i].pos, amount);

	// Shift all inverters
	for(u32 i=0; i<circ->inv_num; ++i)
		circ->inverters[i].pos = point_add(circ->inverters[i].pos, amount);
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