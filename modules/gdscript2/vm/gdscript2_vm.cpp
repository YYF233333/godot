/**************************************************************************/
/*  gdscript2_vm.cpp                                                      */
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

#include "gdscript2_vm.h"

#include "core/variant/variant_utility.h"
#include "modules/gdscript2/runtime/gdscript2_builtin.h"
#include "modules/gdscript2/runtime/gdscript2_signal.h"
#include "modules/gdscript2/runtime/gdscript2_variant_utils.h"
#include "modules/gdscript2/vm/gdscript2_coroutine.h"

// ============================================================================
// GDScript2IteratorState
// ============================================================================

void GDScript2IteratorState::begin(const Variant &p_container) {
	container = p_container;
	index = 0;
	valid = false;

	switch (container.get_type()) {
		case Variant::ARRAY:
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::PACKED_VECTOR4_ARRAY:
		case Variant::STRING: {
			valid = true;
		} break;

		case Variant::DICTIONARY: {
			Dictionary dict = container;
			iterator = dict.keys();
			valid = true;
		} break;

		case Variant::OBJECT: {
			// For objects, try to call _iter_init
			Object *obj = container.get_validated_object();
			if (obj) {
				Array args;
				args.push_back(Variant()); // Placeholder for iterator state
				Variant result = obj->callv("_iter_init", args);
				valid = result.booleanize();
			}
		} break;

		default:
			break;
	}
}

bool GDScript2IteratorState::next() {
	if (!valid) {
		return false;
	}

	switch (container.get_type()) {
		case Variant::ARRAY: {
			Array arr = container;
			index++;
			return index < arr.size();
		}

		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::PACKED_VECTOR4_ARRAY: {
			index++;
			Variant size_var = container.call("size");
			int size = size_var;
			return index < size;
		}

		case Variant::STRING: {
			String str = container;
			index++;
			return index < str.length();
		}

		case Variant::DICTIONARY: {
			Array keys = iterator;
			index++;
			return index < keys.size();
		}

		case Variant::OBJECT: {
			Object *obj = container.get_validated_object();
			if (obj) {
				Array args;
				args.push_back(Variant()); // Placeholder
				Variant result = obj->callv("_iter_next", args);
				return result.booleanize();
			}
			return false;
		}

		default:
			return false;
	}
}

