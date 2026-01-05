/**************************************************************************/
/*  gdscript2_signal.cpp                                                  */
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

#include "gdscript2_signal.h"

// ============================================================================
// GDScript2SignalRegistry - Binding
// ============================================================================

void GDScript2SignalRegistry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("define_signal", "signal_name"), &GDScript2SignalRegistry::define_signal);
	// Note: has_signal, get_signal_list, is_connected are inherited from Object and should not be bound again
	ClassDB::bind_method(D_METHOD("disconnect_all", "signal_name"), &GDScript2SignalRegistry::disconnect_all);
	ClassDB::bind_method(D_METHOD("clear_all_connections"), &GDScript2SignalRegistry::clear_all_connections);
}

// ============================================================================
// GDScript2SignalRegistry - Signal Definition
// ============================================================================

void GDScript2SignalRegistry::define_signal(const StringName &p_signal_name) {
	if (has_signal(p_signal_name)) {
		ERR_PRINT(vformat("Signal '%s' is already defined.", p_signal_name));
		return;
	}

	GDScript2SignalDefinition def(p_signal_name, owner);
	signals[p_signal_name] = def;
}

void GDScript2SignalRegistry::define_signal_with_params(const StringName &p_signal_name, const Vector<GDScript2SignalParam> &p_params) {
	if (has_signal(p_signal_name)) {
		ERR_PRINT(vformat("Signal '%s' is already defined.", p_signal_name));
		return;
	}

	GDScript2SignalDefinition def(p_signal_name, owner);
	def.parameters = p_params;
	signals[p_signal_name] = def;
}

bool GDScript2SignalRegistry::has_signal(const StringName &p_signal_name) const {
	return signals.has(p_signal_name);
}

GDScript2SignalDefinition GDScript2SignalRegistry::get_signal_definition(const StringName &p_signal_name) const {
	if (signals.has(p_signal_name)) {
		return signals[p_signal_name];
	}
	return GDScript2SignalDefinition();
}

Array GDScript2SignalRegistry::get_signal_list() const {
	Array list;
	for (const KeyValue<StringName, GDScript2SignalDefinition> &E : signals) {
		list.push_back(E.key);
	}
	return list;
}

// ============================================================================
// GDScript2SignalRegistry - Connection Management
// ============================================================================

Error GDScript2SignalRegistry::connect_signal(const StringName &p_signal_name, const Callable &p_callable, uint32_t p_flags) {
	if (!has_signal(p_signal_name)) {
		ERR_PRINT(vformat("Signal '%s' is not defined.", p_signal_name));
		return ERR_DOES_NOT_EXIST;
	}

	if (!p_callable.is_valid()) {
		ERR_PRINT("Invalid callable.");
		return ERR_INVALID_PARAMETER;
	}

	// Check if already connected
	if (is_connected(p_signal_name, p_callable)) {
		return ERR_ALREADY_EXISTS;
	}

	// Create connection
	GDScript2SignalConnection conn(p_callable.get_object(), p_callable, p_flags);

	// Add to connections list
	if (!connections.has(p_signal_name)) {
		connections[p_signal_name] = Vector<GDScript2SignalConnection>();
	}
	connections[p_signal_name].push_back(conn);

	return OK;
}

Error GDScript2SignalRegistry::connect_signal_to_object(const StringName &p_signal_name, Object *p_target, const StringName &p_method, uint32_t p_flags) {
	if (!p_target) {
		ERR_PRINT("Target object is null.");
		return ERR_INVALID_PARAMETER;
	}

	Callable callable = Callable(p_target, p_method);
	return connect_signal(p_signal_name, callable, p_flags);
}

void GDScript2SignalRegistry::disconnect_signal(const StringName &p_signal_name, const Callable &p_callable) {
	if (!connections.has(p_signal_name)) {
		return;
	}

	Vector<GDScript2SignalConnection> &conns = connections[p_signal_name];
	for (int i = conns.size() - 1; i >= 0; i--) {
		if (conns[i].callable == p_callable) {
			conns.remove_at(i);
			break;
		}
	}

	// Clean up empty connection lists
	if (conns.is_empty()) {
		connections.erase(p_signal_name);
	}
}

