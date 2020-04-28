#include "circuit.h"
#include <stdlib.h>

u8 get_direction(i32 x1, i32 y1, i32 x2, i32 y2)
{
	i32 dx = x2 - x1;
	i32 dy = y2 - y1;

	if (dy == 0)
	{
		if (dx > 0) return DIR_East;
		if (dx < 0) return DIR_West;
	}
	else
	{
		if (dy > 0) return DIR_South;
		if (dy < 0) return DIR_North;
	}

	return DIR_None;
}

/* NODES */
Node* node_get(Circuit* circ, i32 x, i32 y)
{
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Node* node = &circ->nodes[i];
		if (node->valid && node->x == x && node->y == y)
			return node;
	}

	return NULL;
}

Node* node_place(Circuit* circ, i32 x, i32 y)
{
	Node* node = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (circ->nodes[i].valid)
			continue;

		node = &circ->nodes[i];
	}

	assert(node != NULL);
	node->valid = true;
	node->x = x;
	node->y = y;

	// Should we intercept and split any connections?
	Connection conn = connection_get(circ, x, y);
	if (conn.first)
	{
		node_remove_connection(conn.first, conn.second);
		node_remove_connection(conn.second, conn.first);
		node_connect(conn.first, node);
		node_connect(conn.second, node);
	}

	return node;
}

void node_delete(Node* node)
{
	// Remove the connection to other circ->nodes
	for(u32 i=0; i<node->num_connections; ++i)
		node_remove_connection(node->connections[i], node);

	mem_zero(node, sizeof(Node));
}

void node_connect(Node* a, Node* b)
{
	bool a_con_b = false;
	bool b_con_a = false;

	// Check if they're already connected...
	for(u32 i=0; i<a->num_connections; ++i)
	{
		if (a->connections[i] == b)
		{
			a_con_b = true;
			break;
		}
	}
	for(u32 i=0; i<b->num_connections; ++i)
	{
		if (b->connections[i] == a)
		{
			b_con_a = true;
			break;
		}
	}

	assert(a_con_b == b_con_a);

	if (!a_con_b)
	{
		a->connections[a->num_connections] = b,
		a->num_connections++;
		assert(a->num_connections <= 4);
	}

	if (!b_con_a)
	{
		b->connections[b->num_connections] = a;
		b->num_connections++;
		assert(b->num_connections <= 4);
	}
}

void node_remove_connection(Node* node, Node* other)
{
	bool found = false;
	for(u32 i=0; i<node->num_connections; ++i)
	{
		// If we found the entry, move everything backwards
		if (found)
		{
			node->connections[i - 1] = node->connections[i];
		}
		// otherwise, look until we find the entry
		else
		{
			if (node->connections[i] == other)
				found = true;
		}
	}

	if (found)
		node->num_connections--;
}

void node_activate(Node* node)
{
	node->state = true;
	node->tic = tic_num;

	for(u32 i=0; i<node->num_connections; ++i)
	{
		Node* other = node->connections[i];
		if (other->tic != tic_num)
			node_activate(other);
	}
}

Connection connection_get(Circuit* circ, i32 x, i32 y)
{
	Connection conn;
	mem_zero(&conn, sizeof(conn));

	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Node* first = &circ->nodes[i];
		if (!first->valid)
			continue;

		for(u32 c=0; c<first->num_connections; ++c)
		{
			Node* second = first->connections[c];
			if (first->x == second->x && first->x == x)
			{
				i32 min_y = min(first->y, second->y);
				i32 max_y = max(first->y, second->y);
				if (min_y < y && max_y > y)
				{
					conn.first = first;
					conn.second = second;
					break;
				}
			}
			else if (first->y == y)
			{
				i32 min_x = min(first->x, second->x);
				i32 max_x = max(first->x, second->x);
				if (min_x < x && max_x > x)
				{
					conn.first = first;
					conn.second = second;
					break;
				}
			}

		}
	}

	return conn;
}