Variant GDScript2IteratorState::get() const {
	if (!valid) {
		return Variant();
	}

	switch (container.get_type()) {
		case Variant::ARRAY: {
			Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_BYTE_ARRAY: {
			PackedByteArray arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_INT32_ARRAY: {
			PackedInt32Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_INT64_ARRAY: {
			PackedInt64Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_FLOAT32_ARRAY: {
			PackedFloat32Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_FLOAT64_ARRAY: {
			PackedFloat64Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_STRING_ARRAY: {
			PackedStringArray arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_VECTOR2_ARRAY: {
			PackedVector2Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_VECTOR3_ARRAY: {
			PackedVector3Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_COLOR_ARRAY: {
			PackedColorArray arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::PACKED_VECTOR4_ARRAY: {
			PackedVector4Array arr = container;
			if (index >= 0 && index < arr.size()) {
				return arr[index];
			}
		} break;

		case Variant::STRING: {
			String str = container;
			if (index >= 0 && index < str.length()) {
				return str.substr(index, 1);
			}
		} break;

		case Variant::DICTIONARY: {
			Array keys = iterator;
			if (index >= 0 && index < keys.size()) {
				return keys[index];
			}
		} break;

		case Variant::OBJECT: {
			Object *obj = container.get_validated_object();
			if (obj) {
				Array args;
				args.push_back(Variant()); // Placeholder
				return obj->callv("_iter_get", args);
			}
		} break;

		default:
			break;
	}

	return Variant();
}

// ============================================================================
// GDScript2CallFrame
// ============================================================================

Variant &GDScript2CallFrame::get_reg(int p_idx) {
	ensure_stack_size(p_idx + 1);
	return stack.write[p_idx];
}

const Variant &GDScript2CallFrame::get_reg(int p_idx) const {
	static Variant nil;
	if (p_idx >= 0 && p_idx < stack.size()) {
		return stack[p_idx];
	}
	return nil;
}

void GDScript2CallFrame::ensure_stack_size(int p_size) {
	if (p_size > stack.size()) {
		stack.resize(p_size);
	}
}

// ============================================================================
// GDScript2VM - Core
// ============================================================================

void GDScript2VM::load_module(const GDScript2CompiledModule *p_module) {
	module = p_module;
	reset();

	// Initialize builtin functions if not already done
	GDScript2BuiltinRegistry::initialize();
}

void GDScript2VM::reset() {
	call_stack.clear();
	call_depth = 0;
	globals.clear();
	current_coroutine = nullptr;
	if (coroutine_manager) {
		coroutine_manager->cancel_all();
	}
}

// ============================================================================
// Signal Operations
// ============================================================================

void GDScript2VM::exec_signal_define(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	// Get signal name from name pool
	int32_t name_idx = get_operand(p_instr, 0);
	const StringName &signal_name = get_name(p_frame, name_idx);

	// Get self object
	if (p_frame.self.get_type() != Variant::OBJECT) {
		ERR_PRINT("Cannot define signal on non-object.");
		return;
	}

	Object *obj = p_frame.self;
	if (!obj) {
		ERR_PRINT("Self is null.");
		return;
	}

	// Add user signal if not already defined
	if (!obj->has_signal(signal_name)) {
		obj->add_user_signal(MethodInfo(signal_name));
	}
}

void GDScript2VM::exec_signal_connect(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t signal_reg = get_operand(p_instr, 1);
	int32_t callable_reg = get_operand(p_instr, 2);
	int32_t flags = (p_instr.operands.size() >= 4) ? get_operand(p_instr, 3) : 0;

	Variant signal_var = get_stack_value(p_frame, signal_reg);
	Variant callable_var = get_stack_value(p_frame, callable_reg);

	if (signal_var.get_type() != Variant::SIGNAL) {
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_TYPE_MISMATCH,
				"First argument must be a Signal.",
				p_frame.current_line);
		return;
	}

	if (callable_var.get_type() != Variant::CALLABLE) {
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_TYPE_MISMATCH,
				"Second argument must be a Callable.",
				p_frame.current_line);
		return;
	}

	Signal sig = signal_var;
	Callable callable = callable_var;

	Error err = GDScript2SignalUtils::safe_connect(sig, callable, flags);
	get_stack_value(p_frame, dest) = (err == OK);
}

void GDScript2VM::exec_signal_disconnect(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t signal_reg = get_operand(p_instr, 0);
	int32_t callable_reg = get_operand(p_instr, 1);

	Variant signal_var = get_stack_value(p_frame, signal_reg);
	Variant callable_var = get_stack_value(p_frame, callable_reg);

	if (signal_var.get_type() != Variant::SIGNAL) {
		ERR_PRINT("First argument must be a Signal.");
		return;
	}

	if (callable_var.get_type() != Variant::CALLABLE) {
		ERR_PRINT("Second argument must be a Callable.");
		return;
	}

	Signal sig = signal_var;
	Callable callable = callable_var;

	GDScript2SignalUtils::safe_disconnect(sig, callable);
}

void GDScript2VM::exec_signal_emit(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t signal_reg = get_operand(p_instr, 0);
	Variant signal_var = get_stack_value(p_frame, signal_reg);

	if (signal_var.get_type() != Variant::SIGNAL) {
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_TYPE_MISMATCH,
				"First argument must be a Signal.",
				p_frame.current_line);
		return;
	}

	Signal sig = signal_var;

	// Collect arguments
	LocalVector<const Variant *> args;
	for (int i = 1; i < (int)p_instr.operands.size(); i++) {
		int32_t arg_reg = get_operand(p_instr, i);
		args.push_back(&get_stack_value_const(p_frame, arg_reg));
	}

	Error err = GDScript2SignalUtils::safe_emit(sig, args.ptr(), args.size());
	if (err != OK) {
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Failed to emit signal.",
				p_frame.current_line);
	}
}

void GDScript2VM::exec_signal_is_connected(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t signal_reg = get_operand(p_instr, 1);
	int32_t callable_reg = get_operand(p_instr, 2);

	Variant signal_var = get_stack_value(p_frame, signal_reg);
	Variant callable_var = get_stack_value(p_frame, callable_reg);

	bool connected = false;

	if (signal_var.get_type() == Variant::SIGNAL && callable_var.get_type() == Variant::CALLABLE) {
		Signal sig = signal_var;
		Callable callable = callable_var;

		Object *obj = sig.get_object();
		if (obj) {
			connected = obj->is_connected(sig.get_name(), callable);
		}
	}

	get_stack_value(p_frame, dest) = connected;
}

void GDScript2VM::exec_make_signal(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t object_reg = get_operand(p_instr, 1);
	int32_t name_idx = get_operand(p_instr, 2);

	Variant object_var = get_stack_value(p_frame, object_reg);
	const StringName &signal_name = get_name(p_frame, name_idx);

	if (object_var.get_type() != Variant::OBJECT) {
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_TYPE_MISMATCH,
				"First argument must be an Object.",
				p_frame.current_line);
		return;
	}

	Object *obj = object_var;
	if (!obj) {
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_NULL_REFERENCE,
				"Object is null.",
				p_frame.current_line);
		return;
	}

	Signal sig = GDScript2SignalUtils::make_signal(obj, signal_name);
	get_stack_value(p_frame, dest) = sig;
}

// ============================================================================
// Coroutine Support
// ============================================================================

void GDScript2VM::exec_await(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t source = get_operand(p_instr, 1);

	Variant await_target = get_stack_value(p_frame, source);

	// Check what we're awaiting
	if (await_target.get_type() == Variant::SIGNAL) {
		// Awaiting a signal
		Signal sig = await_target;
		Object *obj = sig.get_object();
		StringName signal_name = sig.get_name();

		if (!obj) {
			r_result = GDScript2ExecutionResult::make_error(
					GDScript2ExecutionResult::ERROR_NULL_REFERENCE,
					"Cannot await signal on null object.",
					p_frame.current_line);
			return;
		}

		// Create or get coroutine
		GDScript2Coroutine *coro = suspend_coroutine(await_target);
		if (!coro) {
			r_result = GDScript2ExecutionResult::make_error(
					GDScript2ExecutionResult::ERROR_RUNTIME,
					"Failed to create coroutine for await.",
					p_frame.current_line);
			return;
		}

		// Set up signal wait
		coro->wait_for_signal(obj, signal_name);

		// Mark as yielded
		r_result.status = GDScript2ExecutionResult::YIELD;
		r_result.return_value = Ref<GDScript2Coroutine>(coro);

	} else if (await_target.get_type() == Variant::OBJECT) {
		// Check if it's a coroutine
		Ref<GDScript2Coroutine> coro_ref = await_target;
		if (coro_ref.is_valid()) {
			// Awaiting another coroutine
			if (coro_ref->is_completed()) {
				// Already completed, return its value immediately
				get_stack_value(p_frame, dest) = coro_ref->get_resume_value();
			} else {
				// Wait for completion
				GDScript2Coroutine *coro = suspend_coroutine(await_target);
				if (!coro) {
					r_result = GDScript2ExecutionResult::make_error(
							GDScript2ExecutionResult::ERROR_RUNTIME,
							"Failed to create coroutine for await.",
							p_frame.current_line);
					return;
				}

				// Set up completion callback to resume this coroutine
				Callable callback = callable_mp(coro, &GDScript2Coroutine::resume);
				coro_ref->set_completion_callback(callback);

				// Mark as yielded
				r_result.status = GDScript2ExecutionResult::YIELD;
				r_result.return_value = Ref<GDScript2Coroutine>(coro);
			}
		} else {
			// Unknown object type
			r_result = GDScript2ExecutionResult::make_error(
					GDScript2ExecutionResult::ERROR_TYPE_MISMATCH,
					vformat("Cannot await value of type '%s'.", Variant::get_type_name(await_target.get_type())),
					p_frame.current_line);
		}
	} else {
		// Invalid await target
		r_result = GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_TYPE_MISMATCH,
				vformat("Cannot await value of type '%s'.", Variant::get_type_name(await_target.get_type())),
				p_frame.current_line);
	}
}

GDScript2Coroutine *GDScript2VM::suspend_coroutine(const Variant &p_await_target) {
	if (!coroutine_manager) {
		ERR_PRINT("No coroutine manager set on VM.");
		return nullptr;
	}

	// Create new coroutine if not already in one
	GDScript2Coroutine *coro = current_coroutine;
	if (!coro) {
		Ref<GDScript2Coroutine> coro_ref = coroutine_manager->create_coroutine();
		coro = coro_ref.ptr();
		current_coroutine = coro;
	}

	// Save current execution state
	coro->save_state(call_stack, call_depth);

	return coro;
}

void GDScript2VM::resume_coroutine(GDScript2Coroutine *p_coroutine) {
	if (!p_coroutine || !p_coroutine->has_saved_state()) {
		ERR_PRINT("Cannot resume coroutine with no saved state.");
		return;
	}

	// Set as current coroutine
	current_coroutine = p_coroutine;

	// Restore execution state
	p_coroutine->restore_state(call_stack, call_depth);

	// Get the resume value
	Variant resume_value = p_coroutine->get_resume_value();

	// Store resume value in the destination register of the await instruction
	// The await instruction should have stored the dest register somewhere accessible
	// For now, we'll store it in the top frame's first register (simplified)
	if (!call_stack.is_empty()) {
		GDScript2CallFrame &frame = call_stack.write[call_stack.size() - 1];
		if (frame.stack.size() > 0) {
			// Find the await instruction and get its dest register
			// For simplicity, we'll use register 0 for now
			// In a real implementation, this should be tracked properly
			frame.stack.write[0] = resume_value;
		}
	}

	// Continue execution from where we left off
	GDScript2ExecutionResult result;
	while (call_depth > 0 && call_depth <= call_stack.size()) {
		GDScript2CallFrame &frame = call_stack.write[call_depth - 1];

		if (!execute_instruction(frame, result)) {
			// Hit another yield or error
			if (result.status == GDScript2ExecutionResult::YIELD) {
				// Suspended again
				return;
			} else if (result.has_error()) {
				// Error occurred
				p_coroutine->set_error(result.error_message, result.error_line);
				current_coroutine = nullptr;
				return;
			} else {
				// Normal return
				call_depth--;
			}
		}

		// Check if we've completed
		if (call_depth == 0) {
			p_coroutine->complete(result.return_value);
			current_coroutine = nullptr;
			return;
		}
	}
}

const GDScript2CompiledFunction *GDScript2VM::find_function(const StringName &p_name) const {
	if (module == nullptr) {
		return nullptr;
	}

	if (module->function_map.has(p_name)) {
		int idx = module->function_map[p_name];
		return &module->functions[idx];
	}

	// Fallback: linear search
	for (int i = 0; i < module->functions.size(); i++) {
		if (module->functions[i].name == p_name) {
			return &module->functions[i];
		}
	}

	return nullptr;
}

// ============================================================================
// Operand Access Helpers
// ============================================================================

int32_t GDScript2VM::get_operand(const GDScript2BytecodeInstr &p_instr, int p_idx) const {
	if ((uint32_t)p_idx < p_instr.operands.size()) {
		return p_instr.operands[p_idx];
	}
	return -1;
}

Variant &GDScript2VM::get_stack_value(GDScript2CallFrame &p_frame, int32_t p_reg) {
	return p_frame.get_reg(p_reg);
}

const Variant &GDScript2VM::get_stack_value_const(const GDScript2CallFrame &p_frame, int32_t p_reg) const {
	return p_frame.get_reg(p_reg);
}

const Variant &GDScript2VM::get_constant(const GDScript2CallFrame &p_frame, int32_t p_idx) const {
	static Variant nil;
	if (p_frame.function && p_idx >= 0 && p_idx < p_frame.function->constants.size()) {
		return p_frame.function->constants[p_idx];
	}
	return nil;
}

const StringName &GDScript2VM::get_name(const GDScript2CallFrame &p_frame, int32_t p_idx) const {
	static StringName empty;
	if (p_frame.function && p_idx >= 0 && p_idx < p_frame.function->names.size()) {
		return p_frame.function->names[p_idx];
	}
	return empty;
}

// ============================================================================
// Global Variable Access
// ============================================================================

void GDScript2VM::set_global(const StringName &p_name, const Variant &p_value) {
	globals[p_name] = p_value;
}

Variant GDScript2VM::get_global(const StringName &p_name) const {
	if (globals.has(p_name)) {
		return globals[p_name];
	}
	return Variant();
}

bool GDScript2VM::has_global(const StringName &p_name) const {
	return globals.has(p_name);
}

// ============================================================================
// Instruction Implementations - Load/Store
// ============================================================================

void GDScript2VM::exec_load_const(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t const_idx = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = get_constant(p_frame, const_idx);
}

void GDScript2VM::exec_load_nil(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	get_stack_value(p_frame, dest) = Variant();
}

void GDScript2VM::exec_load_true(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	get_stack_value(p_frame, dest) = true;
}

void GDScript2VM::exec_load_false(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	get_stack_value(p_frame, dest) = false;
}

void GDScript2VM::exec_load_small_int(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t value = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = value;
}

void GDScript2VM::exec_load_local(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t src = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = get_stack_value_const(p_frame, src);
}

void GDScript2VM::exec_store_local(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t src = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = get_stack_value_const(p_frame, src);
}

void GDScript2VM::exec_load_member(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t name_idx = get_operand(p_instr, 1);
	const StringName &name = get_name(p_frame, name_idx);

	if (p_frame.self.get_type() == Variant::OBJECT) {
		Object *obj = p_frame.self.get_validated_object();
		if (obj) {
			bool valid = false;
			get_stack_value(p_frame, dest) = obj->get(name, &valid);
		}
	}
}

void GDScript2VM::exec_store_member(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t name_idx = get_operand(p_instr, 0);
	int32_t src = get_operand(p_instr, 1);
	const StringName &name = get_name(p_frame, name_idx);

	if (p_frame.self.get_type() == Variant::OBJECT) {
		Object *obj = p_frame.self.get_validated_object();
		if (obj) {
			bool valid = false;
			obj->set(name, get_stack_value_const(p_frame, src), &valid);
		}
	}
}

void GDScript2VM::exec_load_global(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t name_idx = get_operand(p_instr, 1);
	const StringName &name = get_name(p_frame, name_idx);
	get_stack_value(p_frame, dest) = get_global(name);
}

void GDScript2VM::exec_store_global(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t name_idx = get_operand(p_instr, 0);
	int32_t src = get_operand(p_instr, 1);
	const StringName &name = get_name(p_frame, name_idx);
	set_global(name, get_stack_value_const(p_frame, src));
}

void GDScript2VM::exec_load_self(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	get_stack_value(p_frame, dest) = p_frame.self;
}

// ============================================================================
// Instruction Implementations - Operators
// ============================================================================

void GDScript2VM::exec_binary_op(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, Variant::Operator p_op) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t left = get_operand(p_instr, 1);
	int32_t right = get_operand(p_instr, 2);

	Variant result;
	bool valid = true;
	Variant::evaluate(p_op, get_stack_value_const(p_frame, left), get_stack_value_const(p_frame, right), result, valid);
	get_stack_value(p_frame, dest) = result;
}

void GDScript2VM::exec_unary_op(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, Variant::Operator p_op) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t operand = get_operand(p_instr, 1);

	Variant result;
	bool valid = true;
	// Unary operators: negate, positive, not, bit_not
	Variant::evaluate(p_op, get_stack_value_const(p_frame, operand), Variant(), result, valid);
	get_stack_value(p_frame, dest) = result;
}

void GDScript2VM::exec_comparison(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, Variant::Operator p_op) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t left = get_operand(p_instr, 1);
	int32_t right = get_operand(p_instr, 2);

	Variant result;
	bool valid = true;
	Variant::evaluate(p_op, get_stack_value_const(p_frame, left), get_stack_value_const(p_frame, right), result, valid);
	get_stack_value(p_frame, dest) = result;
}

