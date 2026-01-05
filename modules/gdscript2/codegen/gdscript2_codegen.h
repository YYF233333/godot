/**************************************************************************/
/*  gdscript2_codegen.h                                                   */
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

#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#include "modules/gdscript2/ir/gdscript2_ir.h"

// ============================================================================
// Bytecode Opcodes
// ============================================================================

enum class GDScript2Opcode : uint32_t {
	// Special
	OP_NOP, // No operation
	OP_END, // End of bytecode (implicit return nil)

	// Constants & Literals
	OP_LOAD_CONST, // dest = constants[idx]
	OP_LOAD_NIL, // dest = null
	OP_LOAD_TRUE, // dest = true
	OP_LOAD_FALSE, // dest = false
	OP_LOAD_SMALL_INT, // dest = immediate int value (-128..127)

	// Variables - Local
	OP_LOAD_LOCAL, // dest = locals[idx]
	OP_STORE_LOCAL, // locals[idx] = src

	// Variables - Member (instance variables)
	OP_LOAD_MEMBER, // dest = self.name
	OP_STORE_MEMBER, // self.name = src

	// Variables - Global/Class
	OP_LOAD_GLOBAL, // dest = globals[name_idx]
	OP_STORE_GLOBAL, // globals[name_idx] = src

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

	// Type operations
	OP_IS, // dest = operand is type
	OP_AS, // dest = operand as type (cast)
	OP_TYPEOF, // dest = typeof(operand)
	OP_IN, // dest = left in right

	// Container operations
	OP_CONSTRUCT_ARRAY, // dest = [args...] (followed by arg count and regs)
	OP_CONSTRUCT_DICT, // dest = {k:v, ...} (followed by pair count and regs)
	OP_GET_INDEX, // dest = base[index]
	OP_SET_INDEX, // base[index] = value
	OP_GET_NAMED, // dest = base.name
	OP_SET_NAMED, // base.name = value

	// Control flow
	OP_JUMP, // goto addr
	OP_JUMP_IF, // if (cond) goto addr
	OP_JUMP_IF_NOT, // if (!cond) goto addr

	// Function calls
	OP_CALL, // dest = func(args...) - general call
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
	OP_JUMP_IF_ITER_END, // if (iter ended) goto addr

	// Special constructs
	OP_AWAIT, // dest = await signal/coroutine
	OP_YIELD, // yield from coroutine

	// Signal operations
	OP_SIGNAL_DEFINE, // Define signal: name_idx
	OP_SIGNAL_CONNECT, // dest = signal_reg.connect(callable_reg, flags)
	OP_SIGNAL_DISCONNECT, // signal_reg.disconnect(callable_reg)
	OP_SIGNAL_EMIT, // signal_reg.emit(arg1, arg2, ...)
	OP_SIGNAL_IS_CONNECTED, // dest = signal_reg.is_connected(callable_reg)
	OP_MAKE_SIGNAL, // dest = Signal(object_reg, name_idx)

	OP_PRELOAD, // dest = preload(path)
	OP_GET_NODE, // dest = get_node(path) ($path)
	OP_LAMBDA, // dest = create lambda with captures

	// Copy/Move
	OP_COPY, // dest = src (register copy)

	// Assertions & Debug
	OP_ASSERT, // assert condition, message
	OP_BREAKPOINT, // debugger breakpoint
	OP_LINE, // debug: line number marker

	// Total count
	OP_MAX
};

// Get opcode name for debugging/disassembly
const char *gdscript2_opcode_name(GDScript2Opcode p_op);

// ============================================================================
// Bytecode Instruction
// ============================================================================

// Variable-length bytecode instruction
struct GDScript2BytecodeInstr {
	GDScript2Opcode op = GDScript2Opcode::OP_NOP;
	LocalVector<int32_t> operands; // Variable number of operands

	// Source location for debugging
	int32_t line = 0;

	GDScript2BytecodeInstr() = default;
	explicit GDScript2BytecodeInstr(GDScript2Opcode p_op) :
			op(p_op) {}

	void add_operand(int32_t p_value) { operands.push_back(p_value); }

	String to_string() const;
};

// ============================================================================
// Debug Info
// ============================================================================

struct GDScript2LineInfo {
	int32_t bytecode_offset = 0;
	int32_t source_line = 0;
};

// ============================================================================
// Compiled Function
// ============================================================================

struct GDScript2CompiledFunction {
	StringName name;
	bool is_static = false;
	bool is_coroutine = false;

	// Bytecode
	Vector<GDScript2BytecodeInstr> code;

	// Constant pool
	Vector<Variant> constants;

	// Name pool (for member/method names)
	Vector<StringName> names;

	// Stack info
	int32_t local_count = 0;
	int32_t param_count = 0;
	int32_t temp_count = 0; // Temporary registers needed
	int32_t max_stack = 0; // Maximum stack depth

	// Debug info
	Vector<GDScript2LineInfo> line_info;
	int32_t source_start_line = 0;

	// For disassembly
	String to_string() const;
};

// ============================================================================
// Compiled Module
// ============================================================================

struct GDScript2CompiledModule {
	StringName name;

	// Functions
	Vector<GDScript2CompiledFunction> functions;
	HashMap<StringName, int> function_map;

	// Global variables
	Vector<StringName> globals;

