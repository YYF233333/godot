/**************************************************************************/
/*  gdscript2_builtin.h                                                   */
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
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/variant/variant.h"

// ============================================================================
// Builtin Function Signature
// ============================================================================

struct GDScript2BuiltinFunction {
	typedef Variant (*FunctionPtr)(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	StringName name;
	FunctionPtr function = nullptr;
	int min_args = 0;
	int max_args = -1; // -1 = unlimited
	bool is_vararg = false;
	String description;

	GDScript2BuiltinFunction() = default;

	GDScript2BuiltinFunction(const StringName &p_name, FunctionPtr p_func, int p_min_args = 0, int p_max_args = -1) :
			name(p_name), function(p_func), min_args(p_min_args), max_args(p_max_args) {
		is_vararg = (p_max_args == -1);
	}

	Variant call(const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const {
		if (function == nullptr) {
			r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
			return Variant();
		}

		// Check argument count
		if (p_arg_count < min_args) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.expected = min_args;
			return Variant();
		}

		if (!is_vararg && max_args >= 0 && p_arg_count > max_args) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.expected = max_args;
			return Variant();
		}

		return function(p_args, p_arg_count, r_error);
	}
};

// ============================================================================
// Builtin Function Registry
// ============================================================================

class GDScript2BuiltinRegistry {
	static HashMap<StringName, GDScript2BuiltinFunction> functions;
	static bool initialized;

	// Registration helpers
	static void register_function(const StringName &p_name, GDScript2BuiltinFunction::FunctionPtr p_func, int p_min_args = 0, int p_max_args = -1);

	// Core functions
	static Variant func_print(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_print_debug(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_printerr(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_printraw(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_push_error(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_push_warning(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Type conversion
	static Variant func_str(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_int(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_float(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_bool(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Math functions
	static Variant func_abs(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_sign(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_min(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_max(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_clamp(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_floor(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_ceil(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_round(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_sqrt(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_pow(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_sin(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_cos(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_tan(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_exp(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_log(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Array/Container functions
	static Variant func_len(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_range(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Type checking
	static Variant func_typeof(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_type_exists(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_is_instance_valid(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Object functions
	static Variant func_instance_from_id(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_is_instance_of(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// String functions
	static Variant func_char(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_ord(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Variant utilities
	static Variant func_var_to_str(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_str_to_var(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_var_to_bytes(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_bytes_to_var(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_hash(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Resource/Scene functions
	static Variant func_load(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);
	static Variant func_preload(const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

public:
	// Initialize all builtin functions
	static void initialize();
	static void finalize();

	// Query functions
	static bool has_function(const StringName &p_name);
	static const GDScript2BuiltinFunction *get_function(const StringName &p_name);

	// Call a builtin function
	static Variant call_function(const StringName &p_name, const Variant **p_args, int p_arg_count, Callable::CallError &r_error);

	// Get list of all function names
	static void get_function_list(List<StringName> *r_list);
};

// ============================================================================
// Convenience Macros
// ============================================================================

#define GDSCRIPT2_BUILTIN_FUNC(m_name) \
	Variant GDScript2BuiltinRegistry::func_##m_name(const Variant **p_args, int p_arg_count, Callable::CallError &r_error)

#define GDSCRIPT2_CHECK_ARG_COUNT(m_count)                                                                                                                \
	if (p_arg_count != m_count) {                                                                                                                         \
		r_error.error = (p_arg_count < m_count) ? Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS : Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS; \
		r_error.expected = m_count;                                                                                                                       \
		return Variant();                                                                                                                                 \
	}

#define GDSCRIPT2_CHECK_ARG_TYPE(m_idx, m_type)                           \
	if (p_args[m_idx]->get_type() != m_type) {                            \
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT; \
		r_error.argument = m_idx;                                         \
		r_error.expected = m_type;                                        \
		return Variant();                                                 \
	}