void GDScript2VM::exec_not(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t operand = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = !get_stack_value_const(p_frame, operand).booleanize();
}

// ============================================================================
// Instruction Implementations - Type Operations
// ============================================================================

void GDScript2VM::exec_type_is(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t value_reg = get_operand(p_instr, 1);
	int32_t type_reg = get_operand(p_instr, 2);

	const Variant &value = get_stack_value_const(p_frame, value_reg);
	const Variant &type_val = get_stack_value_const(p_frame, type_reg);

	bool result = false;

	// Check if type is a type constant (int representing Variant::Type)
	if (type_val.get_type() == Variant::INT) {
		result = (int)value.get_type() == (int)type_val;
	} else if (type_val.get_type() == Variant::OBJECT) {
		// Check if value is instance of class
		Object *type_obj = type_val.get_validated_object();
		Object *value_obj = value.get_validated_object();
		if (value_obj && type_obj) {
			// Would need class comparison logic here
			result = value_obj->is_class(type_obj->get_class());
		}
	}

	get_stack_value(p_frame, dest) = result;
}

void GDScript2VM::exec_type_as(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t value_reg = get_operand(p_instr, 1);
	int32_t type_reg = get_operand(p_instr, 2);

	const Variant &value = get_stack_value_const(p_frame, value_reg);
	// const Variant &type_val = get_stack_value_const(p_frame, type_reg);
	(void)type_reg; // TODO: Implement proper type conversion

	// Simple type cast - just assign for now
	// Full implementation would do proper type conversion
	get_stack_value(p_frame, dest) = value;
}

