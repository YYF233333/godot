/**************************************************************************/
/*  gdscript2_vm.h                                                        */
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
#include "core/variant/variant.h"
#include "modules/gdscript2/codegen/gdscript2_codegen.h"

// ============================================================================
// Iterator State (for for-loops)
// ============================================================================

struct GDScript2IteratorState {
	Variant container;
	Variant iterator;
	bool valid = false;
	int index = 0; // For array-like iteration

	void begin(const Variant &p_container);
	bool next();
	Variant get() const;
};

// ============================================================================
// Call Frame
// ============================================================================

struct GDScript2CallFrame {
	const GDScript2CompiledFunction *function = nullptr;
	Vector<Variant> stack; // Locals + temporaries
	int ip = 0; // Instruction pointer
	int base_stack = 0; // Base stack index for this frame

	// Self reference for method calls
	Variant self;

	// Iterator states for nested for loops
	Vector<GDScript2IteratorState> iterators;

	// Debug info
	int current_line = 0;

	// Get operand value
	Variant &get_reg(int p_idx);
	const Variant &get_reg(int p_idx) const;

	// Resize stack if needed
	void ensure_stack_size(int p_size);
};

// ============================================================================
// Execution Result
// ============================================================================

struct GDScript2ExecutionResult {
	enum Status {
		OK,
		YIELD, // Coroutine yielded
		ERROR_RUNTIME,
		ERROR_STACK_OVERFLOW,
		ERROR_INVALID_OPCODE,
		ERROR_DIVISION_BY_ZERO,
		ERROR_NULL_REFERENCE,
		ERROR_TYPE_MISMATCH,
		ERROR_INDEX_OUT_OF_BOUNDS,
		ERROR_ASSERTION_FAILED,
	};

	Status status = OK;
	Variant return_value;
	String error_message;
	int error_line = 0;
	String error_function;

	bool is_ok() const { return status == OK; }
	bool has_error() const { return status != OK && status != YIELD; }

	static GDScript2ExecutionResult make_ok(const Variant &p_value = Variant()) {
		GDScript2ExecutionResult r;
		r.status = OK;
		r.return_value = p_value;
		return r;
	}

	static GDScript2ExecutionResult make_error(Status p_status, const String &p_message, int p_line = 0) {
		GDScript2ExecutionResult r;
		r.status = p_status;
		r.error_message = p_message;
		r.error_line = p_line;
		return r;
	}
};

// ============================================================================
// Debug Hooks
// ============================================================================

class GDScript2VMDebugger {
public:
	virtual ~GDScript2VMDebugger() = default;

	// Called when execution enters a new line
	virtual void on_line(int p_line, const StringName &p_function) {}

	// Called when a breakpoint is hit
	virtual void on_breakpoint() {}

	// Called on assertion failure
	virtual void on_assert(bool p_condition, const String &p_message) {}

	// Check if execution should be paused
	virtual bool should_break() { return false; }
};

// ============================================================================
// Virtual Machine
// ============================================================================

class GDScript2VM : public RefCounted {
	GDCLASS(GDScript2VM, RefCounted);

public:
	static constexpr int MAX_CALL_DEPTH = 1024;
	static constexpr int DEFAULT_STACK_SIZE = 256;

private:
	// Module being executed
	const GDScript2CompiledModule *module = nullptr;

	// Call stack
	Vector<GDScript2CallFrame> call_stack;
	int call_depth = 0;

	// Global variables
	HashMap<StringName, Variant> globals;

	// Debug hook
	GDScript2VMDebugger *debugger = nullptr;

	// Execution helpers
	bool execute_instruction(GDScript2CallFrame &p_frame, GDScript2ExecutionResult &r_result);
	const GDScript2CompiledFunction *find_function(const StringName &p_name) const;

	// Operand access helpers
	int32_t get_operand(const GDScript2BytecodeInstr &p_instr, int p_idx) const;
	Variant &get_stack_value(GDScript2CallFrame &p_frame, int32_t p_reg);
	const Variant &get_stack_value_const(const GDScript2CallFrame &p_frame, int32_t p_reg) const;
	const Variant &get_constant(const GDScript2CallFrame &p_frame, int32_t p_idx) const;
	const StringName &get_name(const GDScript2CallFrame &p_frame, int32_t p_idx) const;

	// Instruction implementations
	void exec_load_const(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_nil(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_true(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_false(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_small_int(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_local(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_store_local(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_member(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_store_member(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_global(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_store_global(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_load_self(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	void exec_binary_op(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, Variant::Operator p_op);
	void exec_unary_op(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, Variant::Operator p_op);
	void exec_comparison(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, Variant::Operator p_op);
	void exec_not(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	void exec_type_is(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_type_as(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_typeof(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_in(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	void exec_construct_array(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_construct_dict(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_get_index(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result);
	void exec_set_index(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result);
	void exec_get_named(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result);
	void exec_set_named(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result);

	bool exec_jump(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	bool exec_jump_if(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	bool exec_jump_if_not(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	GDScript2ExecutionResult exec_call(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	GDScript2ExecutionResult exec_call_method(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	GDScript2ExecutionResult exec_call_builtin(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	GDScript2ExecutionResult exec_call_super(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	GDScript2ExecutionResult exec_call_self(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	void exec_iter_begin(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_iter_next(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	void exec_iter_get(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	bool exec_jump_if_iter_end(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	void exec_copy(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);
	bool exec_assert(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result);
	void exec_breakpoint(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr);

	// Internal call implementation
	GDScript2ExecutionResult call_internal(const GDScript2CompiledFunction *p_func, const Vector<Variant> &p_args, const Variant &p_self = Variant());

protected:
	static void _bind_methods() {}

public:
	// Module management
	void load_module(const GDScript2CompiledModule *p_module);
	const GDScript2CompiledModule *get_module() const { return module; }

	// Function execution
	GDScript2ExecutionResult call(const StringName &p_function, const Vector<Variant> &p_args = Vector<Variant>());
	GDScript2ExecutionResult call_with_self(const StringName &p_function, const Variant &p_self, const Vector<Variant> &p_args = Vector<Variant>());

	// Entry point execution
	GDScript2ExecutionResult execute();

	// Global variable access
	void set_global(const StringName &p_name, const Variant &p_value);
	Variant get_global(const StringName &p_name) const;
	bool has_global(const StringName &p_name) const;

	// Debug support
	void set_debugger(GDScript2VMDebugger *p_debugger) { debugger = p_debugger; }
	GDScript2VMDebugger *get_debugger() const { return debugger; }

	// State inspection
	int get_call_depth() const { return call_depth; }
	const Vector<GDScript2CallFrame> &get_call_stack() const { return call_stack; }

	// Reset VM state
	void reset();
};