/* INVERTERS */
Inverter* inverter_get(Circuit* circ, i32 x, i32 y)
{
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (circ->inverters[i].valid && circ->inverters[i].x == x && circ->inverters[i].y == y)
			return &circ->inverters[i];
	}

	return NULL;
}

Inverter* inverter_place(Circuit* circ, i32 x, i32 y)
{
	Inverter* inverter = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (!circ->inverters[i].valid)
		{
			inverter = &circ->inverters[i];
			break;
		}
	}

	assert(inverter != NULL);
	inverter->valid = true;
	inverter->x = x;
	inverter->y = y;

	// If we're placing on a connection, intercept the connection by placing two new circ->nodes
	Connection conn = connection_get(circ, x, y);
	if (conn.first)
	{
		Node* left;
		Node* right;

		left = node_get(circ, x - 1, y);
		if (!left)
			left = node_place(circ, x - 1, y);

		right = node_get(circ, x + 1, y);
		if (!right)
			right = node_place(circ, x + 1, y);

		node_remove_connection(left, right);
		node_remove_connection(right, left);
	}

	return inverter;
}

void inverter_delete(Inverter* inv)
{
	mem_zero(inv, sizeof(Inverter));
}

/* COMMENTS */
Comment* comment_place(Circuit* circ, i32 x, i32 y)
{
	Comment* comment = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (!circ->comments[i].valid)
		{
			comment = &circ->comments[i];
			break;
		}
	}

	assert(comment);
	comment->valid = true;
	comment->x = x;
	comment->y = y;
	return comment;
}

/* CHIPS */
Chip* chip_get(Circuit* circ, i32 x, i32 y)
{
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		Chip* chip = &circ->chips[i];
		if (!chip->valid)
			continue;

		i32 dx = x - chip->x;
		i32 dy = y - chip->y;
		if (dx < 0 || dx >= chip->width || dy < 0 || dy >= chip->height)
			continue;

		return chip;
	}

	return NULL;
}

Chip* chip_place(Circuit* circ, i32 x, i32 y)
{
	Chip* chip = NULL;
	for(u32 i=0; i<MAX_THINGS; ++i)
	{
		if (!circ->chips[i].valid)
		{
			chip = &circ->chips[i];
			break;
		}
	}

	assert(chip);
	chip->valid = true;
	chip->x = x;
	chip->y = y;
	chip->width = 5;
	chip->height = 5;

	chip->link_circuit = circuit_make("CHIP");
	return chip;
}

/* THINGS */
Thing thing_get(Circuit* circ, i32 x, i32 y)
{
	Thing thing;
	mem_zero(&thing, sizeof(thing));

	// Nodes
	Node* node = node_get(circ, x, y);
	if (node)
	{
		thing.type = THING_Node;
		thing.ptr = node;
		return thing;
	}

	// Inverters
	Inverter* inv = inverter_get(circ, x, y);
	if (inv)
	{
		thing.type = THING_Inverter;
		thing.ptr = inv;
		return thing;
	}

	return thing;
}

u32 things_get(Circuit* circ, i32 x1, i32 y1, i32 x2, i32 y2, Thing* out_things, u32 arr_size)
{
	i32 min_x = min(x1, x2);
	i32 max_x = max(x1, x2);
	i32 min_y = min(y1, y2);
	i32 max_y = max(y1, y2);

	u32 index = 0;
	for(i32 y=min_y; y<=max_y; ++y)
	{
		for(i32 x=min_x; x<=max_x; ++x)
		{
			Thing thing = thing_get(circ, x, y);
			if (thing.type == THING_NULL)
				continue;

			out_things[index++] = thing;
			if (index >= arr_size)
				break;
		}
	}

	return index;
}

Circuit* circuit_make(const char* name)
{
	Circuit* circ = (Circuit*)malloc(sizeof(Circuit));
	mem_zero(circ, sizeof(Circuit));

	circ->name = name;
	return circ;
}

void circuit_free(Circuit* circ)
{
	free(circ);
}