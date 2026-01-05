/**************************************************************************/
/*  gdscript2_symbol_table.cpp                                            */
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

#include "gdscript2_symbol_table.h"

#include "core/object/class_db.h"
#include "core/variant/variant.h"
#include "modules/gdscript2/front/gdscript2_ast.h"

// ============================================================================
// GDScript2SymbolTable - Constructor/Destructor
// ============================================================================

GDScript2SymbolTable::GDScript2SymbolTable() {
	// Create global scope
	global_scope = memnew(GDScript2Scope(GDScript2ScopeKind::SCOPE_GLOBAL));
	all_scopes.push_back(global_scope);
	current_scope = global_scope;

	// Initialize built-in symbols
	initialize_builtins();
}

GDScript2SymbolTable::~GDScript2SymbolTable() {
	for (GDScript2Scope *scope : all_scopes) {
		memdelete(scope);
	}
}

// ============================================================================
// GDScript2SymbolTable - Built-in Initialization
// ============================================================================

void GDScript2SymbolTable::initialize_builtins() {
	// Built-in type names
	static const char *builtin_types[] = {
		"bool", "int", "float", "String", "StringName", "NodePath",
		"Vector2", "Vector2i", "Vector3", "Vector3i", "Vector4", "Vector4i",
		"Rect2", "Rect2i", "Transform2D", "Plane", "Quaternion",
		"AABB", "Basis", "Transform3D", "Projection",
		"Color", "RID", "Callable", "Signal",
		"Array", "Dictionary",
		"PackedByteArray", "PackedInt32Array", "PackedInt64Array",
		"PackedFloat32Array", "PackedFloat64Array", "PackedStringArray",
		"PackedVector2Array", "PackedVector3Array", "PackedVector4Array", "PackedColorArray",
		nullptr
	};

	for (int i = 0; builtin_types[i]; i++) {
		GDScript2Symbol *sym = memnew(GDScript2Symbol);
		sym->name = builtin_types[i];
		sym->kind = GDScript2SymbolKind::SYMBOL_BUILTIN_TYPE;
		sym->type = GDScript2Type::make_metatype(nullptr); // Type of type
		global_scope->add_symbol(sym);
	}

	// Built-in constants
	struct BuiltinConst {
		const char *name;
		GDScript2TypeKind type_kind;
	};
	static const BuiltinConst builtin_consts[] = {
		{ "PI", GDScript2TypeKind::TYPE_FLOAT },
		{ "TAU", GDScript2TypeKind::TYPE_FLOAT },
		{ "INF", GDScript2TypeKind::TYPE_FLOAT },
		{ "NAN", GDScript2TypeKind::TYPE_FLOAT },
		{ nullptr, GDScript2TypeKind::TYPE_UNKNOWN }
	};

	for (int i = 0; builtin_consts[i].name; i++) {
		GDScript2Symbol *sym = memnew(GDScript2Symbol);
		sym->name = builtin_consts[i].name;
		sym->kind = GDScript2SymbolKind::SYMBOL_CONSTANT;
		sym->type = GDScript2Type(builtin_consts[i].type_kind);
		sym->type.is_constant = true;
		global_scope->add_symbol(sym);
	}

	// Built-in functions
	static const char *builtin_funcs[] = {
		// Math functions
		"abs", "absf", "absi", "acos", "acosh", "angle_difference", "asin", "asinh",
		"atan", "atan2", "atanh", "bezier_derivative", "bezier_interpolate",
		"ceil", "ceilf", "ceili", "clamp", "clampf", "clampi", "cos", "cosh",
		"cubic_interpolate", "cubic_interpolate_angle", "cubic_interpolate_angle_in_time",
		"cubic_interpolate_in_time", "db_to_linear", "deg_to_rad", "ease",
		"exp", "floor", "floorf", "floori", "fmod", "fposmod", "hash",
		"inverse_lerp", "is_equal_approx", "is_finite", "is_inf", "is_nan",
		"is_zero_approx", "lerp", "lerp_angle", "lerpf", "linear_to_db", "log",
		"max", "maxf", "maxi", "min", "minf", "mini", "move_toward",
		"nearest_po2", "pingpong", "posmod", "pow", "rad_to_deg", "randf",
		"randf_range", "randfn", "randi", "randi_range", "randomize",
		"remap", "rotate_toward", "round", "roundf", "roundi", "seed",
		"sign", "signf", "signi", "sin", "sinh", "smoothstep", "snapped",
		"snappedf", "snappedi", "sqrt", "step_decimals", "tan", "tanh",
		"wrap", "wrapf", "wrapi",

		// Type conversion
		"type_convert",

		// String functions
		"str", "str_to_var", "var_to_str", "var_to_bytes", "bytes_to_var",
		"var_to_bytes_with_objects", "bytes_to_var_with_objects",

		// Object functions
		"instance_from_id", "is_instance_id_valid", "is_instance_valid",

		// Utility
		"print", "print_rich", "print_verbose", "printerr", "printraw", "prints", "printt",
		"push_error", "push_warning",
		"range", "len", "typeof", "weakref",
		"is_same",

		// Resource
		"load", "preload",

		nullptr
	};

	for (int i = 0; builtin_funcs[i]; i++) {
		GDScript2Symbol *sym = memnew(GDScript2Symbol);
		sym->name = builtin_funcs[i];
		sym->kind = GDScript2SymbolKind::SYMBOL_BUILTIN_FUNC;
		sym->type = GDScript2Type::make_callable();
		global_scope->add_symbol(sym);
	}

	// Register native classes from ClassDB
	List<StringName> class_list;
	ClassDB::get_class_list(&class_list);

	for (const StringName &class_name : class_list) {
		// Skip internal classes
		if (String(class_name).begins_with("_")) {
			continue;
		}

		GDScript2Symbol *sym = memnew(GDScript2Symbol);
		sym->name = class_name;
		sym->kind = GDScript2SymbolKind::SYMBOL_NATIVE_CLASS;
		sym->type = GDScript2Type::make_object(class_name);
		sym->type.kind = GDScript2TypeKind::TYPE_METATYPE; // It's a type, not an instance
		global_scope->add_symbol(sym);
	}
}

