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

#include "core/string/string_name.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"

// ============================================================================
// IR Opcodes
// ============================================================================

enum class GDScript2IROp {
	// Special
	OP_NOP, // No operation
	OP_COMMENT, // Debug comment (ignored in codegen)

	// Constants & Literals
	OP_LOAD_CONST, // dest = constant_value
	OP_LOAD_NIL, // dest = null

	// Variables - Local
	OP_LOAD_LOCAL, // dest = locals[index]
	OP_STORE_LOCAL, // locals[index] = src

	// Variables - Member (instance variables)
	OP_LOAD_MEMBER, // dest = self.name
	OP_STORE_MEMBER, // self.name = src

	// Variables - Global/Class
	OP_LOAD_GLOBAL, // dest = globals[name]
	OP_STORE_GLOBAL, // globals[name] = src

	// Self reference
	OP_LOAD_SELF, // dest = self

	// Arithmetic operations
	OP_ADD, // dest = left + right
	OP_SUB, // dest = left - right
	OP_MUL, // dest = left * right
	OP_DIV, // dest = left / right
	OP_MOD, // dest = left % right
	OP_POW, // dest = left ** right
	OP_NEG, // dest = -operand
	OP_POS, // dest = +operand

	// Bitwise operations
	OP_BIT_AND, // dest = left & right
	OP_BIT_OR, // dest = left | right
	OP_BIT_XOR, // dest = left ^ right
	OP_BIT_NOT, // dest = ~operand
	OP_BIT_LSH, // dest = left << right
	OP_BIT_RSH, // dest = left >> right

	// Comparison operations
	OP_EQ, // dest = left == right
	OP_NE, // dest = left != right
	OP_LT, // dest = left < right
	OP_LE, // dest = left <= right
	OP_GT, // dest = left > right
	OP_GE, // dest = left >= right

	// Logical operations
	OP_NOT, // dest = not operand
	OP_AND, // dest = left and right (short-circuit)
	OP_OR, // dest = left or right (short-circuit)

	// Type operations
	OP_IS, // dest = operand is type
	OP_AS, // dest = operand as type (cast)
	OP_TYPEOF, // dest = typeof(operand)
	OP_IN, // dest = left in right

	// Container operations
	OP_CONSTRUCT_ARRAY, // dest = [args...]
	OP_CONSTRUCT_DICT, // dest = {key:value, ...}
	OP_GET_INDEX, // dest = base[index]
	OP_SET_INDEX, // base[index] = value
	OP_GET_NAMED, // dest = base.name
	OP_SET_NAMED, // base.name = value

	// Control flow
	OP_JUMP, // goto target_block
	OP_JUMP_IF, // if (cond) goto true_block else goto false_block
	OP_JUMP_IF_NOT, // if (!cond) goto target_block

	// Function calls
	OP_CALL, // dest = callee(args...)
	OP_CALL_METHOD, // dest = base.method(args...)
	OP_CALL_BUILTIN, // dest = builtin_func(args...)
	OP_CALL_SUPER, // dest = super.method(args...)
	OP_CALL_SELF, // dest = self.method(args...)

	// Return
	OP_RETURN, // return value
	OP_RETURN_VOID, // return (void)

	// Iterators (for loops)
	OP_ITER_BEGIN, // dest = iter_begin(container)
	OP_ITER_NEXT, // dest = iter_next(iterator), sets flag
	OP_ITER_GET, // dest = iter_get(iterator)

	// Special constructs
	OP_AWAIT, // dest = await signal/coroutine
	OP_YIELD, // yield from coroutine

	// Signal operations
	OP_SIGNAL_DEFINE, // Define a signal: signal_name
	OP_SIGNAL_CONNECT, // Connect signal: dest = signal.connect(callable, flags)
	OP_SIGNAL_DISCONNECT, // Disconnect signal: signal.disconnect(callable)
	OP_SIGNAL_EMIT, // Emit signal: signal.emit(args...)
	OP_SIGNAL_IS_CONNECTED, // dest = signal.is_connected(callable)
	OP_MAKE_SIGNAL, // dest = Signal(object, signal_name)

	OP_PRELOAD, // dest = preload(path)
	OP_GET_NODE, // dest = get_node(path) ($path)
	OP_LAMBDA, // dest = lambda_id (captures)

