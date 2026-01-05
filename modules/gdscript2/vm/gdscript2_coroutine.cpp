/**************************************************************************/
/*  gdscript2_coroutine.cpp                                               */
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

#include "gdscript2_coroutine.h"

#include "gdscript2_vm.h"

// ============================================================================
// GDScript2Coroutine - Binding
// ============================================================================

void GDScript2Coroutine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_valid"), &GDScript2Coroutine::is_valid);
	ClassDB::bind_method(D_METHOD("resume", "value"), &GDScript2Coroutine::resume, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("cancel"), &GDScript2Coroutine::cancel);
	ClassDB::bind_method(D_METHOD("get_state"), &GDScript2Coroutine::get_state);

	BIND_ENUM_CONSTANT(STATE_SUSPENDED);
	BIND_ENUM_CONSTANT(STATE_RUNNING);
	BIND_ENUM_CONSTANT(STATE_COMPLETED);
	BIND_ENUM_CONSTANT(STATE_CANCELLED);
	BIND_ENUM_CONSTANT(STATE_ERROR);
}

// ============================================================================
// GDScript2Coroutine - Constructor/Destructor
// ============================================================================

GDScript2Coroutine::GDScript2Coroutine() {
	state = STATE_SUSPENDED;
}

GDScript2Coroutine::~GDScript2Coroutine() {
	// Clean up signal connection if any
	cancel_signal_wait();
}

// ============================================================================
// GDScript2Coroutine - State Management
// ============================================================================

void GDScript2Coroutine::save_state(const Vector<GDScript2CallFrame> &p_call_stack, int p_call_depth) {
	saved_call_stack = p_call_stack;
	saved_call_depth = p_call_depth;
	state = STATE_SUSPENDED;
}

void GDScript2Coroutine::restore_state(Vector<GDScript2CallFrame> &r_call_stack, int &r_call_depth) {
	r_call_stack = saved_call_stack;
	r_call_depth = saved_call_depth;
	state = STATE_RUNNING;
	// Clear saved state to prevent double restoration
	saved_call_stack.clear();
	saved_call_depth = 0;
}

// ============================================================================
// GDScript2Coroutine - Resume Control
// ============================================================================

void GDScript2Coroutine::resume(const Variant &p_value) {
	if (!is_valid()) {
		ERR_PRINT("Cannot resume completed or invalid coroutine.");
		return;
	}

	if (!vm) {
		ERR_PRINT("Coroutine has no associated VM.");
		return;
	}

	resume_value = p_value;

	// Cancel any signal waiting
	cancel_signal_wait();

	// Restore execution state in VM and continue
	if (!saved_call_stack.is_empty()) {
		vm->resume_coroutine(this);
	}
}

// ============================================================================
// GDScript2Coroutine - Signal Waiting
// ============================================================================

void GDScript2Coroutine::wait_for_signal(Object *p_target, const StringName &p_signal) {
	if (!p_target) {
		ERR_PRINT("Cannot wait for signal on null object.");
		return;
	}

	// Cancel previous signal wait if any
	cancel_signal_wait();

	signal_target = p_target;
	signal_name = p_signal;
	waiting_for_signal = true;

	// Create callable to handle signal
	signal_callback = callable_mp(this, &GDScript2Coroutine::_on_signal_received);

	// Connect to the signal
	if (p_target->has_signal(p_signal)) {
		Error err = p_target->connect(p_signal, signal_callback);
		if (err != OK) {
			ERR_PRINT(vformat("Failed to connect to signal '%s' on object.", p_signal));
			waiting_for_signal = false;
			signal_target = nullptr;
			signal_name = StringName();
		}
	} else {
		ERR_PRINT(vformat("Object does not have signal '%s'.", p_signal));
		waiting_for_signal = false;
		signal_target = nullptr;
		signal_name = StringName();
	}
}