	// Module-level constants
	Vector<Variant> constants;

	// For disassembly
	String to_string() const;
};

// ============================================================================
// Code Generator
// ============================================================================

class GDScript2CodeGenerator : public RefCounted {
	GDCLASS(GDScript2CodeGenerator, RefCounted);

	// Current state
	GDScript2CompiledModule module;
	GDScript2CompiledFunction *current_func = nullptr;
	const GDScript2IRFunction *current_ir_func = nullptr;

	// Block address mapping (block_id -> bytecode offset)
	HashMap<int, int32_t> block_addresses;

	// Pending jump patches (bytecode offset -> block_id to patch)
	struct JumpPatch {
		int32_t bytecode_idx; // Index in code array
		int32_t operand_idx; // Index in operands array
		int block_id; // Target block ID
	};
	LocalVector<JumpPatch> pending_jumps;

	// Constant pool management
	HashMap<Variant, int, VariantHasher, VariantComparator> constant_map;

	// Name pool management
	HashMap<StringName, int> name_map;

	// Helper methods
	int emit(const GDScript2BytecodeInstr &p_instr);
	int emit_op(GDScript2Opcode p_op);
	int emit_op(GDScript2Opcode p_op, int32_t p_arg0);
	int emit_op(GDScript2Opcode p_op, int32_t p_arg0, int32_t p_arg1);
	int emit_op(GDScript2Opcode p_op, int32_t p_arg0, int32_t p_arg1, int32_t p_arg2);

	int add_constant(const Variant &p_value);
	int add_name(const StringName &p_name);

	// Operand conversion
	int32_t operand_to_reg(const GDScript2IROperand &p_operand);
	int32_t emit_load_operand(const GDScript2IROperand &p_operand, int32_t p_temp_reg);

	// IR to bytecode conversion
	void generate_function(const GDScript2IRFunction &p_ir_func);
	void generate_block(const GDScript2IRBlock &p_block);
	void generate_instruction(const GDScript2IRInstr &p_instr);

	// Specific instruction generators
	void gen_load_const(const GDScript2IRInstr &p_instr);
	void gen_load_nil(const GDScript2IRInstr &p_instr);
	void gen_load_local(const GDScript2IRInstr &p_instr);
	void gen_store_local(const GDScript2IRInstr &p_instr);
	void gen_load_member(const GDScript2IRInstr &p_instr);
	void gen_store_member(const GDScript2IRInstr &p_instr);
	void gen_load_global(const GDScript2IRInstr &p_instr);
	void gen_store_global(const GDScript2IRInstr &p_instr);
	void gen_load_self(const GDScript2IRInstr &p_instr);
	void gen_binary_op(const GDScript2IRInstr &p_instr);
	void gen_unary_op(const GDScript2IRInstr &p_instr);
	void gen_comparison(const GDScript2IRInstr &p_instr);
	void gen_type_op(const GDScript2IRInstr &p_instr);
	void gen_construct_array(const GDScript2IRInstr &p_instr);
	void gen_construct_dict(const GDScript2IRInstr &p_instr);
	void gen_get_index(const GDScript2IRInstr &p_instr);
	void gen_set_index(const GDScript2IRInstr &p_instr);
	void gen_get_named(const GDScript2IRInstr &p_instr);
	void gen_set_named(const GDScript2IRInstr &p_instr);
	void gen_jump(const GDScript2IRInstr &p_instr);
	void gen_jump_if(const GDScript2IRInstr &p_instr);
	void gen_jump_if_not(const GDScript2IRInstr &p_instr);
	void gen_call(const GDScript2IRInstr &p_instr);
	void gen_call_method(const GDScript2IRInstr &p_instr);
	void gen_call_builtin(const GDScript2IRInstr &p_instr);
	void gen_call_super(const GDScript2IRInstr &p_instr);
	void gen_call_self(const GDScript2IRInstr &p_instr);
	void gen_return(const GDScript2IRInstr &p_instr);
	void gen_return_void(const GDScript2IRInstr &p_instr);
	void gen_iter_begin(const GDScript2IRInstr &p_instr);
	void gen_iter_next(const GDScript2IRInstr &p_instr);
	void gen_iter_get(const GDScript2IRInstr &p_instr);
	void gen_await(const GDScript2IRInstr &p_instr);
	void gen_preload(const GDScript2IRInstr &p_instr);
	void gen_get_node(const GDScript2IRInstr &p_instr);
	void gen_lambda(const GDScript2IRInstr &p_instr);
	void gen_copy(const GDScript2IRInstr &p_instr);
	void gen_assert(const GDScript2IRInstr &p_instr);
	void gen_breakpoint(const GDScript2IRInstr &p_instr);

	// Jump patching
	void add_jump_patch(int32_t p_bytecode_idx, int32_t p_operand_idx, int p_target_block);
	void patch_jumps();

	// Reset state for new function
	void reset_function_state();

protected:
	static void _bind_methods() {}

public:
	struct Result {
		GDScript2CompiledModule module;
		Vector<String> errors;
		bool has_errors() const { return !errors.is_empty(); }
	};

	Result generate(const GDScript2IRModule &p_ir);

	// Utility
	static String disassemble(const GDScript2CompiledModule &p_module);
	static String disassemble_function(const GDScript2CompiledFunction &p_func);
};