void GDScript2VM::exec_typeof(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t operand = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = (int)get_stack_value_const(p_frame, operand).get_type();
}

void GDScript2VM::exec_in(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t left = get_operand(p_instr, 1);
	int32_t right = get_operand(p_instr, 2);

	Variant result;
	bool valid = true;
	Variant::evaluate(Variant::OP_IN, get_stack_value_const(p_frame, left), get_stack_value_const(p_frame, right), result, valid);
	get_stack_value(p_frame, dest) = result;
}

// ============================================================================
// Instruction Implementations - Container Operations
// ============================================================================

void GDScript2VM::exec_construct_array(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t count = get_operand(p_instr, 1);

	Array arr;
	arr.resize(count);
	for (int i = 0; i < count; i++) {
		int32_t elem_reg = get_operand(p_instr, 2 + i);
		arr[i] = get_stack_value_const(p_frame, elem_reg);
	}
	get_stack_value(p_frame, dest) = arr;
}

void GDScript2VM::exec_construct_dict(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t pair_count = get_operand(p_instr, 1);

	Dictionary dict;
	for (int i = 0; i < pair_count; i++) {
		int32_t key_reg = get_operand(p_instr, 2 + i * 2);
		int32_t val_reg = get_operand(p_instr, 2 + i * 2 + 1);
		dict[get_stack_value_const(p_frame, key_reg)] = get_stack_value_const(p_frame, val_reg);
	}
	get_stack_value(p_frame, dest) = dict;
}

void GDScript2VM::exec_get_index(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t base = get_operand(p_instr, 1);
	int32_t index = get_operand(p_instr, 2);

	bool valid = false;
	Variant result = get_stack_value_const(p_frame, base).get(get_stack_value_const(p_frame, index), &valid);

	if (!valid) {
		r_result.status = GDScript2ExecutionResult::ERROR_INDEX_OUT_OF_BOUNDS;
		r_result.error_message = "Invalid index access";
		return;
	}

	get_stack_value(p_frame, dest) = result;
}

void GDScript2VM::exec_set_index(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t base = get_operand(p_instr, 0);
	int32_t index = get_operand(p_instr, 1);
	int32_t value = get_operand(p_instr, 2);

	bool valid = false;
	get_stack_value(p_frame, base).set(get_stack_value_const(p_frame, index), get_stack_value_const(p_frame, value), &valid);

	if (!valid) {
		r_result.status = GDScript2ExecutionResult::ERROR_INDEX_OUT_OF_BOUNDS;
		r_result.error_message = "Invalid index assignment";
	}
}

