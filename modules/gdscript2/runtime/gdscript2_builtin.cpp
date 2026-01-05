/**************************************************************************/
/*  gdscript2_builtin.cpp                                                 */
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

#include "gdscript2_builtin.h"

#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/variant/variant_utility.h"

HashMap<StringName, GDScript2BuiltinFunction> GDScript2BuiltinRegistry::functions;
bool GDScript2BuiltinRegistry::initialized = false;

// ============================================================================
// Registry Management
// ============================================================================

void GDScript2BuiltinRegistry::register_function(const StringName &p_name, GDScript2BuiltinFunction::FunctionPtr p_func, int p_min_args, int p_max_args) {
	GDScript2BuiltinFunction func(p_name, p_func, p_min_args, p_max_args);
	functions[p_name] = func;
}

void GDScript2BuiltinRegistry::initialize() {
	if (initialized) {
		return;
	}

	// Print functions
	register_function("print", func_print, 0, -1);
	register_function("print_debug", func_print_debug, 0, -1);
	register_function("printerr", func_printerr, 0, -1);
	register_function("printraw", func_printraw, 0, -1);
	register_function("push_error", func_push_error, 1, 1);
	register_function("push_warning", func_push_warning, 1, 1);

	// Type conversion
	register_function("str", func_str, 0, -1);
	register_function("int", func_int, 1, 1);
	register_function("float", func_float, 1, 1);
	register_function("bool", func_bool, 1, 1);

	// Math functions
	register_function("abs", func_abs, 1, 1);
	register_function("sign", func_sign, 1, 1);
	register_function("min", func_min, 2, -1);
	register_function("max", func_max, 2, -1);
	register_function("clamp", func_clamp, 3, 3);
	register_function("floor", func_floor, 1, 1);
	register_function("ceil", func_ceil, 1, 1);
	register_function("round", func_round, 1, 1);
	register_function("sqrt", func_sqrt, 1, 1);
	register_function("pow", func_pow, 2, 2);
	register_function("sin", func_sin, 1, 1);
	register_function("cos", func_cos, 1, 1);
	register_function("tan", func_tan, 1, 1);
	register_function("exp", func_exp, 1, 1);
	register_function("log", func_log, 1, 1);

	// Container functions
	register_function("len", func_len, 1, 1);
	register_function("range", func_range, 1, 3);

	// Type checking
	register_function("typeof", func_typeof, 1, 1);
	register_function("type_exists", func_type_exists, 1, 1);
	register_function("is_instance_valid", func_is_instance_valid, 1, 1);

	// Object functions
	register_function("instance_from_id", func_instance_from_id, 1, 1);
	register_function("is_instance_of", func_is_instance_of, 2, 2);

	// String functions
	register_function("char", func_char, 1, 1);
	register_function("ord", func_ord, 1, 1);

	// Variant utilities
	register_function("var_to_str", func_var_to_str, 1, 1);
	register_function("str_to_var", func_str_to_var, 1, 1);
	register_function("var_to_bytes", func_var_to_bytes, 1, 2);
	register_function("bytes_to_var", func_bytes_to_var, 1, 2);
	register_function("hash", func_hash, 1, 1);

	// Resource/Scene functions
	register_function("load", func_load, 1, 1);
	register_function("preload", func_preload, 1, 1);

	initialized = true;
}

void GDScript2BuiltinRegistry::finalize() {
	functions.clear();
	initialized = false;
}

bool GDScript2BuiltinRegistry::has_function(const StringName &p_name) {
	return functions.has(p_name);
}

const GDScript2BuiltinFunction *GDScript2BuiltinRegistry::get_function(const StringName &p_name) {
	if (functions.has(p_name)) {
		return &functions[p_name];
	}
	return nullptr;
}

Variant GDScript2BuiltinRegistry::call_function(const StringName &p_name, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) {
	const GDScript2BuiltinFunction *func = get_function(p_name);
	if (func) {
		return func->call(p_args, p_arg_count, r_error);
	}

	r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
	return Variant();
}

void GDScript2BuiltinRegistry::get_function_list(List<StringName> *r_list) {
	for (const KeyValue<StringName, GDScript2BuiltinFunction> &E : functions) {
		r_list->push_back(E.key);
	}
}

