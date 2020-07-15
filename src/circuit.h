#pragma once
#include "tic.h"
#include <stdio.h>

#define MAX_PUBLIC_NODES 32

typedef struct Thing Thing;

/* CIRCUIT */
typedef struct Circuit
{
	char name[20];
	u16 gen_num;

	Thing* things;
	u32 thing_max;
	u32 thing_num;

	Thing_Id public_nodes[MAX_PUBLIC_NODES];
	Circuit* parent;
} Circuit;

Circuit* circuit_make(const char* name);
void circuit_clear(Circuit* circ);
void circuit_free(Circuit* circ);

void circuit_subtic(Circuit* circ);
void circuit_tic(Circuit* circ);

void circuit_merge(Circuit* circ, Circuit* other);
void circuit_copy(Circuit* circ, Circuit* other);
void circuit_copy_rect(Circuit* circ, Circuit* other, Rect copy_rect);
void circuit_shift(Circuit* circ, Point amount);

void circuit_save(Circuit* circ, const char* path);
void circuit_load(Circuit* circ, const char* path);

