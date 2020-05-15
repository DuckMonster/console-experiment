#pragma once
#include "circuit.h"

#define KEY_CANCEL 0x01
#define KEY_PLACE_NODE 0x11
#define KEY_PLACE_INVERTER 0x17
#define KEY_PLACE_COMMENT 0x2E
#define KEY_PLACE_CHIP 0x2E
#define KEY_PLACE_DELAY 0x20

#define KEY_TOGGLE_PUBLIC 0x19

#define KEY_DELETE 0x2D
#define KEY_MOVE_LEFT 0x23
#define KEY_MOVE_DOWN 0x24
#define KEY_MOVE_UP 0x25
#define KEY_MOVE_RIGHT 0x26

#define KEY_YANK 0x15
#define KEY_PUT 0x19

#define KEY_SAVE 0x1F
#define KEY_LOAD 0x18

#define KEY_VISUAL_MODE 0x2F
#define KEY_TIC 0x34
#define KEY_SUBTIC 0x33

#define KEY_PROMPT 0x20
#define EDIT_STACK_SIZE 8

/* BOARD */
typedef struct
{
	bool visual;

	Point offset;
	Point vis_origin;
	Point cursor;

	Circuit* edit_stack[EDIT_STACK_SIZE];
	i32 edit_index;

	bool debug;
} Board;
extern Board board;

void board_init();
void board_tic();
void board_draw();

bool board_key_event(u32 code, char chr, u32 mods);
Circuit* board_get_edit_circuit();