// ============================================================================
// Print Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(print) {
	String s;
	for (int i = 0; i < p_arg_count; i++) {
		if (i > 0) {
			s += " ";
		}
		s += p_args[i]->operator String();
	}
	print_line(s);
	r_error.error = Callable::CallError::CALL_OK;
	return Variant();
}

GDSCRIPT2_BUILTIN_FUNC(print_debug) {
	String s;
	for (int i = 0; i < p_arg_count; i++) {
		if (i > 0) {
			s += " ";
		}
		s += p_args[i]->operator String();
	}
	print_line("[DEBUG] " + s);
	r_error.error = Callable::CallError::CALL_OK;
	return Variant();
}

GDSCRIPT2_BUILTIN_FUNC(printerr) {
	String s;
	for (int i = 0; i < p_arg_count; i++) {
		if (i > 0) {
			s += " ";
		}
		s += p_args[i]->operator String();
	}
	print_error(s);
	r_error.error = Callable::CallError::CALL_OK;
	return Variant();
}

GDSCRIPT2_BUILTIN_FUNC(printraw) {
	String s;
	for (int i = 0; i < p_arg_count; i++) {
		s += p_args[i]->operator String();
	}
	OS::get_singleton()->print("%s", s.utf8().get_data());
	r_error.error = Callable::CallError::CALL_OK;
	return Variant();
}

GDSCRIPT2_BUILTIN_FUNC(push_error) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	ERR_PRINT(p_args[0]->operator String());
	r_error.error = Callable::CallError::CALL_OK;
	return Variant();
}

GDSCRIPT2_BUILTIN_FUNC(push_warning) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	WARN_PRINT(p_args[0]->operator String());
	r_error.error = Callable::CallError::CALL_OK;
	return Variant();
}

// ============================================================================
// Type Conversion Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(str) {
	String s;
	for (int i = 0; i < p_arg_count; i++) {
		s += p_args[i]->operator String();
	}
	r_error.error = Callable::CallError::CALL_OK;
	return s;
}

GDSCRIPT2_BUILTIN_FUNC(int) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return p_args[0]->operator int64_t();
}

GDSCRIPT2_BUILTIN_FUNC(float) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return p_args[0]->operator double();
}

GDSCRIPT2_BUILTIN_FUNC(bool) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return p_args[0]->booleanize();
}

// ============================================================================
// Math Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(abs) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;

	switch (p_args[0]->get_type()) {
		case Variant::INT:
			return Math::abs(p_args[0]->operator int64_t());
		case Variant::FLOAT:
			return Math::abs(p_args[0]->operator double());
		default:
			return Math::abs(p_args[0]->operator double());
	}
}

GDSCRIPT2_BUILTIN_FUNC(sign) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;

	switch (p_args[0]->get_type()) {
		case Variant::INT: {
			int64_t val = p_args[0]->operator int64_t();
			return val < 0 ? -1 : (val > 0 ? 1 : 0);
		}
		case Variant::FLOAT: {
			double val = p_args[0]->operator double();
			return val < 0.0 ? -1.0 : (val > 0.0 ? 1.0 : 0.0);
		}
		default:
			return 0;
	}
}

GDSCRIPT2_BUILTIN_FUNC(min) {
	if (p_arg_count < 2) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 2;
		return Variant();
	}

	Variant result = *p_args[0];
	for (int i = 1; i < p_arg_count; i++) {
		bool valid = true;
		Variant cmp_result;
		Variant::evaluate(Variant::OP_LESS, *p_args[i], result, cmp_result, valid);
		if (valid && cmp_result.operator bool()) {
			result = *p_args[i];
		}
	}

	r_error.error = Callable::CallError::CALL_OK;
	return result;
}

GDSCRIPT2_BUILTIN_FUNC(max) {
	if (p_arg_count < 2) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 2;
		return Variant();
	}

	Variant result = *p_args[0];
	for (int i = 1; i < p_arg_count; i++) {
		bool valid = true;
		Variant cmp_result;
		Variant::evaluate(Variant::OP_GREATER, *p_args[i], result, cmp_result, valid);
		if (valid && cmp_result.operator bool()) {
			result = *p_args[i];
		}
	}

	r_error.error = Callable::CallError::CALL_OK;
	return result;
}