void GDScript2SignalRegistry::disconnect_all(const StringName &p_signal_name) {
	if (connections.has(p_signal_name)) {
		connections.erase(p_signal_name);
	}
}

bool GDScript2SignalRegistry::is_connected(const StringName &p_signal_name, const Callable &p_callable) const {
	if (!connections.has(p_signal_name)) {
		return false;
	}

	const Vector<GDScript2SignalConnection> &conns = connections[p_signal_name];
	for (const GDScript2SignalConnection &conn : conns) {
		if (conn.callable == p_callable) {
			return true;
		}
	}
	return false;
}

int GDScript2SignalRegistry::get_connection_count(const StringName &p_signal_name) const {
	if (!connections.has(p_signal_name)) {
		return 0;
	}
	return connections[p_signal_name].size();
}

Vector<Callable> GDScript2SignalRegistry::get_connections(const StringName &p_signal_name) const {
	Vector<Callable> result;
	if (!connections.has(p_signal_name)) {
		return result;
	}

	const Vector<GDScript2SignalConnection> &conns = connections[p_signal_name];
	for (const GDScript2SignalConnection &conn : conns) {
		if (conn.is_valid()) {
			result.push_back(conn.callable);
		}
	}
	return result;
}

// ============================================================================
// GDScript2SignalRegistry - Signal Emission
// ============================================================================

void GDScript2SignalRegistry::emit_signal(const StringName &p_signal_name, const Variant **p_args, int p_argcount) {
	if (!has_signal(p_signal_name)) {
		ERR_PRINT(vformat("Signal '%s' is not defined.", p_signal_name));
		return;
	}

	if (!connections.has(p_signal_name)) {
		return; // No connections, nothing to do
	}

	// Get a copy of connections to avoid issues with modifications during emission
	Vector<GDScript2SignalConnection> conns = connections[p_signal_name];

	// Call each connected callable
	for (int i = 0; i < conns.size(); i++) {
		GDScript2SignalConnection &conn = conns.write[i];

		if (!conn.is_valid()) {
			continue;
		}

		// Check if target still exists
		if (conn.target && !ObjectDB::get_instance(conn.target->get_instance_id())) {
			conn.invalidate();
			continue;
		}

		// Call the callable
		Variant ret;
		Callable::CallError ce;
		conn.callable.callp(p_args, p_argcount, ret, ce);

		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Error calling signal '%s' connection: %d", p_signal_name, ce.error));
		}

		// Check for one-shot flag
		if (conn.flags & Object::CONNECT_ONE_SHOT) {
			conn.invalidate();
		}
	}

	// Clean up invalid connections
	if (connections.has(p_signal_name)) {
		Vector<GDScript2SignalConnection> &stored_conns = connections[p_signal_name];
		for (int i = stored_conns.size() - 1; i >= 0; i--) {
			if (!stored_conns[i].is_valid()) {
				stored_conns.remove_at(i);
			}
		}
	}
}

void GDScript2SignalRegistry::emit_signal(const StringName &p_signal_name, const Vector<Variant> &p_args) {
	LocalVector<const Variant *> args;
	args.resize(p_args.size());
	for (int i = 0; i < p_args.size(); i++) {
		args[i] = &p_args[i];
	}
	emit_signal(p_signal_name, args.ptr(), args.size());
}

void GDScript2SignalRegistry::emit_signal(const StringName &p_signal_name) {
	emit_signal(p_signal_name, nullptr, 0);
}

// ============================================================================
// GDScript2SignalRegistry - Cleanup
// ============================================================================

void GDScript2SignalRegistry::clear_all_connections() {
	connections.clear();
}

void GDScript2SignalRegistry::clear() {
	signals.clear();
	connections.clear();
	owner = nullptr;
}

// ============================================================================
// GDScript2SignalEmitter - Binding
// ============================================================================

void GDScript2SignalEmitter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("connect_to_signal", "signal_name", "callable", "flags"), &GDScript2SignalEmitter::connect_to_signal, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("disconnect_from_signal", "signal_name", "callable"), &GDScript2SignalEmitter::disconnect_from_signal);
	ClassDB::bind_method(D_METHOD("emit", "signal_name", "args"), &GDScript2SignalEmitter::emit, DEFVAL(Vector<Variant>()));
	// Note: has_signal and is_connected are inherited from Object and should not be bound again
}

