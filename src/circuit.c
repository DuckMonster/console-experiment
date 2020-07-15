#include "circuit.h"
#include <stdlib.h>
#include <stdio.h>
/* CIRCUIT */
Circuit* circuit_make(const char* name)
{
	Circuit* circ = malloc(sizeof(Circuit));
	mem_zero(circ, sizeof(Circuit));

	strcpy(circ->name, name);
	return circ;
}

void circuit_clear(Circuit* circ)
{
	if (circ->things)
		free(circ->things);

	mem_zero(circ, sizeof(Circuit));
}

void circuit_free(Circuit* circ)
{
	free(circ);
}

void circuit_subtic(Circuit* circ)
{
	/*
	Dirty_Stack* tic_stack = &circ->dirty_stacks[circ->stack_index];
	if (tic_stack->count == 0)
		return;

	if (tic_stack->count != 0)
	{
		Thing* thing = dirty_stack_pop(tic_stack, circ);
		if (thing)
			thing_clean(circ, thing);
	}

	// If we emptied our stack this subtic, advance tic
	if (tic_stack->count == 0)
	{
		circ->stack_index = !circ->stack_index;
		tic++;
	}
	*/
}

void circuit_tic(Circuit* circ)
{
	/*
	Dirty_Stack* tic_stack = &circ->dirty_stacks[circ->stack_index];
	if (tic_stack->count == 0)
		return;

	while(tic_stack->count != 0)
	{
		Thing* thing = dirty_stack_pop(tic_stack, circ);
		if (thing)
			thing_clean(circ, thing);
	}

	circ->stack_index = !circ->stack_index;
	tic++;
	*/
}

void circuit_merge(Circuit* circ, Circuit* other)
{
	// Make sure they actually fit..
	things_reserve(circ, circ->thing_num + other->thing_num);

	// Copy over all the things, at the end of the target list
	memcpy(circ->things + circ->thing_num, other->things, sizeof(Thing) * (other->thing_num));

	// After that we have to update all of the connection ID's, since the indecies have been shifted
	for(u32 i=circ->thing_num; i<circ->thing_num + other->thing_num; ++i)
	{
		// Re-dirty everything
		Thing* thing = &circ->things[i];
		thing->dirty = false;
		thing_set_dirty(circ, thing);

		if (thing->type == THING_Node)
		{
			Node* node = (Node*)thing;
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

	static Thing* duplicates[4];

	// After all of this, merge all duplicates!
	THINGS_FOREACH(circ, THING_All)
	{
		u32 dup_num = things_find(circ, thing_get_bbox(it), duplicates, 4);

		// Only one thing found, it is the original thing!
		if (dup_num == 1)
			continue;

		for(u32 i=0; i<dup_num; ++i)
		{
			// This IS the thing we're merging...
			if (duplicates[i] == it)
				continue;

			// Only merge stuff of the same type..
			if (it->type == duplicates[i]->type)
			{
				Thing_Type_Data* type = thing_type_data(it);
				if (type->on_merge)
					type->on_merge(circ, it, duplicates[i]);
			}

			thing_delete(circ, duplicates[i]);
		}
	}
}

void circuit_copy(Circuit* circ, Circuit* other)
{
	memcpy(circ, other, sizeof(Circuit));
	circ->things = malloc(sizeof(Thing) * other->thing_max);
	memcpy(circ->things, other->things, sizeof(Thing) * other->thing_max);

	THINGS_FOREACH(circ, THING_All)
	{
		it->dirty = false;
		it->tic = 0;
		Thing_Type_Data* type = thing_type_data(it);
		if (type->on_copy)
		{
			u32 index = it - circ->things;
			Thing* other_thing = &other->things[index];

			type->on_copy(circ, it, other_thing);
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

	// Write things
	fwrite_t(circ->thing_max, file);
	fwrite_t(circ->thing_num, file);
	for(u32 i=0; i<circ->thing_num; ++i)
	{
		Thing* thing = &circ->things[i];

		fwrite(thing, sizeof(Thing), 1, file);
		Thing_Type_Data* type = thing_type_data(thing);
		if (type->on_save)
			type->on_save(circ, thing, file);
	}

	// Write public nodes
	fwrite_t(circ->public_nodes, file);
}

void circuit_fread(Circuit* circ, FILE* file)
{
	circuit_clear(circ);

	fread_t(circ->name, file);
	fread_t(circ->gen_num, file);

	// Read things
	fread_t(circ->thing_max, file);
	circ->things = malloc(sizeof(Thing) * circ->thing_max);
	mem_zero(circ->things, sizeof(Thing) * circ->thing_max);

	fread_t(circ->thing_num, file);
	for(u32 i=0; i<circ->thing_num; ++i)
	{
		Thing* thing = &circ->things[i];

		fread(thing, sizeof(Thing), 1, file);
		Thing_Type_Data* type = thing_type_data(thing);
		if (type->on_load)
			type->on_load(circ, thing, file);
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
	fclose(file);
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