GDSCRIPT2_BUILTIN_FUNC(clamp) {
	GDSCRIPT2_CHECK_ARG_COUNT(3);

	const Variant &value = *p_args[0];
	const Variant &min_val = *p_args[1];
	const Variant &max_val = *p_args[2];

	r_error.error = Callable::CallError::CALL_OK;

	bool valid;
	Variant result;

	// If value < min, return min
	Variant::evaluate(Variant::OP_LESS, value, min_val, result, valid);
	if (valid && result.booleanize()) {
		return min_val;
	}

	// If value > max, return max
	Variant::evaluate(Variant::OP_GREATER, value, max_val, result, valid);
	if (valid && result.booleanize()) {
		return max_val;
	}

	return value;
}

GDSCRIPT2_BUILTIN_FUNC(floor) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::floor(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(ceil) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::ceil(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(round) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::round(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(sqrt) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::sqrt(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(pow) {
	GDSCRIPT2_CHECK_ARG_COUNT(2);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::pow(p_args[0]->operator double(), p_args[1]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(sin) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::sin(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(cos) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::cos(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(tan) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::tan(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(exp) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::exp(p_args[0]->operator double());
}

GDSCRIPT2_BUILTIN_FUNC(log) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return Math::log(p_args[0]->operator double());
}

// ============================================================================
// Container Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(len) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);

	const Variant &container = *p_args[0];

	switch (container.get_type()) {
		case Variant::STRING:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator String().length();

		case Variant::ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator Array().size();

		case Variant::DICTIONARY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator Dictionary().size();

		case Variant::PACKED_BYTE_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedByteArray().size();

		case Variant::PACKED_INT32_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedInt32Array().size();

		case Variant::PACKED_INT64_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedInt64Array().size();

		case Variant::PACKED_FLOAT32_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedFloat32Array().size();

		case Variant::PACKED_FLOAT64_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedFloat64Array().size();

		case Variant::PACKED_STRING_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedStringArray().size();

		case Variant::PACKED_VECTOR2_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedVector2Array().size();

		case Variant::PACKED_VECTOR3_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedVector3Array().size();

		case Variant::PACKED_COLOR_ARRAY:
			r_error.error = Callable::CallError::CALL_OK;
			return container.operator PackedColorArray().size();

		default:
			r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
			r_error.argument = 0;
			return Variant();
	}
}

GDSCRIPT2_BUILTIN_FUNC(range) {
	if (p_arg_count < 1 || p_arg_count > 3) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		return Variant();
	}

	int64_t start = 0;
	int64_t end = 0;
	int64_t step = 1;

	if (p_arg_count == 1) {
		end = p_args[0]->operator int64_t();
	} else if (p_arg_count == 2) {
		start = p_args[0]->operator int64_t();
		end = p_args[1]->operator int64_t();
	} else {
		start = p_args[0]->operator int64_t();
		end = p_args[1]->operator int64_t();
		step = p_args[2]->operator int64_t();
	}

	if (step == 0) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		return Variant();
	}

	Array result;
	if (step > 0) {
		for (int64_t i = start; i < end; i += step) {
			result.push_back(i);
		}
	} else {
		for (int64_t i = start; i > end; i += step) {
			result.push_back(i);
		}
	}

	r_error.error = Callable::CallError::CALL_OK;
	return result;
}

// ============================================================================
// Type Checking Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(typeof) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return (int)p_args[0]->get_type();
}

GDSCRIPT2_BUILTIN_FUNC(type_exists) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	GDSCRIPT2_CHECK_ARG_TYPE(0, Variant::STRING);

	String class_name = p_args[0]->operator String();
	r_error.error = Callable::CallError::CALL_OK;
	return ClassDB::class_exists(class_name);
}

GDSCRIPT2_BUILTIN_FUNC(is_instance_valid) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);

	if (p_args[0]->get_type() != Variant::OBJECT) {
		r_error.error = Callable::CallError::CALL_OK;
		return false;
	}

	Object *obj = p_args[0]->get_validated_object();
	r_error.error = Callable::CallError::CALL_OK;
	return obj != nullptr;
}

// ============================================================================
// Object Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(instance_from_id) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);

	ObjectID id = p_args[0]->operator ObjectID();
	Object *obj = ObjectDB::get_instance(id);

	r_error.error = Callable::CallError::CALL_OK;
	return Variant(obj);
}