	// SSA operations
	OP_PHI, // dest = phi(block1:val1, block2:val2, ...)
	OP_COPY, // dest = src (for SSA destruction)

	// Assertions & Debug
	OP_ASSERT, // assert condition, message
	OP_BREAKPOINT, // debugger breakpoint
};

// Get opcode name for debugging
const char *gdscript2_ir_op_name(GDScript2IROp p_op);

// ============================================================================
// IR Operand
// ============================================================================

// Operand types for IR instructions
enum class GDScript2IROperandType {
	NONE,
	REGISTER, // Virtual register (SSA value)
	CONSTANT, // Constant pool index
	IMMEDIATE, // Inline constant value
	BLOCK, // Basic block reference
	NAME, // StringName (for member access, etc.)
	LOCAL_VAR, // Local variable slot index
	FUNC_REF, // Function reference
};

struct GDScript2IROperand {
	GDScript2IROperandType type = GDScript2IROperandType::NONE;

	union {
		int reg_id; // For REGISTER
		int const_idx; // For CONSTANT
		int block_id; // For BLOCK
		int local_idx; // For LOCAL_VAR
		int func_idx; // For FUNC_REF
	};

	Variant imm_value; // For IMMEDIATE
	StringName name; // For NAME

	GDScript2IROperand() :
			reg_id(-1) {}

	static GDScript2IROperand make_none() { return GDScript2IROperand(); }

	static GDScript2IROperand make_reg(int p_id) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::REGISTER;
		op.reg_id = p_id;
		return op;
	}

	static GDScript2IROperand make_const(int p_idx) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::CONSTANT;
		op.const_idx = p_idx;
		return op;
	}

	static GDScript2IROperand make_imm(const Variant &p_value) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::IMMEDIATE;
		op.imm_value = p_value;
		return op;
	}

	static GDScript2IROperand make_block(int p_id) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::BLOCK;
		op.block_id = p_id;
		return op;
	}

	static GDScript2IROperand make_name(const StringName &p_name) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::NAME;
		op.name = p_name;
		return op;
	}

	static GDScript2IROperand make_local(int p_idx) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::LOCAL_VAR;
		op.local_idx = p_idx;
		return op;
	}

	static GDScript2IROperand make_func(int p_idx) {
		GDScript2IROperand op;
		op.type = GDScript2IROperandType::FUNC_REF;
		op.func_idx = p_idx;
		return op;
	}

	bool is_valid() const { return type != GDScript2IROperandType::NONE; }
	bool is_reg() const { return type == GDScript2IROperandType::REGISTER; }
	bool is_const() const { return type == GDScript2IROperandType::CONSTANT || type == GDScript2IROperandType::IMMEDIATE; }

	String to_string() const;
};

// ============================================================================
// IR Instruction
// ============================================================================

struct GDScript2IRInstr {
	GDScript2IROp op = GDScript2IROp::OP_NOP;

	// Destination register (for instructions that produce a value)
	GDScript2IROperand dest;

	// Source operands
	LocalVector<GDScript2IROperand> args;

	// For jumps: target blocks
	int target_block = -1; // Primary target (for JUMP, JUMP_IF true branch)
	int else_block = -1; // Else target (for JUMP_IF false branch)

	// Source location for debugging
	int source_line = 0;
	int source_column = 0;

	// Flags
	bool is_dead = false; // Marked for removal by DCE

	GDScript2IRInstr() = default;
	explicit GDScript2IRInstr(GDScript2IROp p_op) :
			op(p_op) {}

	// Helper constructors
	static GDScript2IRInstr make_nop();
	static GDScript2IRInstr make_load_const(GDScript2IROperand p_dest, const Variant &p_value);
	static GDScript2IRInstr make_load_local(GDScript2IROperand p_dest, int p_local_idx);
	static GDScript2IRInstr make_store_local(int p_local_idx, GDScript2IROperand p_src);
	static GDScript2IRInstr make_binary(GDScript2IROp p_op, GDScript2IROperand p_dest, GDScript2IROperand p_left, GDScript2IROperand p_right);
	static GDScript2IRInstr make_unary(GDScript2IROp p_op, GDScript2IROperand p_dest, GDScript2IROperand p_operand);
	static GDScript2IRInstr make_jump(int p_target);
	static GDScript2IRInstr make_jump_if(GDScript2IROperand p_cond, int p_true_block, int p_false_block);
	static GDScript2IRInstr make_return(GDScript2IROperand p_value);
	static GDScript2IRInstr make_return_void();
	static GDScript2IRInstr make_call(GDScript2IROperand p_dest, GDScript2IROperand p_callee, const LocalVector<GDScript2IROperand> &p_args);

