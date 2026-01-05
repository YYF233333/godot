/**************************************************************************/
/*  gdscript2_variant_utils.cpp                                           */
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

#include "gdscript2_variant_utils.h"

bool GDScript2VariantUtils::check_type(const Variant &p_variant, Variant::Type p_expected, String &r_error) {
	if (p_variant.get_type() != p_expected) {
		r_error = "Expected " + get_type_name(p_expected) + " but got " + get_type_name(p_variant);
		return false;
	}
	return true;
}

bool GDScript2VariantUtils::is_numeric(const Variant &p_variant) {
	Variant::Type type = p_variant.get_type();
	return type == Variant::INT || type == Variant::FLOAT;
}

bool GDScript2VariantUtils::is_container(const Variant &p_variant) {
	Variant::Type type = p_variant.get_type();
	return type == Variant::ARRAY || type == Variant::DICTIONARY ||
			type == Variant::PACKED_BYTE_ARRAY || type == Variant::PACKED_INT32_ARRAY ||
			type == Variant::PACKED_INT64_ARRAY || type == Variant::PACKED_FLOAT32_ARRAY ||
			type == Variant::PACKED_FLOAT64_ARRAY || type == Variant::PACKED_STRING_ARRAY ||
			type == Variant::PACKED_VECTOR2_ARRAY || type == Variant::PACKED_VECTOR3_ARRAY ||
			type == Variant::PACKED_COLOR_ARRAY;
}

bool GDScript2VariantUtils::is_iterable(const Variant &p_variant) {
	return is_container(p_variant) || p_variant.get_type() == Variant::STRING;
}

int64_t GDScript2VariantUtils::to_int(const Variant &p_variant, bool &r_valid) {
	r_valid = true;

	switch (p_variant.get_type()) {
		case Variant::NIL:
			return 0;
		case Variant::BOOL:
			return p_variant.operator bool() ? 1 : 0;
		case Variant::INT:
			return p_variant.operator int64_t();
		case Variant::FLOAT:
			return static_cast<int64_t>(p_variant.operator double());
		case Variant::STRING: {
			String s = p_variant.operator String();
			if (s.is_valid_int()) {
				return s.to_int();
			}
			r_valid = false;
			return 0;
		}
		default:
			r_valid = false;
			return 0;
	}
}

double GDScript2VariantUtils::to_float(const Variant &p_variant, bool &r_valid) {
	r_valid = true;

	switch (p_variant.get_type()) {
		case Variant::NIL:
			return 0.0;
		case Variant::BOOL:
			return p_variant.operator bool() ? 1.0 : 0.0;
		case Variant::INT:
			return static_cast<double>(p_variant.operator int64_t());
		case Variant::FLOAT:
			return p_variant.operator double();
		case Variant::STRING: {
			String s = p_variant.operator String();
			if (s.is_valid_float()) {
				return s.to_float();
			}
			r_valid = false;
			return 0.0;
		}
		default:
			r_valid = false;
			return 0.0;
	}
}

bool GDScript2VariantUtils::to_bool(const Variant &p_variant) {
	return p_variant.booleanize();
}

String GDScript2VariantUtils::to_string(const Variant &p_variant) {
	return p_variant.operator String();
}

int GDScript2VariantUtils::get_length(const Variant &p_container, bool &r_valid) {
	r_valid = true;

	switch (p_container.get_type()) {
		case Variant::STRING:
			return p_container.operator String().length();
		case Variant::ARRAY:
			return p_container.operator Array().size();
		case Variant::DICTIONARY:
			return p_container.operator Dictionary().size();
		case Variant::PACKED_BYTE_ARRAY:
			return p_container.operator PackedByteArray().size();
		case Variant::PACKED_INT32_ARRAY:
			return p_container.operator PackedInt32Array().size();
		case Variant::PACKED_INT64_ARRAY:
			return p_container.operator PackedInt64Array().size();
		case Variant::PACKED_FLOAT32_ARRAY:
			return p_container.operator PackedFloat32Array().size();
		case Variant::PACKED_FLOAT64_ARRAY:
			return p_container.operator PackedFloat64Array().size();
		case Variant::PACKED_STRING_ARRAY:
			return p_container.operator PackedStringArray().size();
		case Variant::PACKED_VECTOR2_ARRAY:
			return p_container.operator PackedVector2Array().size();
		case Variant::PACKED_VECTOR3_ARRAY:
			return p_container.operator PackedVector3Array().size();
		case Variant::PACKED_COLOR_ARRAY:
			return p_container.operator PackedColorArray().size();
		default:
			r_valid = false;
			return 0;
	}
}

