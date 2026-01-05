/**************************************************************************/
/*  gdscript2_variant_utils.h                                             */
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

#include "core/variant/variant.h"

// Utility class for Variant operations in GDScript2
class GDScript2VariantUtils {
public:
	// Type checking with better error messages
	static bool check_type(const Variant &p_variant, Variant::Type p_expected, String &r_error);
	static bool is_numeric(const Variant &p_variant);
	static bool is_container(const Variant &p_variant);
	static bool is_iterable(const Variant &p_variant);

	// Safe conversions
	static int64_t to_int(const Variant &p_variant, bool &r_valid);
	static double to_float(const Variant &p_variant, bool &r_valid);
	static bool to_bool(const Variant &p_variant);
	static String to_string(const Variant &p_variant);

	// Container operations
	static int get_length(const Variant &p_container, bool &r_valid);
	static Variant get_index(const Variant &p_container, const Variant &p_index, bool &r_valid);
	static void set_index(Variant &p_container, const Variant &p_index, const Variant &p_value, bool &r_valid);

	// Safe arithmetic operations with error checking
	static Variant safe_add(const Variant &p_left, const Variant &p_right, bool &r_valid);
	static Variant safe_subtract(const Variant &p_left, const Variant &p_right, bool &r_valid);
	static Variant safe_multiply(const Variant &p_left, const Variant &p_right, bool &r_valid);
	static Variant safe_divide(const Variant &p_left, const Variant &p_right, bool &r_valid);
	static Variant safe_modulo(const Variant &p_left, const Variant &p_right, bool &r_valid);

	// Comparison operations
	static bool equals(const Variant &p_left, const Variant &p_right);
	static bool less_than(const Variant &p_left, const Variant &p_right, bool &r_valid);
	static bool less_equal(const Variant &p_left, const Variant &p_right, bool &r_valid);

	// Type name utilities
	static String get_type_name(Variant::Type p_type);
	static String get_type_name(const Variant &p_variant);

	// Deep copy
	static Variant deep_copy(const Variant &p_variant);

	// Stringify for debugging
	static String stringify_for_error(const Variant &p_variant);
};
