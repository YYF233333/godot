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

// Execution state for a single call frame.
struct GDScript2CallFrame {
	const GDScript2CompiledFunction *function = nullptr;
	Vector<Variant> locals;
	int ip = 0; // Instruction pointer.
};

// Execution result.
struct GDScript2ExecutionResult {
	enum Status {
		OK,
		ERROR_RUNTIME,
		ERROR_STACK_OVERFLOW,
	};
	Status status = OK;
	Variant return_value;
	String error_message;
};

// Virtual machine for executing GDScript2 bytecode.
class GDScript2VM : public RefCounted {
	GDCLASS(GDScript2VM, RefCounted);

	static constexpr int MAX_CALL_DEPTH = 1024;

	const GDScript2CompiledModule *module = nullptr;
	Vector<GDScript2CallFrame> call_stack;
	int call_depth = 0;

	// Execute a single instruction, returns false if execution should stop.
	bool execute_instruction(GDScript2CallFrame &p_frame, GDScript2ExecutionResult &r_result);

	// Find function by name.
	const GDScript2CompiledFunction *find_function(const StringName &p_name) const;

protected:
	static void _bind_methods() {}

public:
	// Load a compiled module for execution.
	void load_module(const GDScript2CompiledModule *p_module);

	// Call a function by name with arguments.
	GDScript2ExecutionResult call(const StringName &p_function, const Vector<Variant> &p_args);

	// Execute the entry function (_main).
	GDScript2ExecutionResult execute();
};
