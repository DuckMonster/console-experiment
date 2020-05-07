#include "circuit.h"
#include <stdlib.h>
#include <stdio.h>

u32 tic_num = 0;

bool id_null(Thing_Id id)
{
	if (!id.generation)
		return true;

	return false;
}

u8 get_direction(Point from, Point to)
{
	if (from.y == to.y)
	{
		if (from.x < to.x) return DIR_East;
		if (from.x > to.x) return DIR_West;
	}
	else
	{
		if (from.y < to.y) return DIR_South;
		if (from.y > to.y) return DIR_North;
	}

	return DIR_None;
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

	Node* node = &circ->nodes[id.index];
	if (node->generation != id.generation)
		return NULL;

	if (!node->valid)
		return NULL;

	return node;
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
	node_update_state(circ, node);

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
			node_update_state(circ, other);
	}

	mem_zero(node, sizeof(Node));
}

Thing_Id node_id(Circuit* circ, Node* node)
{
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
	node_update_state(circ, a);
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

i32 recurse_num = 0;
// Updates the state of a node-batch
// If the node is on, it will spread it to its connections
// If it isn't, it will check if any of its connections recursively are powered and set itself
bool node_batch_contains_power(Circuit* circ, Node* node, i32 recurse_id)
{
	if (recurse_id < 0)
		recurse_id = ++recurse_num;

	if (node->recurse_id == recurse_id)
		return false;

	node->recurse_id = recurse_id;

	// If we have an active inverter to our left, we have a power source!
	Inverter* inv = inverter_find(circ, point_add(node->pos, point(-1, 0)));
	if (inv && inv->active)
		return true;

	// Otherwise, keep searching...
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other && node_batch_contains_power(circ, other, recurse_id))
			return true;
	}

	// Follow links
	if (node->link_type == LINK_Chip)
	{
		Chip* chip = chip_get(circ, node->link_chip);
		assert(chip);

		if (circuit_get_public_state(chip->circuit, node->link_index))
			return true;
	}

	return false;
}

void node_batch_set_state(Circuit* circ, Node* node, bool active, i32 recurse_id)
{
	if (recurse_id < 0)
		recurse_id = ++recurse_num;

	if (node->recurse_id == recurse_id)
		return;

	// If the state will change, make output inverters as dirty!
	if (node->state != active)
	{
		Inverter* inv = inverter_find(circ, point_add(node->pos, point(1, 0)));
		if (inv)
			inverter_make_dirty(circ, inv);
	}

	node->recurse_id = recurse_id;
	node->state = active;

	// Spread the state
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_batch_set_state(circ, other, active, recurse_id);
	}

	// Follow links
	if (node->link_type == LINK_Chip)
	{
		Chip* chip = chip_get(circ, node->link_chip);
		assert(chip);

		circuit_set_public_state(chip->circuit, node->link_index, active);
	}
}

void node_update_state(Circuit* circ, Node* node)
{
	bool has_power = node_batch_contains_power(circ, node, -1);
	node_batch_set_state(circ, node, has_power, -1);
}

void node_toggle_public(Circuit* circ, Node* node)
{
	if (node->link_type == LINK_Chip)
		return;

	if (node->link_type == LINK_Public)
	{
		for(u32 i=0; i<MAX_PUBLIC_NODES; ++i)
		{
			if (node_get(circ, circ->public_nodes[i]) == node)
			{
				zero_t(circ->public_nodes[i]);
				break;
			}
		}

		node->link_type = LINK_None;
	}
	else
	{
		for(u32 i=0; i<MAX_PUBLIC_NODES; ++i)
		{
			if (node_get(circ, circ->public_nodes[i]))
				continue;

			circ->public_nodes[i] = node_id(circ, node);
			node->link_type = LINK_Public;
			break;
		}
	}

	node_update_state(circ, node);
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
			node_update_state(circ, tar_node);
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

bool inverter_clean_up(Circuit* circ, Inverter* inv)
{
	if (!inv->dirty)
		return false;

	// Already cleaned this tic
	if (inv->tic == tic_num)
		return false;

	bool prev_active = inv->active;

	inv->tic = tic_num;
	inv->dirty = false;
	circ->dirty_inverters--;

	// Update state
	Node* src_node = node_find(circ, point_add(inv->pos, point(-1, 0)));
	if (src_node)
	{
		inv->active = !src_node->state;
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
			node_update_state(circ, tar_node);
		}
	}

	return true;
}

