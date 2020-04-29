#include "circuit.h"
#include <stdlib.h>

Circuit* circuit_make(const char* name)
{
	Circuit* circ = malloc(sizeof(Circuit));
	mem_zero(circ, sizeof(Circuit));

	circ->name = name;
	return circ;
}

void circuit_free(Circuit* circ)
{

}