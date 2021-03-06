#include "tic.h"
#include "circuit.h"

// Dirty stack
u32 tic = 1;
u32 tic_push_count = 0;
u32 tic_pop_count = 0;
void dirty_stack_push(Dirty_Stack* stack, Circuit* circ, Thing* thing)
{
	if (thing->dirty)
		return;

	assert(stack->count < DIRTY_STACK_SIZE);
	Dirty_Entry* entry = NULL;

	Thing_Type_Data* type = thing_type_data(thing);
	u8 push_type = type->push_type;
	if (push_type == PUSH_Top)
	{
		entry = &stack->list[stack->count++];
	}
	else
	{
		// Move every entry in the stack upwards
		memmove(stack->list + 1, stack->list, sizeof(Thing_Id) * stack->count);
		stack->count++;

		entry = &stack->list[0];
	}

	entry->thing = thing;
	entry->circ = circ;

	tic_push_count++;
	thing->dirty = true;
}

Dirty_Entry dirty_stack_pop(Dirty_Stack* stack)
{
	assert(stack->count > 0);

	// Iterate through the stack to find a valid (if stuff was deleted mid-tic)
	Dirty_Entry entry;
	entry = stack->list[--stack->count];
	entry.thing->dirty = false;
	tic_pop_count++;

	return entry;
}

Dirty_Entry dirty_stack_peek(Dirty_Stack* stack)
{
	Dirty_Entry entry;
	zero_t(entry);

	if (stack->count == 0)
		return entry;

	return stack->list[stack->count - 1];
}

void thing_set_dirty(Circuit* circ, Thing* thing)
{
	/*
	Dirty_Stack* stack = &circ->dirty_stacks[circ->stack_index];

	// If this thing ticed this frame, push onto the next stack
	if (thing->tic == tic)
		stack = &circ->dirty_stacks[!circ->stack_index];

	dirty_stack_push(stack, circ, thing);

	Thing_Type_Data* type = thing_type_data(thing);
	if (type->on_dirty)
		type->on_dirty(circ, thing);
		*/
}

void thing_dirty_at(Circuit* circ, Point pos)
{
	/*
	Thing* thing = thing_find(circ, pos, THING_All);
	if (!thing)
		return;

	thing_set_dirty(circ, thing);
	*/
}

void thing_clean(Circuit* circ, Thing* thing)
{
	/*
	thing->tic = tic;
	Thing_Type_Data* type = thing_type_data(thing);
	if (type->on_clean)
		type->on_clean(circ, thing);
		*/
}