Chip* chip_find(Circuit* circ, Point pos)
{
	for(u32 i=0; i < circ->chip_num; i++)
	{
		if (circ->chips[i].valid && point_eq(circ->chips[i].pos, pos))
			return &circ->chips[i];
	}

	return NULL;
}

Chip* chip_get(Circuit* circ, Thing_Id id)
{
	if (id_null(id))
		return NULL;

	if (id.index >= MAX_CHIPS)
		return NULL;

	Chip* chip = &circ->chips[id.index];
	if (chip->generation != id.generation)
		return NULL;

	if (!chip->valid)
		return NULL;

	return chip;
}

Chip* chip_create(Circuit* circ, Point pos)
{
	Chip* chip = NULL;
	for(u32 i=0; i<MAX_CHIPS; ++i)
	{
		if (!circ->chips[i].valid)
		{
			chip = &circ->chips[i];
			break;
		}
	}

	assert(chip);
	chip->generation = ++circ->gen_num;
	chip->valid = true;
	chip->pos = pos;

	chip->circuit = circuit_make("CHIP");

	u32 index = chip - circ->chips;
	if (index >= circ->chip_num)
		circ->chip_num = index + 1;

	return chip;
}

Thing_Id chip_id(Circuit* circ, Chip* chip)
{
	Thing_Id id;
	id.index = chip - circ->chips;
	id.generation = chip->generation;
	return id;
}

void chip_delete(Circuit* circ, Chip* chip)
{
	mem_zero(chip, sizeof(Chip));
}