void GDScript2VM::exec_get_named(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t base = get_operand(p_instr, 1);
	int32_t name_idx = get_operand(p_instr, 2);
	const StringName &name = get_name(p_frame, name_idx);

	bool valid = false;
	Variant result = get_stack_value_const(p_frame, base).get_named(name, valid);

	get_stack_value(p_frame, dest) = result;
}

void GDScript2VM::exec_set_named(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t base = get_operand(p_instr, 0);
	int32_t name_idx = get_operand(p_instr, 1);
	int32_t value = get_operand(p_instr, 2);
	const StringName &name = get_name(p_frame, name_idx);

	bool valid = false;
	get_stack_value(p_frame, base).set_named(name, get_stack_value_const(p_frame, value), valid);
}

// ============================================================================
// Instruction Implementations - Control Flow
// ============================================================================

bool GDScript2VM::exec_jump(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t target = get_operand(p_instr, 0);
	p_frame.ip = target;
	return true; // Jump handled, don't increment IP
}

bool GDScript2VM::exec_jump_if(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t cond = get_operand(p_instr, 0);
	int32_t target = get_operand(p_instr, 1);

	if (get_stack_value_const(p_frame, cond).booleanize()) {
		p_frame.ip = target;
		return true;
	}
	return false; // Condition false, increment IP normally
}

bool GDScript2VM::exec_jump_if_not(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t cond = get_operand(p_instr, 0);
	int32_t target = get_operand(p_instr, 1);

	if (!get_stack_value_const(p_frame, cond).booleanize()) {
		p_frame.ip = target;
		return true;
	}
	return false;
}

// ============================================================================
// Instruction Implementations - Function Calls
// ============================================================================

GDScript2ExecutionResult GDScript2VM::exec_call(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t callee_reg = get_operand(p_instr, 1);
	int32_t arg_count = get_operand(p_instr, 2);

	const Variant &callee = get_stack_value_const(p_frame, callee_reg);

	// Build arguments
	Vector<Variant> args;
	args.resize(arg_count);
	for (int i = 0; i < arg_count; i++) {
		int32_t arg_reg = get_operand(p_instr, 3 + i);
		args.write[i] = get_stack_value_const(p_frame, arg_reg);
	}

	// If callee is a callable
	if (callee.get_type() == Variant::CALLABLE) {
		Callable callable = callee;
		Variant result;
		Callable::CallError error;

		const Variant **argptrs = nullptr;
		if (!args.is_empty()) {
			argptrs = (const Variant **)alloca(sizeof(Variant *) * args.size());
			for (int i = 0; i < args.size(); i++) {
				argptrs[i] = &args[i];
			}
		}

		callable.callp(argptrs, args.size(), result, error);

		if (error.error != Callable::CallError::CALL_OK) {
			return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME, "Call failed");
		}

		get_stack_value(p_frame, dest) = result;
	}

	return GDScript2ExecutionResult::make_ok();
}

GDScript2ExecutionResult GDScript2VM::exec_call_method(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t base_reg = get_operand(p_instr, 1);
	int32_t name_idx = get_operand(p_instr, 2);
	int32_t arg_count = get_operand(p_instr, 3);

	Variant base = get_stack_value_const(p_frame, base_reg);
	const StringName &method = get_name(p_frame, name_idx);

	// Build arguments
	Vector<Variant> args;
	args.resize(arg_count);
	for (int i = 0; i < arg_count; i++) {
		int32_t arg_reg = get_operand(p_instr, 4 + i);
		args.write[i] = get_stack_value_const(p_frame, arg_reg);
	}

	// Call method on variant
	Variant result;
	Callable::CallError error;

	const Variant **argptrs = nullptr;
	if (!args.is_empty()) {
		argptrs = (const Variant **)alloca(sizeof(Variant *) * args.size());
		for (int i = 0; i < args.size(); i++) {
			argptrs[i] = &args[i];
		}
	}

	base.callp(method, argptrs, args.size(), result, error);

	if (error.error != Callable::CallError::CALL_OK) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
				"Method call failed: " + String(method));
	}

	get_stack_value(p_frame, dest) = result;
	return GDScript2ExecutionResult::make_ok();
}

GDScript2ExecutionResult GDScript2VM::exec_call_builtin(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t name_idx = get_operand(p_instr, 1);
	int32_t arg_count = get_operand(p_instr, 2);

	const StringName &func_name = get_name(p_frame, name_idx);

	// Build arguments
	Vector<Variant> args;
	args.resize(arg_count);
	for (int i = 0; i < arg_count; i++) {
		int32_t arg_reg = get_operand(p_instr, 3 + i);
		args.write[i] = get_stack_value_const(p_frame, arg_reg);
	}

	Variant result;
	Callable::CallError error;

	// Try GDScript2 builtin registry first
	if (GDScript2BuiltinRegistry::has_function(func_name)) {
		const Variant **argptrs = nullptr;
		if (!args.is_empty()) {
			argptrs = (const Variant **)alloca(sizeof(Variant *) * args.size());
			for (int i = 0; i < args.size(); i++) {
				argptrs[i] = &args[i];
			}
		}

		result = GDScript2BuiltinRegistry::call_function(func_name, argptrs, args.size(), error);

		if (error.error != Callable::CallError::CALL_OK) {
			return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
					"Builtin call failed: " + String(func_name));
		}

		get_stack_value(p_frame, dest) = result;
		return GDScript2ExecutionResult::make_ok();
	}

	// Fallback to Godot utility functions
	if (Variant::has_utility_function(func_name)) {
		const Variant **argptrs = nullptr;
		if (!args.is_empty()) {
			argptrs = (const Variant **)alloca(sizeof(Variant *) * args.size());
			for (int i = 0; i < args.size(); i++) {
				argptrs[i] = &args[i];
			}
		}

		Variant::call_utility_function(func_name, &result, argptrs, args.size(), error);

		if (error.error != Callable::CallError::CALL_OK) {
			return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
					"Utility function call failed: " + String(func_name));
		}

		get_stack_value(p_frame, dest) = result;
		return GDScript2ExecutionResult::make_ok();
	}

	// Function not found
	return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
			"Builtin function not found: " + String(func_name));
}