	String to_string() const;
};

// ============================================================================
// Basic Block
// ============================================================================

struct GDScript2IRBlock {
	int id = -1;
	StringName label; // Optional label for debugging

	LocalVector<GDScript2IRInstr> instructions;

	// Control flow graph edges
	LocalVector<int> successors;
	LocalVector<int> predecessors;

	// For loop analysis
	bool is_loop_header = false;
	int loop_depth = 0;

	// Flags
	bool is_reachable = true;

	GDScript2IRBlock() = default;
	explicit GDScript2IRBlock(int p_id) :
			id(p_id) {}

	void add_successor(int p_block_id);
	void add_predecessor(int p_block_id);

	// Check if block ends with a terminator (jump, return)
	bool has_terminator() const;

	// Get the terminator instruction (or nullptr)
	GDScript2IRInstr *get_terminator();
	const GDScript2IRInstr *get_terminator() const;

	String to_string() const;
};

// ============================================================================
// Local Variable Info
// ============================================================================

struct GDScript2IRLocal {
	StringName name;
	int index = -1;
	bool is_parameter = false;
	int parameter_index = -1; // For parameters: position in parameter list

	GDScript2IRLocal() = default;
	GDScript2IRLocal(const StringName &p_name, int p_index) :
			name(p_name), index(p_index) {}
};

// ============================================================================
// IR Function
// ============================================================================

struct GDScript2IRFunction {
	StringName name;
	bool is_static = false;
	bool is_coroutine = false;

	// Basic blocks (block 0 is always entry)
	LocalVector<GDScript2IRBlock> blocks;
	int entry_block = 0;

	// Local variables (including parameters)
	LocalVector<GDScript2IRLocal> locals;
	HashMap<StringName, int> local_map; // name -> local index
	int parameter_count = 0;

	// Constant pool (shared within function)
	LocalVector<Variant> constants;
	HashMap<Variant, int> constant_map;

	// Register allocation
	int next_reg = 0;

	// Source info
	int source_line = 0;

	GDScript2IRFunction() = default;
	explicit GDScript2IRFunction(const StringName &p_name) :
			name(p_name) {}

	// Block management
	int create_block();
	GDScript2IRBlock *get_block(int p_id);
	const GDScript2IRBlock *get_block(int p_id) const;

	// Register allocation
	int alloc_reg() { return next_reg++; }
	GDScript2IROperand alloc_temp() { return GDScript2IROperand::make_reg(alloc_reg()); }

	// Local variable management
	int add_local(const StringName &p_name, bool p_is_param = false);
	int get_local_index(const StringName &p_name) const;
	bool has_local(const StringName &p_name) const;

	// Constant pool management
	int add_constant(const Variant &p_value);
	const Variant &get_constant(int p_index) const;

	// CFG utilities
	void compute_predecessors();
	void compute_reachability();

	String to_string() const;
};

// ============================================================================
// IR Module
// ============================================================================

struct GDScript2IRModule {
	StringName name; // Script/class name

	// Functions
	LocalVector<GDScript2IRFunction> functions;
	HashMap<StringName, int> function_map; // name -> function index

	// Global variables
	LocalVector<StringName> globals;
	HashMap<StringName, int> global_map;

	// Module-level constants
	LocalVector<Variant> constants;

	GDScript2IRModule() = default;

	// Function management
	int add_function(const StringName &p_name);
	GDScript2IRFunction *get_function(int p_index);
	GDScript2IRFunction *get_function(const StringName &p_name);
	const GDScript2IRFunction *get_function(int p_index) const;
	int get_function_index(const StringName &p_name) const;
	bool has_function(const StringName &p_name) const;

	// Global variable management
	int add_global(const StringName &p_name);
	int get_global_index(const StringName &p_name) const;
	bool has_global(const StringName &p_name) const;

	String to_string() const;
};