// ============================================================================
// GDScript2SymbolTable - Scope Management
// ============================================================================

GDScript2Scope *GDScript2SymbolTable::push_scope(GDScript2ScopeKind p_kind) {
	GDScript2Scope *new_scope = memnew(GDScript2Scope(p_kind));
	new_scope->parent = current_scope;

	// Inherit loop depth
	if (current_scope) {
		new_scope->loop_depth = current_scope->loop_depth;
	}

	all_scopes.push_back(new_scope);
	current_scope = new_scope;

	return new_scope;
}

void GDScript2SymbolTable::pop_scope() {
	if (current_scope && current_scope->parent) {
		current_scope = current_scope->parent;
	}
}

// ============================================================================
// GDScript2SymbolTable - Class Scope Management
// ============================================================================

void GDScript2SymbolTable::enter_class(GDScript2ClassNode *p_class) {
	GDScript2Scope *class_scope = push_scope(GDScript2ScopeKind::SCOPE_CLASS);
	class_scope->class_node = p_class;
	class_scope_stack.push_back(class_scope);
}

void GDScript2SymbolTable::exit_class() {
	if (!class_scope_stack.is_empty()) {
		class_scope_stack.resize(class_scope_stack.size() - 1);
	}
	pop_scope();
}

GDScript2Scope *GDScript2SymbolTable::get_current_class_scope() const {
	if (class_scope_stack.is_empty()) {
		return nullptr;
	}
	return class_scope_stack[class_scope_stack.size() - 1];
}

GDScript2ClassNode *GDScript2SymbolTable::get_current_class() const {
	GDScript2Scope *class_scope = get_current_class_scope();
	return class_scope ? class_scope->class_node : nullptr;
}

