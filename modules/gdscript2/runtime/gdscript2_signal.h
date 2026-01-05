/**************************************************************************/
/*  gdscript2_signal.h                                                    */
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

// ============================================================================
// GDScript2 Signal System
// ============================================================================

// Signal parameter definition
struct GDScript2SignalParam {
	StringName name;
	Variant::Type type = Variant::NIL;

	GDScript2SignalParam() {}
	GDScript2SignalParam(const StringName &p_name, Variant::Type p_type = Variant::NIL) :
			name(p_name), type(p_type) {}
};

// Signal definition (metadata)
struct GDScript2SignalDefinition {
	StringName name;
	Vector<GDScript2SignalParam> parameters;
	Object *owner = nullptr; // The object that owns this signal

	GDScript2SignalDefinition() {}
	GDScript2SignalDefinition(const StringName &p_name, Object *p_owner = nullptr) :
			name(p_name), owner(p_owner) {}

	void add_parameter(const StringName &p_name, Variant::Type p_type = Variant::NIL) {
		parameters.push_back(GDScript2SignalParam(p_name, p_type));
	}

	int get_parameter_count() const { return parameters.size(); }
};

// Signal connection info
struct GDScript2SignalConnection {
	Object *target = nullptr;
	Callable callable;
	uint32_t flags = 0;
	bool valid = false;

	GDScript2SignalConnection() {}
	GDScript2SignalConnection(Object *p_target, const Callable &p_callable, uint32_t p_flags = 0) :
			target(p_target), callable(p_callable), flags(p_flags), valid(true) {}

	bool is_valid() const { return valid && target != nullptr && callable.is_valid(); }
	void invalidate() { valid = false; }
};

// Signal registry for GDScript2 objects
class GDScript2SignalRegistry : public RefCounted {
	GDCLASS(GDScript2SignalRegistry, RefCounted);

private:
	// Signal definitions: signal_name -> definition
	HashMap<StringName, GDScript2SignalDefinition> signals;

	// Signal connections: signal_name -> list of connections
	HashMap<StringName, Vector<GDScript2SignalConnection>> connections;

	// Owner object (optional)
	Object *owner = nullptr;

protected:
	static void _bind_methods();

public:
	GDScript2SignalRegistry() {}
	explicit GDScript2SignalRegistry(Object *p_owner) : owner(p_owner) {}

	// Signal definition
	void define_signal(const StringName &p_signal_name);
	void define_signal_with_params(const StringName &p_signal_name, const Vector<GDScript2SignalParam> &p_params);
	bool has_signal(const StringName &p_signal_name) const;
	GDScript2SignalDefinition get_signal_definition(const StringName &p_signal_name) const;
	Vector<StringName> get_signal_list() const;

	// Connection management
	Error connect_signal(const StringName &p_signal_name, const Callable &p_callable, uint32_t p_flags = 0);
	Error connect_signal_to_object(const StringName &p_signal_name, Object *p_target, const StringName &p_method, uint32_t p_flags = 0);
	void disconnect_signal(const StringName &p_signal_name, const Callable &p_callable);
	void disconnect_all(const StringName &p_signal_name);
	bool is_connected(const StringName &p_signal_name, const Callable &p_callable) const;
	int get_connection_count(const StringName &p_signal_name) const;
	Vector<Callable> get_connections(const StringName &p_signal_name) const;

	// Signal emission
	void emit_signal(const StringName &p_signal_name, const Variant **p_args, int p_argcount);
	void emit_signal(const StringName &p_signal_name, const Vector<Variant> &p_args);
	void emit_signal(const StringName &p_signal_name); // No arguments

	// Owner management
	void set_owner(Object *p_owner) { owner = p_owner; }
	Object *get_owner() const { return owner; }

	// Cleanup
	void clear_all_connections();
	void clear();
};

// Helper class for GDScript2 objects with signals
class GDScript2SignalEmitter : public RefCounted {
	GDCLASS(GDScript2SignalEmitter, RefCounted);

private:
	Ref<GDScript2SignalRegistry> signal_registry;

protected:
	static void _bind_methods();

public:
	GDScript2SignalEmitter();
	virtual ~GDScript2SignalEmitter() {}

	// Signal definition (usually called during initialization)
	void _define_signal(const StringName &p_signal_name);
	void _define_signal_with_params(const StringName &p_signal_name, const Vector<String> &p_param_names);

	// Signal operations (exposed to GDScript)
	Error connect_to_signal(const StringName &p_signal_name, const Callable &p_callable, uint32_t p_flags = 0);
	void disconnect_from_signal(const StringName &p_signal_name, const Callable &p_callable);
	void emit(const StringName &p_signal_name, const Vector<Variant> &p_args = Vector<Variant>());
	bool has_signal(const StringName &p_signal_name) const;
	bool is_connected(const StringName &p_signal_name, const Callable &p_callable) const;

	// Get registry
	Ref<GDScript2SignalRegistry> get_signal_registry() const { return signal_registry; }
};

// Utility functions for signal operations
namespace GDScript2SignalUtils {

// Create a Signal variant from name and object
Signal make_signal(Object *p_object, const StringName &p_signal_name);

// Check if a variant is a valid signal
bool is_signal(const Variant &p_variant);

// Connect signal with error checking
Error safe_connect(const Signal &p_signal, const Callable &p_callable, uint32_t p_flags = 0);

// Disconnect signal with error checking
Error safe_disconnect(const Signal &p_signal, const Callable &p_callable);

// Emit signal with error checking
Error safe_emit(const Signal &p_signal, const Variant **p_args, int p_argcount);

} // namespace GDScript2SignalUtils

#endif // GDSCRIPT2_SIGNAL_H