Variant GDScript2VariantUtils::get_index(const Variant &p_container, const Variant &p_index, bool &r_valid) {
	r_valid = true;
	Variant result = p_container.get(p_index, &r_valid);
	return result;
}

void GDScript2VariantUtils::set_index(Variant &p_container, const Variant &p_index, const Variant &p_value, bool &r_valid) {
	r_valid = true;
	p_container.set(p_index, p_value, &r_valid);
}

Variant GDScript2VariantUtils::safe_add(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	Variant result;
	Variant::evaluate(Variant::OP_ADD, p_left, p_right, result, r_valid);
	return result;
}

Variant GDScript2VariantUtils::safe_subtract(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	Variant result;
	Variant::evaluate(Variant::OP_SUBTRACT, p_left, p_right, result, r_valid);
	return result;
}

Variant GDScript2VariantUtils::safe_multiply(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	Variant result;
	Variant::evaluate(Variant::OP_MULTIPLY, p_left, p_right, result, r_valid);
	return result;
}

Variant GDScript2VariantUtils::safe_divide(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	// Check for division by zero
	if (is_numeric(p_right)) {
		double divisor = to_float(p_right, r_valid);
		if (r_valid && Math::is_zero_approx(divisor)) {
			r_valid = false;
			return Variant();
		}
	}

	Variant result;
	Variant::evaluate(Variant::OP_DIVIDE, p_left, p_right, result, r_valid);
	return result;
}

Variant GDScript2VariantUtils::safe_modulo(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	// Check for modulo by zero
	if (p_right.get_type() == Variant::INT && p_right.operator int64_t() == 0) {
		r_valid = false;
		return Variant();
	}

	Variant result;
	Variant::evaluate(Variant::OP_MODULE, p_left, p_right, result, r_valid);
	return result;
}

bool GDScript2VariantUtils::equals(const Variant &p_left, const Variant &p_right) {
	bool valid;
	Variant result;
	Variant::evaluate(Variant::OP_EQUAL, p_left, p_right, result, valid);
	return valid && result.booleanize();
}

bool GDScript2VariantUtils::less_than(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	Variant result;
	Variant::evaluate(Variant::OP_LESS, p_left, p_right, result, r_valid);
	return r_valid && result.booleanize();
}

bool GDScript2VariantUtils::less_equal(const Variant &p_left, const Variant &p_right, bool &r_valid) {
	Variant result;
	Variant::evaluate(Variant::OP_LESS_EQUAL, p_left, p_right, result, r_valid);
	return r_valid && result.booleanize();
}

String GDScript2VariantUtils::get_type_name(Variant::Type p_type) {
	return Variant::get_type_name(p_type);
}

String GDScript2VariantUtils::get_type_name(const Variant &p_variant) {
	return get_type_name(p_variant.get_type());
}

Variant GDScript2VariantUtils::deep_copy(const Variant &p_variant) {
	return p_variant.duplicate(true);
}

String GDScript2VariantUtils::stringify_for_error(const Variant &p_variant) {
	String result = get_type_name(p_variant) + "(";

	switch (p_variant.get_type()) {
		case Variant::NIL:
			result += "null";
			break;
		case Variant::BOOL:
			result += p_variant.operator bool() ? "true" : "false";
			break;
		case Variant::INT:
			result += itos(p_variant.operator int64_t());
			break;
		case Variant::FLOAT:
			result += rtos(p_variant.operator double());
			break;
		case Variant::STRING:
			result += "\"" + p_variant.operator String() + "\"";
			break;
		case Variant::ARRAY: {
			Array arr = p_variant.operator Array();
			result += "[size=" + itos(arr.size()) + "]";
			break;
		}
		case Variant::DICTIONARY: {
			Dictionary dict = p_variant.operator Dictionary();
			result += "{size=" + itos(dict.size()) + "}";
			break;
		}
		case Variant::OBJECT: {
			Object *obj = p_variant.get_validated_object();
			if (obj) {
				result += obj->get_class();
			} else {
				result += "freed";
			}
			break;
		}
		default:
			result += p_variant.operator String();
			break;
	}

	result += ")";
	return result;
}
