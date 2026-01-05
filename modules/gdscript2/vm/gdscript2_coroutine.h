/**************************************************************************/
/*  gdscript2_coroutine.h                                                 */
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

#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/variant/variant.h"

// Forward declarations
struct GDScript2CallFrame;
class GDScript2VM;

// ============================================================================
// Coroutine State
// ============================================================================

class GDScript2Coroutine : public RefCounted {
	GDCLASS(GDScript2Coroutine, RefCounted);

public:
	enum State {
		STATE_SUSPENDED, // Coroutine is suspended, waiting to resume
		STATE_RUNNING, // Coroutine is currently running
		STATE_COMPLETED, // Coroutine has finished execution
		STATE_CANCELLED, // Coroutine was cancelled
		STATE_ERROR, // Coroutine encountered an error
	};

private:
	State state = STATE_SUSPENDED;
	GDScript2VM *vm = nullptr;

	// Saved execution state
	Vector<GDScript2CallFrame> saved_call_stack;
	int saved_call_depth = 0;

	// Resume value (value to return from await)
	Variant resume_value;

	// Error info
	String error_message;
	int error_line = 0;

	// Signal connection info (for await signal)
	Object *signal_target = nullptr;
	StringName signal_name;
	Callable signal_callback;
	bool waiting_for_signal = false;

	// Completion callback
	Callable completion_callback;

protected:
	static void _bind_methods();

public:
	GDScript2Coroutine();
	~GDScript2Coroutine();

	// State management
	State get_state() const { return state; }
	void set_state(State p_state) { state = p_state; }
	bool is_suspended() const { return state == STATE_SUSPENDED; }
	bool is_running() const { return state == STATE_RUNNING; }
	bool is_completed() const { return state == STATE_COMPLETED || state == STATE_CANCELLED || state == STATE_ERROR; }
	bool is_valid() const { return !is_completed(); }

	// VM management
	void set_vm(GDScript2VM *p_vm) { vm = p_vm; }
	GDScript2VM *get_vm() const { return vm; }

	// Execution state save/restore
	void save_state(const Vector<GDScript2CallFrame> &p_call_stack, int p_call_depth);
	void restore_state(Vector<GDScript2CallFrame> &r_call_stack, int &r_call_depth);
	bool has_saved_state() const { return !saved_call_stack.is_empty(); }

	// Resume control
	void resume(const Variant &p_value = Variant());
	void set_resume_value(const Variant &p_value) { resume_value = p_value; }
	Variant get_resume_value() const { return resume_value; }

	// Signal waiting
	void wait_for_signal(Object *p_target, const StringName &p_signal);
	void cancel_signal_wait();
	bool is_waiting_for_signal() const { return waiting_for_signal; }
	Object *get_signal_target() const { return signal_target; }
	StringName get_signal_name() const { return signal_name; }

	// Completion
	void complete(const Variant &p_return_value = Variant());
	void cancel();
	void set_error(const String &p_message, int p_line = 0);
	String get_error_message() const { return error_message; }
	int get_error_line() const { return error_line; }

	// Callback
	void set_completion_callback(const Callable &p_callback) { completion_callback = p_callback; }
	Callable get_completion_callback() const { return completion_callback; }

	// Signal handler for await signal
	void _on_signal_received();
};

// ============================================================================
// Coroutine Manager
// ============================================================================

class GDScript2CoroutineManager : public RefCounted {
	GDCLASS(GDScript2CoroutineManager, RefCounted);

private:
	Vector<Ref<GDScript2Coroutine>> active_coroutines;
	GDScript2VM *vm = nullptr;

protected:
	static void _bind_methods() {}

public:
	void set_vm(GDScript2VM *p_vm) { vm = p_vm; }
	GDScript2VM *get_vm() const { return vm; }

	// Coroutine lifecycle
	Ref<GDScript2Coroutine> create_coroutine();
	void register_coroutine(const Ref<GDScript2Coroutine> &p_coroutine);
	void unregister_coroutine(const Ref<GDScript2Coroutine> &p_coroutine);

	// Query
	int get_active_count() const { return active_coroutines.size(); }
	Ref<GDScript2Coroutine> get_coroutine(int p_index) const;

	// Cleanup
	void cancel_all();
	void cleanup_completed();
	void clear();
};

VARIANT_ENUM_CAST(GDScript2Coroutine::State);