void chip_update(Circuit* circ, Chip* chip)
{
	for(u32 i=0; i<MAX_PUBLIC_NODES; ++i)
	{
		Node* pub_node = node_get(chip->circuit, chip->circuit->public_nodes[i]);
		Node* chp_node = node_get(circ, chip->link_nodes[i]);

		// Public nodes have changed!
		if (pub_node != chp_node)
		{
			// It was created...
			if (pub_node)
			{
				Point chp_node_pos = point_add(chip->pos, point(-1, 1 + i * 2));

				// Find or create a representative link node
				chp_node = node_find(circ, chp_node_pos);
				if (!chp_node)
					chp_node = node_create(circ, chp_node_pos);

				chp_node->link_type = LINK_Chip;
				chp_node->link_chip = chip_id(circ, chip);
				chp_node->link_index = i;
				chip->link_nodes[i] = node_id(circ, chp_node);
			}
			// It was destroyed, so destroy the chip node as well
			else
			{
				node_delete(circ, chp_node);
			}
		}

		if (chp_node)
			node_update_state(circ, chp_node);
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

	Inverter* inv = inverter_find(circ, pos);
	if (inv)
	{
		thing.type = THING_Inverter;
		thing.ptr = inv;
		return thing;
	}

	Chip* chip = chip_find(circ, pos);
	if (chip)
	{
		thing.type = THING_Chip;
		thing.ptr = chip;
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

bool circuit_run_tic(Circuit* circ)
{
	bool was_cleaned = false;
	for(u32 i=0; i<circ->inv_num; ++i)
	{
		Inverter* inv = &circ->inverters[i];
		if (!inv->valid)
			continue;

		bool clean = inverter_clean_up(circ, inv);
		was_cleaned |= clean;
	}

	return was_cleaned;
}

void circuit_tic(Circuit* circ)
{
	tic_num++;
	u32 iterations = 0;
	u32 num_cleaned_inverters = 0;

	// Update all chip input/outputs
	for(u32 i=0; i<circ->chip_num; ++i)
	{
		Chip* chip = &circ->chips[i];
		if (chip->valid)
			break;

		chip_update(circ, &circ->chips[i]);
	}

	// Update all inverters while there are dirty ones
	while(circ->dirty_inverters)
	{
		iterations++;
		bool was_cleaned = false;

		was_cleaned |= circuit_run_tic(circ);

		// All dirty inverters might have already ticked, but remain dirty
		// Continue cleaning next tic instead
		if (!was_cleaned)
			break;
	}

	for(u32 i=0; i<circ->chip_num; ++i)
	{
		Chip* chip = &circ->chips[i];
		if (!chip->valid)
			continue;

		circuit_tic(chip->circuit);
	}

	if (num_cleaned_inverters > 0)
	{
		log("TIC iter=%d", num_cleaned_inverters, iterations);
	}
}

void circuit_merge(Circuit* circ, Circuit* other)
{
	// Make sure they actually fit..
	assert((circ->node_num) + (other->node_num) < MAX_NODES);
	assert((circ->inv_num) + (other->inv_num) < MAX_INVERTERS);
	assert((circ->chip_num) + (other->chip_num) < MAX_INVERTERS);

	// We will put all of the nodes after the last node of the target
	// (+1 since node_last is the index of the last node, not the count)
	memcpy(circ->nodes + circ->node_num, other->nodes, sizeof(Node) * (other->node_num));
	memcpy(circ->inverters + circ->inv_num, other->inverters, sizeof(Inverter) * (other->inv_num));
	memcpy(circ->chips + circ->chip_num, other->chips, sizeof(Chip) * (other->chip_num));

	// After that we have to update all of the connection ID's, since the indecies have been shifted
	for(u32 i=circ->node_num; i<circ->node_num + other->node_num; ++i)
	{
		// We can do this for all connections, since NULL connections have 0 generation anyways
		for(u32 c=0; c<4; ++c)
			circ->nodes[i].connections[c].index += circ->node_num;
	}

	// Merge public nodes
	memcpy(circ->public_nodes, other->public_nodes, sizeof(circ->public_nodes));

	// Avoid repeat generation
	circ->gen_num = max(circ->gen_num, other->gen_num);
	circ->node_num += other->node_num;
	circ->inv_num += other->inv_num;
	circ->chip_num += other->chip_num;
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

#define fwrite_t(expr, file) (fwrite(&expr, sizeof(expr), 1, file))
#define fread_t(expr, file) (fread(&expr, sizeof(expr), 1, file))

void circuit_fwrite(Circuit* circ, FILE* file)
{
	fwrite_t(circ->name, file);
	fwrite_t(circ->gen_num, file);
	fwrite_t(circ->dirty_inverters, file);

	// Write nodes
	fwrite_t(circ->node_num, file);
	fwrite(circ->nodes, sizeof(Node), circ->node_num, file);

	// Write inverters
	fwrite_t(circ->inv_num, file);
	fwrite(circ->inverters, sizeof(Inverter), circ->inv_num, file);

	// Write chips
	fwrite_t(circ->chip_num, file);
	for(u32 i=0; i<circ->chip_num; ++i)
	{
		Chip* chip = &circ->chips[i];
		fwrite(chip, sizeof(Chip), 1, file);

		if (chip->valid)
			circuit_fwrite(chip->circuit, file);
	}

	// Write public nodes
	fwrite_t(circ->public_nodes, file);
}

void circuit_fread(Circuit* circ, FILE* file)
{
	mem_zero(circ, sizeof(Circuit));

	fread_t(circ->name, file);
	fread_t(circ->gen_num, file);
	fread_t(circ->dirty_inverters, file);

	// Read nodes
	fread_t(circ->node_num, file);
	fread(circ->nodes, sizeof(Node), circ->node_num, file);

	// Read inverters
	fread_t(circ->inv_num, file);
	fread(circ->inverters, sizeof(Inverter), circ->inv_num, file);

	// Read chips
	fread_t(circ->chip_num, file);
	for(u32 i=0; i<circ->chip_num; ++i)
	{
		Chip* chip = &circ->chips[i];
		fread(chip, sizeof(Chip), 1, file);

		if (chip->valid)
		{	
			chip->circuit = circuit_make("");
			circuit_fread(chip->circuit, file);
		}
	}

	// Read public nodes
	fread_t(circ->public_nodes, file);
}

void circuit_save(Circuit* circ, const char* path)
{
	FILE* file = fopen(path, "wb");
	assert(file != NULL);

	circuit_fwrite(circ, file);
	u32 bytes_written = ftell(file);

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

	circuit_fread(circ, file);
	u32 bytes_read = ftell(file);
	fclose(file);

	log("Loaded '%s'; %d bytes read", path, bytes_read);
}

bool circuit_get_public_state(Circuit* circ, u32 index)
{
	Node* node = node_get(circ, circ->public_nodes[index]);
	if (!node)
		return false;

	return node_batch_contains_power(circ, node, -1);
}

void circuit_set_public_state(Circuit* circ, u32 index, bool state)
{
	Node* node = node_get(circ, circ->public_nodes[index]);
	if (!node)
		return;

	state |= node_batch_contains_power(circ, node, -1);
	node_batch_set_state(circ, node, state, -1);
}