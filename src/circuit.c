#include "circuit.h"
#include <stdlib.h>
#include <stdio.h>

Thing_Type_Data type_data[] =
{
	// Thing types are bit-masks as well, so the type data need to be spaced accordingly...
	{NULL, NULL, NULL, NULL},
	// 1
	{node_on_deleted, NULL, NULL, NULL},
	// 2
	{inverter_on_deleted, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL},
	// 4
	{chip_on_deleted, chip_on_save, chip_on_load, chip_on_copy},
	{NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL},
	// 8
	{NULL, NULL, NULL, NULL},
};

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

/* THINGS */
Thing* thing_create(Circuit* circ, u8 type, Point pos)
{
	Thing* thing = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (circ->things[i].valid)
			continue;

		thing = &circ->things[i];
		break;
	}

	assert(thing);

	thing->generation = ++circ->gen_num;
	thing->type = type;
	thing->valid = true;
	thing->pos = pos;
	thing->size = point(1, 1);

	u32 index = thing - circ->things;
	if (index >= circ->thing_num)
		circ->thing_num = index + 1;

	return thing;
}

void thing_delete(Circuit* circ, Thing* thing)
{
	if (type_data[thing->type].delete_proc)
		type_data[thing->type].delete_proc(circ, thing);

	mem_zero(thing, sizeof(Thing));
}

Thing* thing_find(Circuit* circ, Point pos, u8 type_mask)
{
	THINGS_FOREACH(circ, type_mask)
	{
		// -1 on the rect, size 1 size means x.min == x.max
		Rect thing_rect = rect(it->pos, point_add(it->pos, point_sub(it->size, point(1, 1))));
		if (point_in_rect(pos, thing_rect))
			return it;
	}

	return NULL;
}

Thing* thing_get(Circuit* circ, Thing_Id id)
{
	if (id.generation == 0)
		return NULL;

	Thing* thing = &circ->things[id.index];
	if (!thing->valid)
		return NULL;

	if (thing->generation != id.generation)
		return NULL;

	return thing;
}

Thing_Id thing_id(Circuit* circ, Thing* thing)
{
	Thing_Id id;
	if (thing == NULL)
	{
		id.generation = 0;
		id.index = 0;
	}
	else
	{
		id.index = thing - circ->things;
		id.generation = thing->generation;
	}

	return id;
}

bool _thing_it_inc(Circuit* circ, Thing** thing, u8 type_mask)
{
	if (*thing == NULL)
		return false;

	u32 index = *thing - circ->things;
	while(index < circ->thing_num)
	{
		if ((*thing)->valid && ((*thing)->type & type_mask))
			return true;

		(*thing)++;
		index++;
	}

	return false;
}

u32 things_find(Circuit* circ, Rect rect, Thing** out_arr, u32 arr_size)
{
	u32 index = 0;
	THINGS_FOREACH(circ, THING_All)
	{
		if (point_in_rect(it->pos, rect))
			out_arr[index++] = it;
	}

	return index;
}


/* NODES */
Node* node_find(Circuit* circ, Point pos)
{
	return (Node*)thing_find(circ, pos, THING_Node);
}

Node* node_get(Circuit* circ, Thing_Id id)
{
	return (Node*)thing_get(circ, id);
}

Node* node_create(Circuit* circ, Point pos)
{
	assert(sizeof(Node) <= sizeof(Thing));
	Node* node = (Node*)thing_create(circ, THING_Node, pos);

	// Dirty up close-by inverters
	Inverter* inv = inverter_find(circ, point_sub(pos, point(1, 0)));
	if (inv)
		inverter_make_dirty(circ, inv);

	// Invalidate right away in case inverters are close-by
	node_update_state(circ, node);

	return node;
}

void node_on_deleted(Circuit* circ, void* ptr)
{
	Node* node = ptr;
	node->valid = false;

	// Deleting node will dirty its connections!
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_update_state(circ, other);
	}
}

void node_set_powered(Circuit* circ, Node* node, bool powered)
{
	if (powered)
	{
		node->state |= POWER_Powered;
	}
	else
	{
		node->state &= ~POWER_Powered;
	}

	node_update_state(circ,node);
}

void node_connect(Circuit* circ, Node* a, Node* b)
{
	assert(a != b);
	Thing_Id a_id = thing_id(circ, (Thing*)a);
	Thing_Id b_id = thing_id(circ, (Thing*)b);

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
	Thing_Id a_id = thing_id(circ, (Thing*)a);
	Thing_Id b_id = thing_id(circ, (Thing*)b);

	for(u32 i=0; i<4; ++i)
	{
		if (id_eq(a->connections[i], b_id))
			zero_t(a->connections[i]);
		if (id_eq(b->connections[i], a_id))
			zero_t(b->connections[i]);
	}
}