GDSCRIPT2_BUILTIN_FUNC(is_instance_of) {
	GDSCRIPT2_CHECK_ARG_COUNT(2);

	const Variant &instance = *p_args[0];
	const Variant &type_or_class = *p_args[1];

	if (instance.get_type() != Variant::OBJECT) {
		r_error.error = Callable::CallError::CALL_OK;
		return false;
	}

	Object *obj = instance.get_validated_object();
	if (obj == nullptr) {
		r_error.error = Callable::CallError::CALL_OK;
		return false;
	}

	// Check if type_or_class is a class name string
	if (type_or_class.get_type() == Variant::STRING) {
		String class_name = type_or_class.operator String();
		r_error.error = Callable::CallError::CALL_OK;
		return obj->is_class(class_name);
	}

	// Check if type_or_class is a class object
	if (type_or_class.get_type() == Variant::OBJECT) {
		Object *type_obj = type_or_class.get_validated_object();
		if (type_obj) {
			r_error.error = Callable::CallError::CALL_OK;
			return obj->is_class(type_obj->get_class());
		}
	}

	r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
	return false;
}

// ============================================================================
// String Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(char) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);

	int64_t code = p_args[0]->operator int64_t();
	if (code < 0 || code > 0x10ffff) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		return Variant();
	}

	char32_t c = static_cast<char32_t>(code);
	r_error.error = Callable::CallError::CALL_OK;
	return String::chr(c);
}

GDSCRIPT2_BUILTIN_FUNC(ord) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	GDSCRIPT2_CHECK_ARG_TYPE(0, Variant::STRING);

	String s = p_args[0]->operator String();
	if (s.is_empty()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_OK;
	return (int64_t)s[0];
}

// ============================================================================
// Variant Utility Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(var_to_str) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return p_args[0]->get_construct_string();
}

GDSCRIPT2_BUILTIN_FUNC(str_to_var) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	GDSCRIPT2_CHECK_ARG_TYPE(0, Variant::STRING);

	String s = p_args[0]->operator String();

	// Simplified implementation - just return the string
	// TODO: Implement proper variant parsing
	r_error.error = Callable::CallError::CALL_OK;
	return Variant(s);
}

GDSCRIPT2_BUILTIN_FUNC(var_to_bytes) {
	if (p_arg_count < 1 || p_arg_count > 2) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		return Variant();
	}

	bool full_objects = false;
	if (p_arg_count == 2) {
		full_objects = p_args[1]->booleanize();
	}

	int len;
	Error err = encode_variant(*p_args[0], nullptr, len, full_objects);
	if (err != OK) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		return Variant();
	}

	PackedByteArray bytes;
	bytes.resize(len);
	encode_variant(*p_args[0], bytes.ptrw(), len, full_objects);

	r_error.error = Callable::CallError::CALL_OK;
	return bytes;
}

GDSCRIPT2_BUILTIN_FUNC(bytes_to_var) {
	if (p_arg_count < 1 || p_arg_count > 2) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		return Variant();
	}

	GDSCRIPT2_CHECK_ARG_TYPE(0, Variant::PACKED_BYTE_ARRAY);

	bool allow_objects = false;
	if (p_arg_count == 2) {
		allow_objects = p_args[1]->booleanize();
	}

	PackedByteArray bytes = p_args[0]->operator PackedByteArray();
	Variant result;
	int len;

	Error err = decode_variant(result, bytes.ptr(), bytes.size(), &len, allow_objects);
	if (err != OK) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_OK;
	return result;
}

GDSCRIPT2_BUILTIN_FUNC(hash) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	r_error.error = Callable::CallError::CALL_OK;
	return p_args[0]->hash();
}

// ============================================================================
// Resource/Scene Functions
// ============================================================================

GDSCRIPT2_BUILTIN_FUNC(load) {
	GDSCRIPT2_CHECK_ARG_COUNT(1);
	GDSCRIPT2_CHECK_ARG_TYPE(0, Variant::STRING);

	String path = p_args[0]->operator String();
	Ref<Resource> res = ResourceLoader::load(path);

	r_error.error = Callable::CallError::CALL_OK;
	return res;
}

GDSCRIPT2_BUILTIN_FUNC(preload) {
	// Preload is typically handled at compile time
	// At runtime, it's equivalent to load
	return func_load(p_args, p_arg_count, r_error);
}
