#pragma once
#include "thing.h"
extern u32 tic;

#define DIRTY_STACK_SIZE 128

// Dirty entry
typedef struct
{
	Thing* thing;
	Circuit* circ;
} Dirty_Entry;

// Dirty stack
typedef struct
{
	Dirty_Entry list[DIRTY_STACK_SIZE];
	u32 count;
} Dirty_Stack;

void dirty_stack_push(Dirty_Stack* stack, Circuit* circ, Thing* thing);
Dirty_Entry dirty_stack_pop(Dirty_Stack* stack);
Dirty_Entry dirty_stack_peek(Dirty_Stack* stack);

void thing_set_dirty(Circuit* circ, Thing* thing);
void thing_dirty_at(Circuit* circ, Point pos);
void thing_clean(Circuit* circ, Thing* thing);