GDScript2ExecutionResult GDScript2VM::exec_call_super(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	// Super calls need parent class context - simplified for now
	int32_t dest = get_operand(p_instr, 0);
	get_stack_value(p_frame, dest) = Variant();
	return GDScript2ExecutionResult::make_ok();
}

GDScript2ExecutionResult GDScript2VM::exec_call_self(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t name_idx = get_operand(p_instr, 1);
	int32_t arg_count = get_operand(p_instr, 2);

	const StringName &func_name = get_name(p_frame, name_idx);

	// Build arguments
	Vector<Variant> args;
	args.resize(arg_count);
	for (int i = 0; i < arg_count; i++) {
		int32_t arg_reg = get_operand(p_instr, 3 + i);
		args.write[i] = get_stack_value_const(p_frame, arg_reg);
	}

	// Try to find function in module
	const GDScript2CompiledFunction *target_func = find_function(func_name);
	if (target_func) {
		GDScript2ExecutionResult result = call_internal(target_func, args, p_frame.self);
		get_stack_value(p_frame, dest) = result.return_value;
		return result;
	}

	// If not found in module, try calling on self object
	if (p_frame.self.get_type() == Variant::OBJECT) {
		Object *obj = p_frame.self.get_validated_object();
		if (obj) {
			Variant result;
			Callable::CallError error;

			const Variant **argptrs = nullptr;
			if (!args.is_empty()) {
				argptrs = (const Variant **)alloca(sizeof(Variant *) * args.size());
				for (int i = 0; i < args.size(); i++) {
					argptrs[i] = &args[i];
				}
			}

			result = obj->callp(func_name, argptrs, args.size(), error);

			if (error.error != Callable::CallError::CALL_OK) {
				return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
						"Self call failed: " + String(func_name));
			}

			get_stack_value(p_frame, dest) = result;
		}
	}

	return GDScript2ExecutionResult::make_ok();
}

// ============================================================================
// Instruction Implementations - Iterators
// ============================================================================

void GDScript2VM::exec_iter_begin(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t container_reg = get_operand(p_instr, 1);

	// Create new iterator state
	GDScript2IteratorState iter;
	iter.begin(get_stack_value_const(p_frame, container_reg));
	p_frame.iterators.push_back(iter);

	// Store iterator index in dest register
	get_stack_value(p_frame, dest) = (int)(p_frame.iterators.size() - 1);
}

void GDScript2VM::exec_iter_next(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t iter_reg = get_operand(p_instr, 1);

	int iter_idx = get_stack_value_const(p_frame, iter_reg);
	if (iter_idx >= 0 && iter_idx < p_frame.iterators.size()) {
		bool has_next = p_frame.iterators.write[iter_idx].next();
		get_stack_value(p_frame, dest) = has_next;
	} else {
		get_stack_value(p_frame, dest) = false;
	}
}

void GDScript2VM::exec_iter_get(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t iter_reg = get_operand(p_instr, 1);

	int iter_idx = get_stack_value_const(p_frame, iter_reg);
	if (iter_idx >= 0 && iter_idx < p_frame.iterators.size()) {
		get_stack_value(p_frame, dest) = p_frame.iterators[iter_idx].get();
	} else {
		get_stack_value(p_frame, dest) = Variant();
	}
}

bool GDScript2VM::exec_jump_if_iter_end(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t iter_reg = get_operand(p_instr, 0);
	int32_t target = get_operand(p_instr, 1);

	int iter_idx = get_stack_value_const(p_frame, iter_reg);
	if (iter_idx >= 0 && iter_idx < p_frame.iterators.size()) {
		if (!p_frame.iterators[iter_idx].valid) {
			p_frame.ip = target;
			return true;
		}
	} else {
		// Invalid iterator, jump to end
		p_frame.ip = target;
		return true;
	}
	return false;
}

// ============================================================================
// Instruction Implementations - Misc
// ============================================================================

void GDScript2VM::exec_copy(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	int32_t dest = get_operand(p_instr, 0);
	int32_t src = get_operand(p_instr, 1);
	get_stack_value(p_frame, dest) = get_stack_value_const(p_frame, src);
}

bool GDScript2VM::exec_assert(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr, GDScript2ExecutionResult &r_result) {
	int32_t cond_reg = get_operand(p_instr, 0);
	int32_t msg_reg = get_operand(p_instr, 1);

	bool condition = get_stack_value_const(p_frame, cond_reg).booleanize();

	if (!condition) {
		String message = "Assertion failed";
		if (msg_reg >= 0) {
			message += ": " + get_stack_value_const(p_frame, msg_reg).stringify();
		}

		if (debugger) {
			debugger->on_assert(false, message);
		}

		r_result.status = GDScript2ExecutionResult::ERROR_ASSERTION_FAILED;
		r_result.error_message = message;
		r_result.error_line = p_frame.current_line;
		return false; // Stop execution
	}

	return true;
}

void GDScript2VM::exec_breakpoint(GDScript2CallFrame &p_frame, const GDScript2BytecodeInstr &p_instr) {
	if (debugger) {
		debugger->on_breakpoint();
	}
}

// ============================================================================
// Main Execution Loop
// ============================================================================