void GDScript2Coroutine::cancel_signal_wait() {
	if (waiting_for_signal && signal_target && signal_target->is_connected(signal_name, signal_callback)) {
		signal_target->disconnect(signal_name, signal_callback);
	}
	waiting_for_signal = false;
	signal_target = nullptr;
	signal_name = StringName();
	signal_callback = Callable();
}

void GDScript2Coroutine::_on_signal_received() {
	if (!is_valid()) {
		return;
	}

	// Prepare resume value - signal received with no specific value
	Variant resume_val;

	// Cancel the signal wait
	waiting_for_signal = false;
	signal_target = nullptr;
	signal_name = StringName();
	signal_callback = Callable();

	// Resume the coroutine with the signal value
	resume(resume_val);
}

// ============================================================================
// GDScript2Coroutine - Completion
// ============================================================================

void GDScript2Coroutine::complete(const Variant &p_return_value) {
	if (state == STATE_COMPLETED) {
		return;
	}

	state = STATE_COMPLETED;
	resume_value = p_return_value;

	// Clean up
	cancel_signal_wait();
	saved_call_stack.clear();
	saved_call_depth = 0;

	// Call completion callback if set
	if (completion_callback.is_valid()) {
		const Variant *args[1] = { &p_return_value };
		Variant ret;
		Callable::CallError ce;
		completion_callback.callp(args, 1, ret, ce);
	}
}

void GDScript2Coroutine::cancel() {
	if (is_completed()) {
		return;
	}

	state = STATE_CANCELLED;

	// Clean up
	cancel_signal_wait();
	saved_call_stack.clear();
	saved_call_depth = 0;

	// Call completion callback with null
	if (completion_callback.is_valid()) {
		Variant null_value;
		const Variant *args[1] = { &null_value };
		Variant ret;
		Callable::CallError ce;
		completion_callback.callp(args, 1, ret, ce);
	}
}

void GDScript2Coroutine::set_error(const String &p_message, int p_line) {
	if (is_completed()) {
		return;
	}

	state = STATE_ERROR;
	error_message = p_message;
	error_line = p_line;

	// Clean up
	cancel_signal_wait();
	saved_call_stack.clear();
	saved_call_depth = 0;

	// Don't call completion callback on error - let error propagate
}

// ============================================================================
// GDScript2CoroutineManager
// ============================================================================

Ref<GDScript2Coroutine> GDScript2CoroutineManager::create_coroutine() {
	Ref<GDScript2Coroutine> coroutine;
	coroutine.instantiate();
	coroutine->set_vm(vm);
	register_coroutine(coroutine);
	return coroutine;
}

void GDScript2CoroutineManager::register_coroutine(const Ref<GDScript2Coroutine> &p_coroutine) {
	if (p_coroutine.is_valid() && !active_coroutines.has(p_coroutine)) {
		active_coroutines.push_back(p_coroutine);
	}
}

void GDScript2CoroutineManager::unregister_coroutine(const Ref<GDScript2Coroutine> &p_coroutine) {
	active_coroutines.erase(p_coroutine);
}

Ref<GDScript2Coroutine> GDScript2CoroutineManager::get_coroutine(int p_index) const {
	if (p_index >= 0 && p_index < active_coroutines.size()) {
		return active_coroutines[p_index];
	}
	return Ref<GDScript2Coroutine>();
}

void GDScript2CoroutineManager::cancel_all() {
	for (int i = 0; i < active_coroutines.size(); i++) {
		if (active_coroutines[i].is_valid()) {
			active_coroutines[i]->cancel();
		}
	}
	active_coroutines.clear();
}

void GDScript2CoroutineManager::cleanup_completed() {
	for (int i = active_coroutines.size() - 1; i >= 0; i--) {
		if (!active_coroutines[i].is_valid() || active_coroutines[i]->is_completed()) {
			active_coroutines.remove_at(i);
		}
	}
}

void GDScript2CoroutineManager::clear() {
	active_coroutines.clear();
}
