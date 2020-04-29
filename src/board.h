#pragma once
#include "circuit.h"

#define KEY_CANCEL 0x01
#define KEY_PLACE_NODE 0x11
#define KEY_PLACE_INVERTER 0x17
#define KEY_PLACE_COMMENT 0x2E
#define KEY_PLACE_CHIP 0x19
#define KEY_TOGGLE_LINK 0x18
#define KEY_DELETE 0x2D
#define KEY_MOVE_LEFT 0x23
#define KEY_MOVE_DOWN 0x24
#define KEY_MOVE_UP 0x25
#define KEY_MOVE_RIGHT 0x26

#define KEY_EDIT_STEP_IN 0x4E
#define KEY_EDIT_STEP_OUT 0x4A

#define KEY_VISUAL_MODE 0x2F
#define KEY_TIC 0x34
#define EDIT_STACK_SIZE 8

/* BOARD */
typedef struct
{
	bool visual;
	i32 vis_x;
	i32 vis_y;

	i32 cursor_x;
	i32 cursor_y;

	Circuit* edit_stack[EDIT_STACK_SIZE];
	i32 edit_index;
} Board;
extern Board board;


void board_init();
void board_tic();
void board_draw();

bool board_key_event(u32 code, char chr);
Circuit* board_get_edit_circuit();