// ============================================================================
// GDScript2SymbolTable - Function Scope Management
// ============================================================================

void GDScript2SymbolTable::enter_function(GDScript2FunctionNode *p_func) {
	GDScript2Scope *func_scope = push_scope(GDScript2ScopeKind::SCOPE_FUNCTION);
	func_scope->function_node = p_func;
	function_scope_stack.push_back(func_scope);
}

void GDScript2SymbolTable::exit_function() {
	if (!function_scope_stack.is_empty()) {
		function_scope_stack.resize(function_scope_stack.size() - 1);
	}
	pop_scope();
}

GDScript2Scope *GDScript2SymbolTable::get_current_function_scope() const {
	if (function_scope_stack.is_empty()) {
		return nullptr;
	}
	return function_scope_stack[function_scope_stack.size() - 1];
}

GDScript2FunctionNode *GDScript2SymbolTable::get_current_function() const {
	GDScript2Scope *func_scope = get_current_function_scope();
	return func_scope ? func_scope->function_node : nullptr;
}

// ============================================================================
// GDScript2SymbolTable - Loop Tracking
// ============================================================================

void GDScript2SymbolTable::enter_loop() {
	if (current_scope) {
		current_scope->loop_depth++;
	}
}

void GDScript2SymbolTable::exit_loop() {
	if (current_scope && current_scope->loop_depth > 0) {
		current_scope->loop_depth--;
	}
}

bool GDScript2SymbolTable::is_in_loop() const {
	return current_scope && current_scope->loop_depth > 0;
}

// ============================================================================
// GDScript2SymbolTable - Symbol Operations
// ============================================================================

bool GDScript2SymbolTable::declare(GDScript2Symbol *p_symbol) {
	if (!current_scope || !p_symbol) {
		return false;
	}

	// Check if already declared in current scope
	if (current_scope->has_local(p_symbol->name)) {
		return false;
	}

	current_scope->add_symbol(p_symbol);
	return true;
}

GDScript2Symbol *GDScript2SymbolTable::lookup(const StringName &p_name) const {
	// Search from current scope up to global scope
	GDScript2Scope *scope = current_scope;
	while (scope) {
		GDScript2Symbol *sym = scope->get_local(p_name);
		if (sym) {
			return sym;
		}
		scope = scope->parent;
	}
	return nullptr;
}

GDScript2Symbol *GDScript2SymbolTable::lookup_local(const StringName &p_name) const {
	if (!current_scope) {
		return nullptr;
	}
	return current_scope->get_local(p_name);
}

GDScript2Symbol *GDScript2SymbolTable::lookup_in_class(const StringName &p_name) const {
	GDScript2Scope *class_scope = get_current_class_scope();
	if (!class_scope) {
		return nullptr;
	}
	return class_scope->get_local(p_name);
}

bool GDScript2SymbolTable::is_declared_local(const StringName &p_name) const {
	return current_scope && current_scope->has_local(p_name);
}

bool GDScript2SymbolTable::is_static_context() const {
	GDScript2FunctionNode *func = get_current_function();
	return func && func->is_static;
}

// ============================================================================
// GDScript2SymbolTable - Legacy Compatibility
// ============================================================================

void GDScript2SymbolTable::set(const StringName &p_name, const GDScript2Type &p_type) {
	GDScript2Symbol *existing = lookup(p_name);
	if (existing) {
		existing->type = p_type;
	} else {
		GDScript2Symbol *sym = memnew(GDScript2Symbol);
		sym->name = p_name;
		sym->kind = GDScript2SymbolKind::SYMBOL_VARIABLE;
		sym->type = p_type;
		declare(sym);
	}
}

bool GDScript2SymbolTable::get(const StringName &p_name, GDScript2Type &r_type) const {
	GDScript2Symbol *sym = lookup(p_name);
	if (!sym) {
		return false;
	}
	r_type = sym->type;
	return true;
}