i32 recurse_num = 0;
// Check if this node batch contains a powered node
bool node_batch_contains_power(Circuit* circ, Node* node, i32 recurse_id)
{
	if (recurse_id < 0)
		recurse_id = ++recurse_num;

	if (node->recurse_id == recurse_id)
		return false;

	node->recurse_id = recurse_id;

	// This node is powered!
	if (node->state & POWER_Powered)
		return true;

	// Otherwise, keep searching...
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other && node_batch_contains_power(circ, other, recurse_id))
			return true;
	}

	// Follow links
	if (node->link_type != LINK_None)
	{
		Node* link_node_ptr = NULL;
		Circuit* link_circ = NULL;

		// Get which circuit to poll
		if (node->link_type == LINK_Chip)
		{
			Chip* chip = chip_get(circ, node->link_chip);
			assert(chip);

			link_circ = chip->circuit;
		}
	 	else
		{
			link_circ = circ->parent;
		}

		// Then poll the link node
		if (link_circ)
		{
			Node* other = node_get(link_circ, node->link_node);
			if (other && node_batch_contains_power(link_circ, other, recurse_id))
				return true;
		}
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
	if ((node->state & POWER_On) != active)
	{
		Inverter* inv = inverter_find(circ, point_add(node->pos, point(1, 0)));
		if (inv)
			inverter_make_dirty(circ, inv);

		// Also if this is a link-node to a chip, make the chip dirty
		Chip* chip = chip_get(circ, node->link_chip);
		if (chip)
			chip_make_dirty(circ, chip);
	}

	node->recurse_id = recurse_id;
	if (active)
	{
		node->state |= POWER_On;
	}
	else
	{
		node->state &= ~POWER_On;
	}

	// Spread the state
	for(u32 i=0; i<4; ++i)
	{
		Node* other = node_get(circ, node->connections[i]);
		if (other)
			node_batch_set_state(circ, other, active, recurse_id);
	}


	// Follow links
	if (node->link_type != LINK_None)
	{
		Circuit* link_circ = NULL;

		// Get which circuit to poll
		if (node->link_type == LINK_Chip)
		{
			Chip* chip = chip_get(circ, node->link_chip);
			assert(chip);

			link_circ = chip->circuit;
		}
	 	else
		{
			link_circ = circ->parent;
		}

		// Then poll the link node
		if (link_circ)
		{
			Node* other = node_get(link_circ, node->link_node);
			if (other)
				node_batch_set_state(link_circ, other, active, recurse_id);
		}
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

			circ->public_nodes[i] = thing_id(circ, (Thing*)node);
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

	THINGS_FOREACH(circ, THING_Node)
	{
		Node* node = (Node*)it;

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
	return (Inverter*)thing_find(circ, pos, THING_Inverter);
}

Inverter* inverter_create(Circuit* circ, Point pos)
{
	assert(sizeof(Inverter) <= sizeof(Thing));

	Inverter* inv = (Inverter*)thing_create(circ, THING_Inverter, pos);

	// Inverters start out dirty!
	inverter_make_dirty(circ, inv);

	return inv;
}

void inverter_on_deleted(Circuit* circ, void* ptr)
{
	Inverter* inv = ptr;
	inv->valid = false;

	// If there is a target-node to this inverter, update it
	Node* node = node_find(circ, point_add(inv->pos, point(1, 0)));
	if (node)
		node_set_powered(circ, node, false);

	if (inv->dirty)
		circ->dirty_counter--;
}

void inverter_make_dirty(Circuit* circ, Inverter* inv)
{
	if (inv->dirty)
		return;

	inv->dirty = true;
	circ->dirty_counter++;
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
	circ->dirty_counter--;

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

	Node* tar_node = node_find(circ, point_add(inv->pos, point(1, 0)));
	if (tar_node)
	{
		bool tar_node_powered = tar_node->state & POWER_Powered;

		// Did our state change?
		if (inv->active != tar_node_powered)
			node_set_powered(circ, tar_node, inv->active);
	}

	return true;
}

Chip* chip_find(Circuit* circ, Point pos)
{
	return (Chip*)thing_find(circ, pos, THING_Chip);
}

Chip* chip_get(Circuit* circ, Thing_Id id)
{
	return (Chip*)thing_get(circ, id);
}

Chip* chip_create(Circuit* circ, Point pos)
{
	assert(sizeof(Chip) <= sizeof(Thing));
	Chip* chip = (Chip*)thing_create(circ, THING_Chip, pos);

	chip->size = point(3, 5);
	chip->circuit = circuit_make("CHIP");
	chip->circuit->parent = circ;
	chip->link_nodes = malloc(sizeof(Thing_Id) * MAX_PUBLIC_NODES);
	mem_zero(chip->link_nodes, sizeof(Thing_Id) * MAX_PUBLIC_NODES);

	return chip;
}

void chip_on_deleted(Circuit* circ, void* ptr)
{

}


void circuit_fwrite(Circuit* circ, FILE* file);
void circuit_fread(Circuit* circ, FILE* file);
void chip_on_save(Circuit* circ, void* ptr, FILE* file)
{
	Chip* chip = ptr;
	circuit_fwrite(chip->circuit, file);
	fwrite(chip->link_nodes, sizeof(Thing_Id), MAX_PUBLIC_NODES, file);
}

void chip_on_load(Circuit* circ, void* ptr, FILE* file)
{
	Chip* chip = ptr;
	chip->circuit = circuit_make("CHIP");
	circuit_fread(chip->circuit, file);
	chip->circuit->parent = circ;

	chip->link_nodes = malloc(sizeof(Thing_Id) * MAX_PUBLIC_NODES);
	fread(chip->link_nodes, sizeof(Thing_Id), MAX_PUBLIC_NODES, file);
}

void chip_on_copy(Circuit* circ, void* ptr, void* other)
{
	Chip* chip = ptr;
	Chip* chip_other = other;

	chip->circuit = circuit_make("CHIP");
	circuit_copy(chip->circuit, chip_other->circuit);
	chip->circuit->parent = circ;

	chip->link_nodes = malloc(sizeof(Thing_Id) * MAX_PUBLIC_NODES);
	mem_zero(chip->link_nodes, sizeof(Thing_Id) * MAX_PUBLIC_NODES);
}

void chip_update(Circuit* circ, Chip* chip)
{
	circuit_run_tic(chip->circuit);
	u32 max_y = 2;

	for(u32 i=0; i<MAX_PUBLIC_NODES; ++i)
	{
		Node* pub_node = node_get(chip->circuit, chip->circuit->public_nodes[i]);
		Node* chp_node = node_get(circ, chip->link_nodes[i]);

		// Weird check, if they're not both NULL or not both some value
		if ((pub_node == NULL) != (chp_node == NULL))
		{
			// It was created...
			if (pub_node)
			{
				Point chp_node_pos = point_add(chip->pos, point(-1, 1 + i));

				// Find or create a representative link node
				chp_node = node_find(circ, chp_node_pos);
				if (!chp_node)
					chp_node = node_create(circ, chp_node_pos);

				chp_node->link_type = LINK_Chip;
				chp_node->link_chip = thing_id(circ, (Thing*)chip);
				chp_node->link_node = thing_id(chip->circuit, (Thing*)pub_node);

				pub_node->link_type = LINK_Public;
				pub_node->link_node = thing_id(circ, (Thing*)chp_node);

				chip->link_nodes[i] = thing_id(circ, (Thing*)chp_node);
			}
			// It was destroyed, so destroy the chip node as well
			else
			{
				thing_delete(circ, (Thing*)chp_node);
			}
		}

		if (chp_node)
		{
			node_update_state(circ, chp_node);
			max_y = 1 + i;
		}
	}

	chip->size.y = max_y + 2;
}

void chip_make_dirty(Circuit* circ, Chip* chip)
{
	if (chip->dirty)
		return;

	chip->dirty = true;
	circ->dirty_counter++;
}

bool chip_clean_up(Circuit* circ, Chip* chip)
{
	if (!chip->dirty)
		return false;

	chip->dirty = false;
	circ->dirty_counter--;

	chip_update(circ, chip);
	return true;
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

u32 clean_num = 0;
void circuit_run_tic(Circuit* circ)
{
	// Start by force-updating all the chips
	THINGS_FOREACH(circ, THING_Chip)
	{
		chip_update(circ, (Chip*)it);
	}

	bool was_cleaned = false;
	while(circ->dirty_counter)
	{
		bool was_cleaned = false;

		THINGS_FOREACH(circ, THING_Inverter)
		{
			bool cleaned = inverter_clean_up(circ, (Inverter*)it);
			was_cleaned |= cleaned;
			clean_num += cleaned;
		}

		THINGS_FOREACH(circ, THING_Chip)
		{
			bool cleaned = chip_clean_up(circ, (Chip*)it);
			was_cleaned |= cleaned;
			clean_num += cleaned;
		}

		// If nothing was cleaned, inverters might have had to tic twice, continue next
		// tic instead
		if (!was_cleaned)
			break;
	}
}

void circuit_tic(Circuit* circ)
{
	tic_num++;
	clean_num = 0;

	circuit_run_tic(circ);

	if (clean_num > 0)
	{
		log("TIC iter=%d", clean_num);
	}
}

void circuit_merge(Circuit* circ, Circuit* other)
{
	// Make sure they actually fit..
	assert((circ->thing_num) + (other->thing_num) < MAX_THINGS);

	// Copy over all the things, at the end of the target list
	memcpy(circ->things + circ->thing_num, other->things, sizeof(Thing) * (other->thing_num));

	// After that we have to update all of the connection ID's, since the indecies have been shifted
	for(u32 i=circ->thing_num; i<circ->thing_num + other->thing_num; ++i)
	{
		if (circ->things[i].type == THING_Node)
		{
			Node* node = (Node*)&circ->things[i];
			for(u32 c=0; c<4; ++c)
			{
				// We can do this for all connections, since NULL connections have 0 generation anyways
				node->connections[c].index += circ->thing_num;
			}
		}
	}

	// Merge public nodes
	memcpy(circ->public_nodes, other->public_nodes, sizeof(circ->public_nodes));

	// Avoid repeat generation
	circ->gen_num = max(circ->gen_num, other->gen_num);
	circ->thing_num += other->thing_num;
}

void circuit_copy(Circuit* circ, Circuit* other)
{
	memcpy(circ, other, sizeof(Circuit));
	THINGS_FOREACH(circ, THING_All)
	{
		if (type_data[it->type].copy_proc)
		{
			u32 index = it - circ->things;
			Thing* other_thing = &other->things[index];

			type_data[it->type].copy_proc(circ, it, other_thing);
		}
	}
}

void circuit_copy_rect(Circuit* circ, Circuit* other, Rect copy_rect)
{
	circuit_copy(circ, other);
	THINGS_FOREACH(circ, THING_All)
	{
		Rect thing_rect = rect(it->pos, point_add(it->pos, point_sub(it->size, point(1, 1))));
		if (!rect_rect_intersect(thing_rect, copy_rect))
			thing_delete(circ, it);
	}
}

void circuit_shift(Circuit* circ, Point amount)
{
	THINGS_FOREACH(circ, THING_All)
	{
		it->pos = point_add(it->pos, amount);
	}
}

#define fwrite_t(expr, file) (fwrite(&(expr), sizeof(expr), 1, file))
#define fread_t(expr, file) (fread(&(expr), sizeof(expr), 1, file))

void circuit_fwrite(Circuit* circ, FILE* file)
{
	fwrite_t(circ->name, file);
	fwrite_t(circ->gen_num, file);
	fwrite_t(circ->dirty_counter, file);

	// Write things
	fwrite_t(circ->thing_num, file);
	for(u32 i=0; i<circ->thing_num; ++i)
	{
		Thing* thing = &circ->things[i];

		fwrite(thing, sizeof(Thing), 1, file);
		if (type_data[thing->type].save_proc)
			type_data[thing->type].save_proc(circ, thing, file);
	}

	// Write public nodes
	fwrite_t(circ->public_nodes, file);
}

void circuit_fread(Circuit* circ, FILE* file)
{
	mem_zero(circ, sizeof(Circuit));

	fread_t(circ->name, file);
	fread_t(circ->gen_num, file);
	fread_t(circ->dirty_counter, file);

	// Read things
	fread_t(circ->thing_num, file);
	for(u32 i=0; i<circ->thing_num; ++i)
	{
		Thing* thing = &circ->things[i];

		fread(thing, sizeof(Thing), 1, file);
		if (type_data[thing->type].load_proc)
			type_data[thing->type].load_proc(circ, thing, file);
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

	bool was_powered = node->state & POWER_Powered;
	node->state &= ~POWER_Powered;

	bool state = node_batch_contains_power(circ, node, -1);

	if (was_powered)
		node->state |= POWER_Powered;

	return state;
}

void circuit_set_public_state(Circuit* circ, u32 index, bool state)
{
	Node* node = node_get(circ, circ->public_nodes[index]);
	if (!node)
		return;

	node_set_powered(circ, node, state);
}