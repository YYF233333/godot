/**************************************************************************/
/*  gdscript2_ir.h                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"

// High-level IR opcodes (abstract from bytecode).
enum class GDScript2IROp {
	NOP,
	// Constants / Literals
	LOAD_CONST,
	// Variables
	LOAD_LOCAL,
	STORE_LOCAL,
	LOAD_GLOBAL,
	STORE_GLOBAL,
	// Arithmetic
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	NEG,
	// Comparison
	EQ,
	NE,
	LT,
	LE,
	GT,
	GE,
	// Logic
	NOT,
	AND,
	OR,
	// Control flow
	JUMP,
	JUMP_IF,
	JUMP_IF_NOT,
	// Calls
	CALL,
	CALL_BUILTIN,
	RETURN,
	// Misc
	PHI, // SSA phi node placeholder
};

struct GDScript2IRInstr {
	GDScript2IROp op = GDScript2IROp::NOP;
	int dest = -1; // Destination register/slot (SSA value id).
	Vector<int> args; // Source operands (register ids or const indices).
	Variant const_value; // For LOAD_CONST.
	StringName name; // For LOAD_GLOBAL, CALL, etc.
	int jump_target = -1; // For jumps.

	GDScript2IRInstr() = default;
	GDScript2IRInstr(GDScript2IROp p_op) :
			op(p_op) {}
};

// A basic block in the IR.
struct GDScript2IRBlock {
	int id = -1;
	Vector<GDScript2IRInstr> instructions;
	Vector<int> successors; // Block ids.
	Vector<int> predecessors;
};

// Function-level IR.
struct GDScript2IRFunction {
	StringName name;
	Vector<GDScript2IRBlock> blocks;
	int entry_block = 0;
	int next_reg = 0; // For SSA value numbering.

	int alloc_reg() { return next_reg++; }
};

// Module-level IR (collection of functions).
struct GDScript2IRModule {
	Vector<GDScript2IRFunction> functions;
};