// ============================================================================
// GDScript2SignalEmitter - Implementation
// ============================================================================

GDScript2SignalEmitter::GDScript2SignalEmitter() {
	signal_registry.instantiate();
	signal_registry->set_owner(this);
}

void GDScript2SignalEmitter::_define_signal(const StringName &p_signal_name) {
	if (signal_registry.is_valid()) {
		signal_registry->define_signal(p_signal_name);
	}
}

void GDScript2SignalEmitter::_define_signal_with_params(const StringName &p_signal_name, const Vector<String> &p_param_names) {
	if (signal_registry.is_valid()) {
		Vector<GDScript2SignalParam> params;
		for (const String &name : p_param_names) {
			params.push_back(GDScript2SignalParam(StringName(name)));
		}
		signal_registry->define_signal_with_params(p_signal_name, params);
	}
}

Error GDScript2SignalEmitter::connect_to_signal(const StringName &p_signal_name, const Callable &p_callable, uint32_t p_flags) {
	if (signal_registry.is_valid()) {
		return signal_registry->connect_signal(p_signal_name, p_callable, p_flags);
	}
	return ERR_UNAVAILABLE;
}

void GDScript2SignalEmitter::disconnect_from_signal(const StringName &p_signal_name, const Callable &p_callable) {
	if (signal_registry.is_valid()) {
		signal_registry->disconnect_signal(p_signal_name, p_callable);
	}
}

void GDScript2SignalEmitter::emit(const StringName &p_signal_name, const Vector<Variant> &p_args) {
	if (signal_registry.is_valid()) {
		signal_registry->emit_signal(p_signal_name, p_args);
	}
}

bool GDScript2SignalEmitter::has_signal(const StringName &p_signal_name) const {
	if (signal_registry.is_valid()) {
		return signal_registry->has_signal(p_signal_name);
	}
	return false;
}

bool GDScript2SignalEmitter::is_connected(const StringName &p_signal_name, const Callable &p_callable) const {
	if (signal_registry.is_valid()) {
		return signal_registry->is_connected(p_signal_name, p_callable);
	}
	return false;
}

// ============================================================================
// GDScript2SignalUtils
// ============================================================================

Signal GDScript2SignalUtils::make_signal(Object *p_object, const StringName &p_signal_name) {
	if (!p_object) {
		return Signal();
	}
	return Signal(p_object, p_signal_name);
}

bool GDScript2SignalUtils::is_signal(const Variant &p_variant) {
	return p_variant.get_type() == Variant::SIGNAL;
}

Error GDScript2SignalUtils::safe_connect(const Signal &p_signal, const Callable &p_callable, uint32_t p_flags) {
	Object *obj = p_signal.get_object();
	if (!obj) {
		return ERR_INVALID_PARAMETER;
	}

	StringName signal_name = p_signal.get_name();
	if (!obj->has_signal(signal_name)) {
		// Try to add user signal
		obj->add_user_signal(MethodInfo(signal_name));
	}

	return obj->connect(signal_name, p_callable, p_flags);
}

Error GDScript2SignalUtils::safe_disconnect(const Signal &p_signal, const Callable &p_callable) {
	Object *obj = p_signal.get_object();
	if (!obj) {
		return ERR_INVALID_PARAMETER;
	}

	StringName signal_name = p_signal.get_name();
	if (!obj->is_connected(signal_name, p_callable)) {
		return ERR_DOES_NOT_EXIST;
	}

	obj->disconnect(signal_name, p_callable);
	return OK;
}

Error GDScript2SignalUtils::safe_emit(const Signal &p_signal, const Variant **p_args, int p_argcount) {
	Object *obj = p_signal.get_object();
	if (!obj) {
		return ERR_INVALID_PARAMETER;
	}

	StringName signal_name = p_signal.get_name();
	if (!obj->has_signal(signal_name)) {
		return ERR_DOES_NOT_EXIST;
	}

	obj->emit_signalp(signal_name, p_args, p_argcount);
	return OK;
}