bool GDScript2VM::execute_instruction(GDScript2CallFrame &p_frame, GDScript2ExecutionResult &r_result) {
	if (p_frame.function == nullptr || p_frame.ip < 0 || p_frame.ip >= p_frame.function->code.size()) {
		r_result.status = GDScript2ExecutionResult::ERROR_RUNTIME;
		r_result.error_message = "Invalid instruction pointer";
		return false;
	}

	const GDScript2BytecodeInstr &instr = p_frame.function->code[p_frame.ip];

	// Update current line
	if (instr.line > 0) {
		p_frame.current_line = instr.line;
		if (debugger) {
			debugger->on_line(p_frame.current_line, p_frame.function->name);
			if (debugger->should_break()) {
				return false;
			}
		}
	}

	bool increment_ip = true;

	switch (instr.op) {
		case GDScript2Opcode::OP_NOP:
			break;

		case GDScript2Opcode::OP_END:
			return false;

		case GDScript2Opcode::OP_LOAD_CONST:
			exec_load_const(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_NIL:
			exec_load_nil(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_TRUE:
			exec_load_true(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_FALSE:
			exec_load_false(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_SMALL_INT:
			exec_load_small_int(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_LOCAL:
			exec_load_local(p_frame, instr);
			break;

		case GDScript2Opcode::OP_STORE_LOCAL:
			exec_store_local(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_MEMBER:
			exec_load_member(p_frame, instr);
			break;

		case GDScript2Opcode::OP_STORE_MEMBER:
			exec_store_member(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_GLOBAL:
			exec_load_global(p_frame, instr);
			break;

		case GDScript2Opcode::OP_STORE_GLOBAL:
			exec_store_global(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LOAD_SELF:
			exec_load_self(p_frame, instr);
			break;

		// Arithmetic
		case GDScript2Opcode::OP_ADD:
			exec_binary_op(p_frame, instr, Variant::OP_ADD);
			break;
		case GDScript2Opcode::OP_SUB:
			exec_binary_op(p_frame, instr, Variant::OP_SUBTRACT);
			break;
		case GDScript2Opcode::OP_MUL:
			exec_binary_op(p_frame, instr, Variant::OP_MULTIPLY);
			break;
		case GDScript2Opcode::OP_DIV:
			exec_binary_op(p_frame, instr, Variant::OP_DIVIDE);
			break;
		case GDScript2Opcode::OP_MOD:
			exec_binary_op(p_frame, instr, Variant::OP_MODULE);
			break;
		case GDScript2Opcode::OP_POW:
			exec_binary_op(p_frame, instr, Variant::OP_POWER);
			break;
		case GDScript2Opcode::OP_NEG:
			exec_unary_op(p_frame, instr, Variant::OP_NEGATE);
			break;
		case GDScript2Opcode::OP_POS:
			exec_unary_op(p_frame, instr, Variant::OP_POSITIVE);
			break;

		// Bitwise
		case GDScript2Opcode::OP_BIT_AND:
			exec_binary_op(p_frame, instr, Variant::OP_BIT_AND);
			break;
		case GDScript2Opcode::OP_BIT_OR:
			exec_binary_op(p_frame, instr, Variant::OP_BIT_OR);
			break;
		case GDScript2Opcode::OP_BIT_XOR:
			exec_binary_op(p_frame, instr, Variant::OP_BIT_XOR);
			break;
		case GDScript2Opcode::OP_BIT_NOT:
			exec_unary_op(p_frame, instr, Variant::OP_BIT_NEGATE);
			break;
		case GDScript2Opcode::OP_BIT_LSH:
			exec_binary_op(p_frame, instr, Variant::OP_SHIFT_LEFT);
			break;
		case GDScript2Opcode::OP_BIT_RSH:
			exec_binary_op(p_frame, instr, Variant::OP_SHIFT_RIGHT);
			break;

		// Comparison
		case GDScript2Opcode::OP_EQ:
			exec_comparison(p_frame, instr, Variant::OP_EQUAL);
			break;
		case GDScript2Opcode::OP_NE:
			exec_comparison(p_frame, instr, Variant::OP_NOT_EQUAL);
			break;
		case GDScript2Opcode::OP_LT:
			exec_comparison(p_frame, instr, Variant::OP_LESS);
			break;
		case GDScript2Opcode::OP_LE:
			exec_comparison(p_frame, instr, Variant::OP_LESS_EQUAL);
			break;
		case GDScript2Opcode::OP_GT:
			exec_comparison(p_frame, instr, Variant::OP_GREATER);
			break;
		case GDScript2Opcode::OP_GE:
			exec_comparison(p_frame, instr, Variant::OP_GREATER_EQUAL);
			break;

		// Logical
		case GDScript2Opcode::OP_NOT:
			exec_not(p_frame, instr);
			break;

		// Type operations
		case GDScript2Opcode::OP_IS:
			exec_type_is(p_frame, instr);
			break;
		case GDScript2Opcode::OP_AS:
			exec_type_as(p_frame, instr);
			break;
		case GDScript2Opcode::OP_TYPEOF:
			exec_typeof(p_frame, instr);
			break;
		case GDScript2Opcode::OP_IN:
			exec_in(p_frame, instr);
			break;

		// Container operations
		case GDScript2Opcode::OP_CONSTRUCT_ARRAY:
			exec_construct_array(p_frame, instr);
			break;
		case GDScript2Opcode::OP_CONSTRUCT_DICT:
			exec_construct_dict(p_frame, instr);
			break;
		case GDScript2Opcode::OP_GET_INDEX:
			exec_get_index(p_frame, instr, r_result);
			if (r_result.has_error()) {
				return false;
			}
			break;
		case GDScript2Opcode::OP_SET_INDEX:
			exec_set_index(p_frame, instr, r_result);
			if (r_result.has_error()) {
				return false;
			}
			break;
		case GDScript2Opcode::OP_GET_NAMED:
			exec_get_named(p_frame, instr, r_result);
			break;
		case GDScript2Opcode::OP_SET_NAMED:
			exec_set_named(p_frame, instr, r_result);
			break;

		// Control flow
		case GDScript2Opcode::OP_JUMP:
			increment_ip = !exec_jump(p_frame, instr);
			break;
		case GDScript2Opcode::OP_JUMP_IF:
			increment_ip = !exec_jump_if(p_frame, instr);
			break;
		case GDScript2Opcode::OP_JUMP_IF_NOT:
			increment_ip = !exec_jump_if_not(p_frame, instr);
			break;

		// Function calls
		case GDScript2Opcode::OP_CALL: {
			GDScript2ExecutionResult call_result = exec_call(p_frame, instr);
			if (call_result.has_error()) {
				r_result = call_result;
				return false;
			}
		} break;
		case GDScript2Opcode::OP_CALL_METHOD: {
			GDScript2ExecutionResult call_result = exec_call_method(p_frame, instr);
			if (call_result.has_error()) {
				r_result = call_result;
				return false;
			}
		} break;
		case GDScript2Opcode::OP_CALL_BUILTIN: {
			GDScript2ExecutionResult call_result = exec_call_builtin(p_frame, instr);
			if (call_result.has_error()) {
				r_result = call_result;
				return false;
			}
		} break;
		case GDScript2Opcode::OP_CALL_SUPER: {
			GDScript2ExecutionResult call_result = exec_call_super(p_frame, instr);
			if (call_result.has_error()) {
				r_result = call_result;
				return false;
			}
		} break;
		case GDScript2Opcode::OP_CALL_SELF: {
			GDScript2ExecutionResult call_result = exec_call_self(p_frame, instr);
			if (call_result.has_error()) {
				r_result = call_result;
				return false;
			}
		} break;

		// Return
		case GDScript2Opcode::OP_RETURN: {
			int32_t ret_reg = get_operand(instr, 0);
			r_result.return_value = get_stack_value_const(p_frame, ret_reg);
			return false;
		}
		case GDScript2Opcode::OP_RETURN_VOID:
			r_result.return_value = Variant();
			return false;

		// Iterators
		case GDScript2Opcode::OP_ITER_BEGIN:
			exec_iter_begin(p_frame, instr);
			break;
		case GDScript2Opcode::OP_ITER_NEXT:
			exec_iter_next(p_frame, instr);
			break;
		case GDScript2Opcode::OP_ITER_GET:
			exec_iter_get(p_frame, instr);
			break;
		case GDScript2Opcode::OP_JUMP_IF_ITER_END:
			increment_ip = !exec_jump_if_iter_end(p_frame, instr);
			break;

		// Special
		case GDScript2Opcode::OP_AWAIT:
			exec_await(p_frame, instr, r_result);
			if (r_result.status == GDScript2ExecutionResult::YIELD) {
				return false; // Suspend execution
			}
			break;
		case GDScript2Opcode::OP_YIELD:
			r_result.status = GDScript2ExecutionResult::YIELD;
			return false;

		// Signal operations
		case GDScript2Opcode::OP_SIGNAL_DEFINE:
			exec_signal_define(p_frame, instr);
			break;
		case GDScript2Opcode::OP_SIGNAL_CONNECT:
			exec_signal_connect(p_frame, instr, r_result);
			if (r_result.has_error()) {
				return false;
			}
			break;
		case GDScript2Opcode::OP_SIGNAL_DISCONNECT:
			exec_signal_disconnect(p_frame, instr);
			break;
		case GDScript2Opcode::OP_SIGNAL_EMIT:
			exec_signal_emit(p_frame, instr, r_result);
			if (r_result.has_error()) {
				return false;
			}
			break;
		case GDScript2Opcode::OP_SIGNAL_IS_CONNECTED:
			exec_signal_is_connected(p_frame, instr);
			break;
		case GDScript2Opcode::OP_MAKE_SIGNAL:
			exec_make_signal(p_frame, instr, r_result);
			if (r_result.has_error()) {
				return false;
			}
			break;

		case GDScript2Opcode::OP_PRELOAD:
			// Preload requires resource loading - simplified for now
			break;
		case GDScript2Opcode::OP_GET_NODE:
			// Get node requires scene tree context - simplified for now
			break;
		case GDScript2Opcode::OP_LAMBDA:
			// Lambda creation requires closure support - simplified for now
			break;

		case GDScript2Opcode::OP_COPY:
			exec_copy(p_frame, instr);
			break;

		case GDScript2Opcode::OP_ASSERT:
			if (!exec_assert(p_frame, instr, r_result)) {
				return false;
			}
			break;

		case GDScript2Opcode::OP_BREAKPOINT:
			exec_breakpoint(p_frame, instr);
			break;

		case GDScript2Opcode::OP_LINE:
			// Line marker - just for debugging
			break;

		default:
			r_result.status = GDScript2ExecutionResult::ERROR_INVALID_OPCODE;
			r_result.error_message = "Unknown opcode: " + itos((int)instr.op);
			return false;
	}

	if (increment_ip) {
		p_frame.ip++;
	}

	return true;
}

// ============================================================================
// Public API
// ============================================================================

GDScript2ExecutionResult GDScript2VM::call_internal(const GDScript2CompiledFunction *p_func, const Vector<Variant> &p_args, const Variant &p_self) {
	GDScript2ExecutionResult result;

	if (p_func == nullptr) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME, "Null function");
	}

	if (call_depth >= MAX_CALL_DEPTH) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_STACK_OVERFLOW, "Stack overflow");
	}

	// Setup call frame
	GDScript2CallFrame frame;
	frame.function = p_func;
	frame.ip = 0;
	frame.self = p_self;
	frame.ensure_stack_size(MAX(p_func->max_stack, DEFAULT_STACK_SIZE));

	// Copy arguments to stack
	for (int i = 0; i < p_args.size() && i < p_func->param_count; i++) {
		frame.stack.write[i] = p_args[i];
	}

	call_stack.push_back(frame);
	call_depth++;

	// Execute
	while (execute_instruction(call_stack.write[call_stack.size() - 1], result)) {
		// Continue execution
	}

	call_stack.resize(call_stack.size() - 1);
	call_depth--;

	return result;
}

GDScript2ExecutionResult GDScript2VM::call(const StringName &p_function, const Vector<Variant> &p_args) {
	if (module == nullptr) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME, "No module loaded");
	}

	const GDScript2CompiledFunction *func = find_function(p_function);
	if (func == nullptr) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
				"Function not found: " + String(p_function));
	}

	return call_internal(func, p_args);
}

GDScript2ExecutionResult GDScript2VM::call_with_self(const StringName &p_function, const Variant &p_self, const Vector<Variant> &p_args) {
	if (module == nullptr) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME, "No module loaded");
	}

	const GDScript2CompiledFunction *func = find_function(p_function);
	if (func == nullptr) {
		return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME,
				"Function not found: " + String(p_function));
	}

	return call_internal(func, p_args, p_self);
}

GDScript2ExecutionResult GDScript2VM::execute() {
	// Try common entry points
	if (find_function("_main")) {
		return call("_main");
	}
	if (find_function("_ready")) {
		return call("_ready");
	}
	if (find_function("_init")) {
		return call("_init");
	}

	// Execute first function if any
	if (module && !module->functions.is_empty()) {
		return call_internal(&module->functions[0], Vector<Variant>());
	}

	return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME, "No entry point found");
}
